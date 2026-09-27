#include "test_mode.h"
#include "nes/config.h"
#include "nes/ppu_sim.h"
#include "nes/apu_sim.h"
#include "game/nmi.h"
#include "game/ppu_registers.h"
#include "game/draw.h"
#include "game/sound_engine.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Детерминированный headless-режим для регрессионных тестов.
 *
 * Обычный режим: game-поток спит в nmi_wait()/vblank_wait() на семафорах,
 * render-поток раз в 1/60 с делает кадр (sdl_run.c::update_game).
 * Тестовый режим: без SDL и без потоков. Игровая логика крутится прямо в
 * main, а каждое ожидание семафора синхронно выполняет один кадр — ту же
 * последовательность, что update_game(). Это «бесконечно быстрый CPU»:
 * игра никогда не опаздывает к кадру, поэтому результат не зависит от
 * тайминга машины. Power-on RAM уже детерминирована (фиксированный seed
 * в nes_init). */

int test_mode_active = 0;

static uint32_t pixels[NES_SCREEN_TOTAL];

static unsigned total_ticks;
static unsigned checkpoint_every = 60;
static unsigned tick;
static FILE *out;
static const char *dump_dir;

/* Скрипт ввода: с кадра frame держатся кнопки p1/p2 до следующей записи. */
typedef struct { unsigned frame; uint8_t p1, p2; } input_step_t;
static input_step_t *steps;
static size_t steps_n;
static uint8_t cur_p1, cur_p2;

static uint64_t fnv1a(uint64_t h, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x100000001B3ull;
    }
    return h;
}

static int load_input(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return -1; }
    char line[256];
    unsigned lineno = 0, prev = 0;
    while (fgets(line, sizeof line, f)) {
        lineno++;
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        unsigned frame, p1 = 0, p2 = 0;
        int n = sscanf(line, "%u %x %x", &frame, &p1, &p2);
        if (n <= 0) continue;
        if (n < 2 || p1 > 0xFF || p2 > 0xFF || (steps_n && frame < prev)) {
            fprintf(stderr, "%s:%u: expected \"<frame> <p1 hex> [<p2 hex>]\" "
                            "with non-decreasing frames\n", path, lineno);
            fclose(f);
            return -1;
        }
        input_step_t *grown = realloc(steps, (steps_n + 1) * sizeof *steps);
        if (!grown) { fclose(f); return -1; }
        steps = grown;
        steps[steps_n++] = (input_step_t){ frame, (uint8_t)p1, (uint8_t)p2 };
        prev = frame;
    }
    fclose(f);
    return 0;
}

static void update_input(void) {
    for (size_t i = 0; i < steps_n && steps[i].frame <= tick; i++) {
        cur_p1 = steps[i].p1;
        cur_p2 = steps[i].p2;
    }
}

static void write_bmp(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    const uint32_t w = NES_SCREEN_W, h = NES_SCREEN_H;
    const uint32_t data = w * h * 4u, off = 14u + 40u;
    uint8_t hdr[54] = { 'B', 'M' };
    #define PUT32(o, v) do { uint32_t v_ = (v); hdr[o] = (uint8_t)v_; hdr[o+1] = (uint8_t)(v_ >> 8); \
                             hdr[o+2] = (uint8_t)(v_ >> 16); hdr[o+3] = (uint8_t)(v_ >> 24); } while (0)
    PUT32(2, off + data);
    PUT32(10, off);
    PUT32(14, 40u);
    PUT32(18, w);
    PUT32(22, h);
    hdr[26] = 1;   /* planes */
    hdr[28] = 32;  /* bpp: BGRA little-endian == ARGB8888 uint32 */
    PUT32(34, data);
    #undef PUT32
    fwrite(hdr, 1, sizeof hdr, f);
    for (uint32_t y = h; y-- > 0;)  /* BMP хранит строки снизу вверх */
        fwrite(&pixels[y * w], 4, w, f);
    fclose(f);
}

static void checkpoint(void) {
    uint64_t fb = fnv1a(0xCBF29CE484222325ull, pixels, sizeof pixels);
    fprintf(out, "%06u %016llx %016llx\n", tick,
            (unsigned long long)fb, (unsigned long long)apu_trace_hash);
    if (dump_dir) {
        char path[1024];
        snprintf(path, sizeof path, "%s/frame_%06u.bmp", dump_dir, tick);
        write_bmp(path);
    }
}

/* Один кадр — зеркало sdl_run.c::update_game() без SDL-вывода. */
static void frame_tick(void) {
    update_input();
    ppu_set_vblank_flag();
    if (atomic_load_explicit(&game_in_nmi_wait, memory_order_acquire)) {
        ppu_render(pixels);
        nmi();
    } else if ((PPU_CTRL_REG1 & 0x80u) != 0u) {  /* NMI включён (см. sdl_run.c) */
        play_sound();
    }
    tick++;
    if (tick % checkpoint_every == 0u || tick == total_ticks)
        checkpoint();
    if (tick >= total_ticks)
        plat_running = 0;  /* nmi_wait()/vblank_wait() сделают longjmp в nes_thread */
}

void test_mode_sem_wait(void) {
    /* Семафор «поднимается» ровно одним кадром. Для vblank_wait() кадр
     * ставит бит VBlank в PPU_STATUS; для nmi_wait() — выполняет nmi(). */
    if (plat_running) frame_tick();
}

uint8_t test_mode_buttons(int player) {
    return player == 0 ? cur_p1 : cur_p2;
}

int test_mode_run(unsigned frames, unsigned every, const char *input_path,
                  const char *out_path, const char *dump) {
    total_ticks = frames;
    if (every) checkpoint_every = every;
    dump_dir = dump;
    out = stdout;
    if (input_path && load_input(input_path) != 0) return 1;
    if (out_path && !(out = fopen(out_path, "w"))) { perror(out_path); return 1; }

    test_mode_active = 1;
    apu_trace_enabled = 1;
    apu_init();
    nes_thread(NULL);

    if (out != stdout) fclose(out);
    free(steps);
    return tick == total_ticks ? 0 : 1;
}
