#include "apu_sim.h"
#include "config.h"
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

/* Region-dependent parameters (CPU clock, frame seq rate, noise table) come
   from RegionParams via current_region — see config.h/config.c.
   Sample rate is fixed by the audio device, not the chip. */
#define SAMPLE_RATE 44100

#define RING_SIZE 512
#define RING_MASK (RING_SIZE - 1)

typedef struct {
    uint8_t reg;
    uint8_t value;
} APU_Event;

typedef struct {
    uint8_t ctrl, sweep, timer_lo, timer_hi;
    int enabled;
    
    float timer_counter;
    int duty_step;
    int volume;
    int duty;
    
    int length_counter;
    int linear_counter;
    int linear_reload;
    int env_vol;
    int env_timer;
    int env_start_flag;
    int sweep_timer;
    int sweep_enable;
    int sweep_reload;
    
    uint16_t lfsr;
    uint8_t noise_period_reg;
} Channel;

static Channel ch[4];
static uint8_t apu_status = 0;
static int frame_counter_mode = 4;
static int frame_counter_step = 0;

static APU_Event ring_buffer[RING_SIZE];
static atomic_uint ring_head = 0;
static atomic_uint ring_tail = 0;

static const uint8_t length_table[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const uint8_t duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

static const uint8_t triangle_wave[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

void apu_init(void) {
    memset(ch, 0, sizeof(ch));
    ch[3].lfsr = 1;
    for (int i = 0; i < 4; i++) ch[i].timer_counter = 0.0f;
    atomic_store(&ring_head, 0);
    atomic_store(&ring_tail, 0);
}

static void apply_register_write(uint8_t reg, uint8_t value) {
    if (reg < 0x10) {
        int c = reg / 4;
        int r = reg % 4;
        Channel *chp = &ch[c];
        switch (r) {
            case 0:
                chp->ctrl = value;
                chp->duty = value >> 6;
                chp->volume = value & 0x0F;
                /* $4008 write updates the control flag and reload value only;
                   the linear counter itself is updated by the reload mechanism
                   on the next quarter-frame after a $400B write. */
                break;
            case 1:
                chp->sweep = value;
                chp->sweep_enable = (value & 0x80) != 0;
                chp->sweep_reload = 1;  /* side-effect of $4001/$4005 write */
                break;
            case 2:
                chp->timer_lo = value;
                if (c == 3) chp->noise_period_reg = value;
                break;
            case 3:
                /* $4003/$4007/$400B/$400F. Per-channel side effects:
                   - Pulse ($4003/$4007): sequencer restart (duty_step=0) + envelope restart
                     (APU_PULSE.md §Side effects). Period divider is NOT reset.
                   - Triangle ($400B): linear-counter reload flag set (APU_TRIANGLE.md §Registers).
                     No envelope (triangle has no envelope unit).
                   - Noise ($400F): envelope restart (APU_NOISE.md / APU_ENVELOPE.md).
                     LFSR is NOT reset.
                   - All: length counter reloads only while channel is enabled
                     (APU_LENGTH_COUNTER.md §Clocking). */
                chp->timer_hi = value;
                if (c < 2) {
                    chp->duty_step = 0;
                    chp->env_start_flag = 1;
                } else if (c == 2) {
                    chp->linear_reload = 1;
                } else {
                    chp->env_start_flag = 1;
                }
                if (chp->enabled) {
                    chp->length_counter = length_table[value >> 3];
                }
                break;
        }
    }
}

/* SPSC ring: single producer (CPU/emu thread), single consumer (audio callback).
   Write payload first, then publish new head with release-store so the consumer's
   acquire-load on head establishes happens-before with the payload write. */
static inline void ring_push(uint8_t reg, uint8_t value) {
    unsigned int head = atomic_load_explicit(&ring_head, memory_order_relaxed);
    ring_buffer[head & RING_MASK].reg = reg;
    ring_buffer[head & RING_MASK].value = value;
    atomic_store_explicit(&ring_head, head + 1, memory_order_release);
}

void apu_write(uint8_t reg, uint8_t value) { ring_push(reg, value); }
void apu_write_status(uint8_t value)       { ring_push(0xFE, value); }
void apu_write_frame(uint8_t value)        { ring_push(0xFF, value); }

static void clock_quarter(void);
static void clock_half(void);

static void process_ring_buffer(void) {
    uint32_t head = atomic_load_explicit(&ring_head, memory_order_acquire);
    uint32_t tail = atomic_load_explicit(&ring_tail, memory_order_relaxed);
    
    while (tail < head) {
        APU_Event *evt = &ring_buffer[tail & RING_MASK];
        
        if (evt->reg == 0xFE) {
            /* APU_LENGTH_COUNTER.md §Clocking: clearing $4015 bit forces length to 0;
               setting it has no immediate effect (length only reloads on $4003/$4007/
               $400B/$400F writes while enabled). */
            for (int i = 0; i < 4; i++) {
                ch[i].enabled = (evt->value & (1 << i)) != 0;
                if (!ch[i].enabled) ch[i].length_counter = 0;
            }
            apu_status = evt->value;
        } else if (evt->reg == 0xFF) {
            frame_counter_mode = (evt->value & 0x80) ? 5 : 4;
            frame_counter_step = 0;
            /* Writing $4017 with bit 7 set immediately clocks quarter + half frame. */
            if (evt->value & 0x80) {
                clock_quarter();
                clock_half();
            }
        } else {
            apply_register_write(evt->reg, evt->value);
        }
        
        tail++;
    }
    atomic_store_explicit(&ring_tail, tail, memory_order_relaxed);
}

static void env_step(Channel *c) {
    if (c->env_start_flag) {
        c->env_start_flag = 0;
        c->env_vol = 15;
        c->env_timer = c->volume;
    } else {
        if (c->env_timer > 0) {
            c->env_timer--;
        } else {
            c->env_timer = c->volume;
            if (c->env_vol > 0) {
                c->env_vol--;
            } else if (c->ctrl & 0x20) {
                c->env_vol = 15;
            }
        }
    }
}

/* Compute the would-be next period of a pulse channel as seen by its sweep unit.
   Used both by the sweep unit (to update the period) and by the renderer
   (to decide whether the channel is muted). */
static int pulse_sweep_target(const Channel *c, int is_ch1) {
    uint16_t t = c->timer_lo | ((c->timer_hi & 7) << 8);
    int shift = c->sweep & 7;
    int change = t >> shift;
    if (c->sweep & 0x08) change = -change - (is_ch1 ? 1 : 0);
    return (int)t + change;
}

static int pulse_is_muted(const Channel *c, int is_ch1) {
    uint16_t t = c->timer_lo | ((c->timer_hi & 7) << 8);
    if (t < 8) return 1;
    return pulse_sweep_target(c, is_ch1) > 0x7FF;
}

static void sweep_step(Channel *c, int is_ch1) {
    int period = (c->sweep >> 4) & 7;
    int shift = c->sweep & 7;

    int do_clock_period = 0;
    if (c->sweep_reload) {
        /* If divider was already 0, the channel period is still updated on this tick. */
        if (c->sweep_timer == 0 && c->sweep_enable && shift > 0) do_clock_period = 1;
        c->sweep_timer = period;
        c->sweep_reload = 0;
    } else if (c->sweep_timer > 0) {
        c->sweep_timer--;
    } else {
        c->sweep_timer = period;
        if (c->sweep_enable && shift > 0) do_clock_period = 1;
    }

    /* Per APU_SWEEP.md §Updating the period: the period is set to the target only
       when the divider just clocked, sweep is enabled, shift>0, and not muting.
       (Length counter is NOT part of the sweep condition — it only gates the
       mixer output.) */
    if (do_clock_period && !pulse_is_muted(c, is_ch1)) {
        int target = pulse_sweep_target(c, is_ch1);
        c->timer_lo = target & 0xFF;
        c->timer_hi = (c->timer_hi & 0xF8) | ((target >> 8) & 7);
    }
}

/* Quarter-frame tick: envelopes (pulse/noise) + triangle linear counter.
   Triangle has no envelope generator — its $4008 bits 0-6 are the linear-counter
   reload value, not envelope rate, so env_step would mis-parse them. */
static void clock_quarter(void) {
    env_step(&ch[0]);
    env_step(&ch[1]);
    env_step(&ch[3]);
    if (ch[2].linear_reload) {
        ch[2].linear_counter = ch[2].ctrl & 0x7F;
        if (!(ch[2].ctrl & 0x80)) ch[2].linear_reload = 0;
    } else if (ch[2].linear_counter > 0) {
        ch[2].linear_counter--;
    }
}

/* Half-frame tick: length counters + sweep units. */
static void clock_half(void) {
    for (int i = 0; i < 4; i++) {
        Channel *c = &ch[i];
        /* Halt bit lives in $4008.7 for triangle, $4000/$400C.5 for pulses/noise. */
        int halt = (i == 2) ? (c->ctrl & 0x80) : (c->ctrl & 0x20);
        if (!halt && c->length_counter > 0) c->length_counter--;
        if (i < 2) sweep_step(c, i == 0);
    }
}

static void process_frame_sequencer(float dt) {
    static float frame_seq_timer = 0.0f;
    const float step_period = 1.0f / region_params(current_region)->frame_seq_hz;
    frame_seq_timer += dt;
    while (frame_seq_timer >= step_period) {
        frame_seq_timer -= step_period;
        if (frame_counter_mode == 4) {
            /* 4-step: Q at every step; H at steps 1 and 3. */
            clock_quarter();
            if (frame_counter_step == 1 || frame_counter_step == 3) clock_half();
            frame_counter_step = (frame_counter_step + 1) % 4;
        } else {
            /* 5-step: Q at step 0; Q+H at step 1; Q at step 2; nothing at step 3; Q+H at step 4. */
            if (frame_counter_step == 0 || frame_counter_step == 2) {
                clock_quarter();
            } else if (frame_counter_step == 1 || frame_counter_step == 4) {
                clock_quarter();
                clock_half();
            }
            frame_counter_step = (frame_counter_step + 1) % 5;
        }
    }
}

void audio_callback(void *user, uint8_t *stream, int len) {
    int16_t *buf = (int16_t*)stream;
    int samples = len / 2;
    
    /* APU_MIXER.md §Hardware Filters — NES post-DAC chain:
       90 Hz HPF → 440 Hz HPF → 14 kHz LPF, all first-order IIR.
       Coefficients: α = exp(-2π·fc/Fs), Fs = SAMPLE_RATE.
       HPF form: y[n] = α·(y[n-1] + x[n] - x[n-1])  (DC-blocking diff + leak)
       LPF form: y[n] = α·y[n-1] + (1-α)·x[n]      (single-pole) */
    static const float HPF1_A = 0.98726f;  /*  90 Hz @ 44100 */
    static const float HPF2_A = 0.93923f;  /* 440 Hz @ 44100 */
    static const float LPF_A  = 0.13606f;  /*  14 kHz @ 44100 (α = 1/τ-ish, see formula) */
    static float hp1_in = 0.0f, hp1_out = 0.0f;
    static float hp2_in = 0.0f, hp2_out = 0.0f;
    static float lp_out = 0.0f;
    /* Single approximate HPF (~28 Hz) — keeps full bass; original behavior. */
    static float hp_prev_out = 0.0f;
    static float hp_prev_in = 0.0f;


    process_ring_buffer();
    const RegionParams *rp = region_params(current_region);
    const float cpu_cycles_per_sample = (float)rp->cpu_clock_hz / (float)SAMPLE_RATE;
    const uint16_t *noise_period = rp->noise_period;
    const float dt_per_sample = 1.0f / (float)SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        process_frame_sequencer(dt_per_sample);
        float pulse_sum = 0.0f;
        float triangle_out = 0.0f;
        float noise_sum = 0.0f;
        
        for (int ch_idx = 0; ch_idx < 4; ch_idx++) {
            Channel *c = &ch[ch_idx];
            if (!c->enabled) continue;
            
            /* Pulse/noise are gated by length counter (length==0 → silence).
               Triangle has no early-out: per APU_TRIANGLE.md, when linear or length
               is 0 the sequencer halts but output stays at the current step (frozen DC),
               not silence. The triangle branch below conditionally advances the step. */
            if (ch_idx != 2 && c->length_counter <= 0) continue;
            
            uint16_t timer = c->timer_lo | ((c->timer_hi & 7) << 8);
            
            int active_vol = (c->ctrl & 0x10) ? c->volume : c->env_vol;
            /* Don't early-out on volume==0: timer/LFSR/duty-step must keep advancing
               so phase stays correct when the channel becomes audible again. */

            int sample_val = 0;

            if (ch_idx < 2) { // Pulse
                c->timer_counter -= cpu_cycles_per_sample;

                while (c->timer_counter <= 0.0f) {
                    c->timer_counter += (timer + 1) * 2.0f;
                    c->duty_step = (c->duty_step - 1) & 7;
                }

                if (!pulse_is_muted(c, ch_idx == 0)) {
                    sample_val = duty_table[c->duty][c->duty_step] ? active_vol : 0;
                }

                pulse_sum += sample_val;
            } else if (ch_idx == 2) { // Triangle
                /* Sequencer clocks only while both linear and length counters are
                   nonzero (APU_TRIANGLE.md §Sequencer). When halted, the step stays
                   put and the output remains at its current level (frozen DC). */
                if (c->linear_counter > 0 && c->length_counter > 0) {
                    c->timer_counter -= cpu_cycles_per_sample;
                    while (c->timer_counter <= 0.0f) {
                        c->timer_counter += (timer + 1);
                        c->duty_step = (c->duty_step + 1) & 31;
                    }
                }

                /* Mute at ultrasonic periods (timer < 2): real hardware emits an
                   inaudible >NYQ tone which aliases into clicks at 44.1 kHz mix.
                   Wiki notes this trade-off "at the expense of accuracy". */
                if (timer >= 2) {
                    triangle_out = (float)triangle_wave[c->duty_step];
                }
            } else if (ch_idx == 3) { // Noise
                c->timer_counter -= cpu_cycles_per_sample;
                uint16_t period = noise_period[c->noise_period_reg & 0x0F];
                
                while (c->timer_counter <= 0.0f) {
                    c->timer_counter += period;
                    int bit = (c->lfsr & 1) ^ ((c->lfsr >> ((c->noise_period_reg & 0x80) ? 6 : 1)) & 1);
                    c->lfsr = (c->lfsr >> 1) | (bit << 14);
                }
                if (!(c->lfsr & 1)) {
                    noise_sum += active_vol;
                }
            }
        }
        
        /* Non-linear DAC mixing (NES hardware behavior) */
        float pulse_out = 0.0f;
        if (pulse_sum > 0.0f) {
            pulse_out = 95.88f / ((8128.0f / pulse_sum) + 100.0f);
        }
        
        /* NES mixer (direct float form, APU_MIXER.md §Output Formula):
           tnd_out = 159.79 / (1/(triangle/8227 + noise/12241 + dmc/22638) + 100).
           Channel weighting (3× tri, 2× noise) belongs to the integer-LUT approximation
           tnd_table[3*tri + 2*noise + dmc], NOT to the direct form — divisors already
           encode the weights. Multiplying here too would over-boost tri/noise. */
        float tnd_input = triangle_out / 8227.0f + noise_sum / 12241.0f;
        float tnd_out = 0.0f;
        if (tnd_input > 0.0f) {
            tnd_out = 159.79f / ((1.0f / tnd_input) + 100.0f);
        }
        
        float mixed = (pulse_out + tnd_out) * 16000.0f;

        float filtered;
        if (apu_filters_enabled) {
            /* HPF 90 Hz */
            float h1 = HPF1_A * (hp1_out + mixed - hp1_in);
            hp1_in  = mixed;
            hp1_out = h1;
            /* HPF 440 Hz */
            float h2 = HPF2_A * (hp2_out + h1 - hp2_in);
            hp2_in  = h1;
            hp2_out = h2;
            /* LPF 14 kHz */
            lp_out = LPF_A * lp_out + (1.0f - LPF_A) * h2;
            filtered = lp_out;
        } else {
            filtered = 0.996f * (hp_prev_out + mixed - hp_prev_in);
            hp_prev_out = filtered;
            hp_prev_in = mixed;
        }

        int out = (int)filtered;
        if (out > 32767) out = 32767;
        else if (out < -32768) out = -32768;
        buf[i] = (int16_t)out;
    }
}
