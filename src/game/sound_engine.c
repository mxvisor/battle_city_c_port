#include "sound_engine.h"
#include "apu_registers.h"
#include "zeropage.h"
#include "bss.h"
#include <stdbool.h>
#include <stddef.h>

/* ── Frequency lookup table (NES APU note periods, big-endian values) ── */
const uint16_t Frequency_LUT[12] = {
    0xF207u, 0x8007u, 0x1407u, 0xAE06u, 0x4306u,
    0xF405u, 0x9E05u, 0x4E05u, 0x0205u, 0xBA04u, 0x7604u,
    0x3604u
};

/* ── ROM sound data (28 slots) ── */
static const uint8_t s_snd_Pause[]             = {2,0x82,0x7F,0x40,0x64,0x1B,0x2B,0x3B,0x1C,0x2C,0x3C,0x6C,0x53,0xE8};
static const uint8_t s_snd_Battle1[]           = {1,0x81,0x7F,0x40,0xEF,0x68,0x1B,0x2B,0x33,0xF0,2,6,0x33,0x43,0x53,0xF0,2,0x0C,0x43,0x53,4,0xF0,2,0x12,0x5B,0x0C,0x1C,0xF0,2,0x18,0x78,0x1C,0x68,0x1C,0x1C,0x1C,0x78,0x1C,0xE8};
static const uint8_t s_snd_Battle2[]           = {3,0x10,0x7F,8,0x78,0x1A,0x68,0x1A,0xF1,3,7,0x78,0x32,0x68,0x32,0xF1,3,0x0E,0x78,0x42,0x68,0x42,0xF1,3,0x15,0x5A,0xF1,3,0x19,0x0B,0xF1,3,0x1D,0x78,0x52,0x68,0x52,0xF1,3,0x24,0x78,0x52,0xE8};
static const uint8_t s_snd_Battle3[]           = {2,0x81,0x7F,0x40,0x78,0x51,0x68,0x51,0xF2,3,7,0x78,0x0A,0x68,0x0A,0xF2,3,0x0E,0x78,0x1A,0x68,0x1A,0xF2,3,0x15,0x32,0xF2,3,0x19,0x42,0xF2,3,0x1D,0x78,0x3A,0x68,0x3A,0xF2,3,0x24,0x78,0x3A,0xE8};
static const uint8_t s_snd_AncillaryLife1[]    = {1,0xA0,0x7F,0x40,0x66,0x1C,0x3C,0x1C,0x53,0x1C,0x3C,5,0x72,0x54,0xE8};
static const uint8_t s_snd_AncillaryLife2[]    = {2,0x90,0x7F,0x40,0x62,0x38,0x66,0xEA,0x20,0x3B,0x53,0x3B,0x1B,0x3B,0x53,0x1C,0x6A,0x14,0xE8};
static const uint8_t s_snd_BonusTaken[]        = {2,0x80,0x7F,0x40,0x63,0x52,0x1B,0x3B,0x53,0x4A,0x13,0x33,0x4B,0x1B,0x3B,0x53,0x1C,0x3C,0xE8};
static const uint8_t s_snd_PlayerExplode[]     = {4,0x1F,0x7F,0x30,0x0A,0x62,0x49,0x49,0xEA,0x1E,0x49,0x49,0xEA,0x1D,0x49,0x49,0xEA,0x1C,0x49,0x49,0xEA,0x1B,0x49,0x49,0xEA,0x1A,0x49,0xEA,0x19,0x49,0xEA,0x18,0x49,0xE8};
static const uint8_t s_snd_Unknown1[]          = {2,0x1F,0x7F,0x30,0x62,0,1,0,0xEA,0x1E,1,0,0xEA,0x1D,1,0,1,0,0xEA,0x1C,1,0xEA,0x1B,0,0xEA,0x1A,1,0xEA,0x19,0,0xE8};
static const uint8_t s_snd_BonusAppears[]      = {2,0x60,0x7F,0x40,0x64,0x52,0x3A,0x52,3,0x52,3,0x13,0x1B,0xE8};
static const uint8_t s_snd_EnemyExplode[]      = {4,0x1F,0x7F,0x40,0x0A,0x62,0x51,0xEA,0x1E,0x51,0xEA,8,0x6A,0x51,0xE8};
static const uint8_t s_snd_HQExplode[]         = {2,0x20,0x7F,0x30,0x63,0x1A,0x12,0x51,0x31,0x19,0x11,0x50,0x30,0x18,0xE8};
static const uint8_t s_snd_BrickRicochet[]     = {3,7,0x7F,8,0x61,0x3A,0x13,0x22,0xE8};
static const uint8_t s_snd_ArmourRicoWall[]    = {2,0xD5,0x7F,0,0x62,0x1C,0x1D,0xE8};
static const uint8_t s_snd_ArmourRicoTank[]    = {2,0x40,0x7F,0,0x61,0x3D,0x62,0x45,0xEA,0x10,0x28,0xE8};
static const uint8_t s_snd_Shoot[]             = {1,0x8F,0x82,0x10,0x6F,0x2C,0xE8};
static const uint8_t s_snd_Ice[]               = {1,0x1F,0x7F,0x28,0x61,0x22,0x42,0x5A,0x1B,0xE8};
static const uint8_t s_snd_Move[]              = {2,0x80,0x94,0x48,0x62,0x40,0x48,0xF9,5};
static const uint8_t s_snd_Engine[]            = {2,0x8C,0x94,0x40,0x61,0x10,0x64,0x18,0xF9,5};
static const uint8_t s_snd_PtsCount1[]         = {2,0x80,0x7F,0x18,0x61,0x39,0xE8};
static const uint8_t s_snd_PtsCount2[]         = {4,0,0x7F,0x28,0x0A,0x61,0x28,0xE8};
static const uint8_t s_snd_RecordPts1[]        = {1,0xB8,0x7F,0x40,0xEF,0x65,0x0C,0x53,0xF0,0x0C,5,0x0C,0x53,0xF0,0x0C,0x0B,0x34,0x24,0xF0,8,0x10,0xEA,0x30,0xB0,0x50,0xEA,0x20,0x9C,0x54,0xE8};
static const uint8_t s_snd_RecordPts2[]        = {2,0xB8,0x7F,0x40,0x65,0x43,0x33,0xF1,0x0C,4,0x43,0x33,0xF1,0x0C,0x0A,0x14,0x4B,0xF1,8,0x0F,0xEA,0x3A,0x30,0x50,9,0x29,0x31,0x51,0x0A,0x2A,0x32,0x52,0x0B,0x2B,0x33,0x53,0x0C,0x2C,0x9C,0xEA,0x20,0x2C,0xE8};
static const uint8_t s_snd_RecordPts3[]        = {3,0,0x7F,8,0xA1,1,1,0xEE,0x15,0x6A,0x0B,0x0B,0x0B,0xEE,0x22,0x6F,0x33,0x65,0x43,0x7E,0xEE,0x33,0x53,0x6A,0xEE,0x15,0x43,0x33,0x53,0x6F,0xEE,0x22,0x13,0x65,0x23,0x7E,0xEE,0x33,0x33,0x6A,0xEE,0x15,0x23,0x13,0x4A,0x9C,0xEE,0xFF,0x32,0xE8};
static const uint8_t s_snd_GameOver1[]         = {1,0x42,0x7F,0x40,0x66,0x1B,0x0B,0x78,0x1B,0x68,0x52,0x42,0x32,0x1A,0x1A,0x1A,0x78,0x1A,0xE8};
static const uint8_t s_snd_GameOver2[]         = {2,0x82,0x7F,0x40,0x66,0x52,0x52,0x78,0x52,0x68,0x32,0x2A,0x12,0x1A,0x1A,0x1A,0x78,0x1A,0xE8};
static const uint8_t s_snd_GameOver3[]         = {3,0x10,0x7F,8,0x66,0x3B,0x33,0x78,0x3B,0x68,0x1B,0x0B,0x52,0x52,0x52,0x52,0x78,0x52,0xE8};
static const uint8_t s_snd_BonusPts[]          = {2,0x82,0x7F,0x40,0x63,0x53,0x1B,0x1C,0x3B,0x3C,0x53,0x6A,0x54,0xE8};

/* 28-slot pointer table (matches Sound_PtrTbl order in ASM) */
static const uint8_t * const s_sound_ptr_tbl[28] = {
    s_snd_Pause,          /* slot  0: Pause music     */
    s_snd_Battle1,        /* slot  1: Battle track 1  */
    s_snd_Battle2,        /* slot  2: Battle track 2  */
    s_snd_Battle3,        /* slot  3: Battle track 3  */
    s_snd_AncillaryLife1, /* slot  4: Snd_Ancillary_Life1  */
    s_snd_AncillaryLife2, /* slot  5: Snd_Ancillary_Life2  */
    s_snd_BonusTaken,     /* slot  6: Snd_BonusTaken       */
    s_snd_PlayerExplode,  /* slot  7: Snd_PlayerExplode    */
    s_snd_Unknown1,       /* slot  8: Snd_Unknown1         */
    s_snd_BonusAppears,   /* slot  9: Snd_BonusAppears     */
    s_snd_EnemyExplode,   /* slot 10: Snd_EnemyExplode     */
    s_snd_HQExplode,      /* slot 11: Snd_HQExplode        */
    s_snd_BrickRicochet,  /* slot 12: Snd_Brick_Ricochet   */
    s_snd_ArmourRicoWall, /* slot 13: Snd_ArmourRicochetWall */
    s_snd_ArmourRicoTank, /* slot 14: Snd_ArmourRicochetTank */
    s_snd_Shoot,          /* slot 15: Snd_Shoot            */
    s_snd_Ice,            /* slot 16: Snd_Ice              */
    s_snd_Move,           /* slot 17: Snd_Move             */
    s_snd_Engine,         /* slot 18: Snd_Engine           */
    s_snd_PtsCount1,      /* slot 19: Snd_PtsCount1        */
    s_snd_PtsCount2,      /* slot 20: Snd_PtsCount2        */
    s_snd_RecordPts1,     /* slot 21: Snd_RecordPts1       */
    s_snd_RecordPts2,     /* slot 22: Snd_RecordPts2       */
    s_snd_RecordPts3,     /* slot 23: Snd_RecordPts3       */
    s_snd_GameOver1,      /* slot 24: Snd_GameOver1        */
    s_snd_GameOver2,      /* slot 25: Snd_GameOver2        */
    s_snd_GameOver3,      /* slot 26: Snd_GameOver3        */
    s_snd_BonusPts,       /* slot 27: Snd_BonusPts         */
};

/* Current ROM data stream pointer (replaces Sound_DataPtr 16-bit NES pointer) */
static const uint8_t *s_snd_data_ptr;

void load_snd_ptr(void) {
    s_snd_data_ptr = s_sound_ptr_tbl[Sound_CurrentSlot];
}

uint8_t sound_load_next_byte(void) {
    uint8_t pos = Sound_DataBlocks[Sound_CurrentSlot * 8u + 5u];
    uint8_t b = s_snd_data_ptr[pos];
    Sound_DataBlocks[Sound_CurrentSlot * 8u + 5u] = (uint8_t)(pos + 1u);
    return b;
}

/* ASM: sound_command_stop_reset */
bool sound_command_stop_reset(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    Sound_PlaybackState[Sound_CurrentSlot] = 0u;
    blk[0] = 0u;
    blk[5]--;   /* undo last byte read (keep ptr at stop marker) */
    return false;
}

/* ASM: sound_command_set_duty_cycle */
bool sound_command_set_duty_cycle(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    uint8_t b = sound_load_next_byte();
    blk[1] = (uint8_t)((blk[1] & 0x3Fu) | b);
    return true;
}

/* ASM: sound_command_set_volume */
bool sound_command_set_volume(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    uint8_t b = sound_load_next_byte();
    blk[1] = (uint8_t)((blk[1] & 0xC0u) | b);
    return true;
}

/* ASM: sound_command_set_volume_alt */
bool sound_command_set_volume_alt(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    uint8_t b = sound_load_next_byte();
    blk[1] = (uint8_t)((blk[1] & 0xC0u) | b);
    return true;
}

/* ASM: sound_command_set_sweep */
bool sound_command_set_sweep(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    blk[2] = sound_load_next_byte();
    return true;
}

/* ASM: sound_command_set_timer_high */
bool sound_command_set_timer_high(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    blk[4] = sound_load_next_byte();
    return true;
}

/* ASM: sound_command_set_duty_volume */
bool sound_command_set_duty_volume(void) {
    uint8_t *blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    blk[1] = sound_load_next_byte();
    return true;
}

/* ASM: sound_command_clear_counters */
bool sound_command_clear_counters(void) {
    Sound_LoopCounter0 = 0u;
    Sound_LoopCounter1 = 0u;
    Sound_LoopCounter2 = 0u;
    return true;
}

/* ASM: Sound_Command_LoopCount0 (7726).
 * В ASM это одна функция с тремя точками входа (LoopCount0/1/2),
 * соединёнными через .BYTE $2C (BIT abs), глотающий следующие LDX #N.
 * В C трюк невозможен — развёрнут в три отдельные C-функции,
 * каждая со своим Sound_LoopCounterX. Внутри сохранены ASM-метки
 * (`equal0`) и вызовы соседних ASM-функций (JumpToOffset, AdvancePointer). */
bool sound_command_loop_count0(void) {
    uint8_t target;
    /* LDX #0; BEQ equal0 (fallthrough — Z от LDX #0) */
equal0:
    /* JSR Sound_LoadNextByte */
    target = sound_load_next_byte();
    /* INC Sound_LoopCounter0,X; CMP Sound_LoopCounter0,X; BNE Sound_Command_JumpToOffset */
    Sound_LoopCounter0++;
    if (target != Sound_LoopCounter0) return sound_command_jump_to_offset();
    /* LDA #0; STA Sound_LoopCounter0,X; BEQ Sound_Command_AdvancePointer (fallthrough) */
    Sound_LoopCounter0 = 0u;
    return sound_command_advance_pointer();
}

/* ASM: Sound_Command_LoopCount1 (7730, точка входа в общий equal0 c X=1) */
bool sound_command_loop_count1(void) {
    uint8_t target;
equal0:
    target = sound_load_next_byte();
    Sound_LoopCounter1++;
    if (target != Sound_LoopCounter1) return sound_command_jump_to_offset();
    Sound_LoopCounter1 = 0u;
    return sound_command_advance_pointer();
}

/* ASM: Sound_Command_LoopCount2 (7734, точка входа в общий equal0 c X=2) */
bool sound_command_loop_count2(void) {
    uint8_t target;
equal0:
    target = sound_load_next_byte();
    Sound_LoopCounter2++;
    if (target != Sound_LoopCounter2) return sound_command_jump_to_offset();
    Sound_LoopCounter2 = 0u;
    return sound_command_advance_pointer();
}

/* ASM: Sound_Command_AdvancePointer (7746) — fallthrough из LoopCountN.
 * LDY #5; LDA (ptr),Y; CLC; ADC #1; STA (ptr),Y; JMP readNextCommandByte */
bool sound_command_advance_pointer(void) {
    Sound_DataBlocks[Sound_CurrentSlot * 8u + 5u]++;
    return true;
}

/* ASM: Sound_Command_JumpToOffset (7754).
 * JSR Sound_LoadNextByte; LDY #5; STA (ptr),Y; JMP readNextCommandByte */
bool sound_command_jump_to_offset(void) {
    Sound_DataBlocks[Sound_CurrentSlot * 8u + 5u] = sound_load_next_byte();
    return true;
}

static bool (* const sound_command_jump_table[18])(void) = {
    sound_command_stop_reset,
    sound_command_set_duty_cycle,
    sound_command_set_volume,
    sound_command_set_volume_alt,
    sound_command_set_sweep,
    sound_command_set_timer_high,
    sound_command_set_duty_volume,
    sound_command_clear_counters,
    sound_command_loop_count0,
    sound_command_loop_count1,
    sound_command_loop_count2,
    sound_command_advance_pointer,
    sound_command_advance_pointer,
    sound_command_advance_pointer,
    sound_command_advance_pointer,
    sound_command_advance_pointer,
    sound_command_advance_pointer,
    sound_command_jump_to_offset,
};

bool sound_dispatch_command(uint8_t cmd_idx) {
    if (cmd_idx >= 18u) return true;
    return sound_command_jump_table[cmd_idx]();
}

/* ASM: Sound_Stop (7315) */
void sound_stop(void) {
    uint8_t x;

    /* LDA #%00001111; STA SND_MASTERCTRL_REG */
    apu_write_status(0x0Fu);
    /* LDA #%11000000; STA JOYPAD_PORT2 */
    JOYPAD_PORT2 = 0xC0u;
    /* (Sound_CurrentData_Ptr init и его инкремент не нужны: C использует индексацию x*8) */
    x = 0u;

at_:
    /* TYA (=0); STA (Sound_CurrentData_Ptr),Y=0; STA Sound_PlaybackState,X */
    Sound_DataBlocks[x * 8u] = 0u;
    Sound_PlaybackState[x]   = 0u;
    /* CLC/ADC #8/STA/BCC @__/INC — поддержка указателя; в C опускается */

at__:
    /* INX; CPX #28; BNE @_ */
    x++;
    if (x != 28u) goto at_;
}

/* ASM: Play_Sound (7349) — все метки сохранены 1-в-1 с ASM */
void play_sound(void) {
    uint8_t *blk = NULL;
    uint8_t x;
    uint8_t ch;
    uint8_t chan;
    uint8_t reg_base;
    uint8_t y;
    uint8_t a;
    uint8_t cmd = 0u;
    uint8_t idx;
    uint16_t val;
    uint8_t shift;
    uint8_t carry;

    /* LDA Pause_Flag; BNE skip */
    if (Pause_Flag != 0u) goto skip;
    /* LDA #$1C; STA Sound_NumberOfSlots; BPL clearChannels */
    Sound_NumberOfSlots = 0x1Cu;
    goto clearChannels;

skip:
    Sound_NumberOfSlots = 1u;
    /* fallthrough */

clearChannels:
    /* LDA #0; LDX #3 */
    x = 3u;
loopChannels:
    /* STA SoundChannels,X; DEX; BPL loopChannels */
    SoundChannels[x] = 0u;
    if (x != 0u) { x--; goto loopChannels; }
    /* ASM DEX делает X=0xFF (signed -1), BPL не берётся → exit. C: x уже 0 после последней записи. */

    Sound_CurrentSlot = 0u;
    /* Sound_CurrentData_Ptr = Sound_DataBlocks (C: индексация slot*8) */

playLoop:
    /* LDX Sound_CurrentSlot; LDA Sound_PlaybackState,X; BEQ endProcessing */
    x = Sound_CurrentSlot;
    if (Sound_PlaybackState[x] == 0u) goto endProcessing;
    blk = &Sound_DataBlocks[x * 8u];
    /* LDY #0; LDA (ptr),Y → blk[0]; BEQ endProcessing */
    ch = blk[0];
    if (ch == 0u) goto endProcessing;
    /* CMP #5; BCC gt5 */
    if (ch < 5u) goto gt5;
    /* SEC; SBC #5; TAX; LDA #1; STA SoundChannels,X; JMP endProcessing */
    SoundChannels[(uint8_t)(ch - 5u)] = 1u;
    goto endProcessing;

gt5:
    /* TAX; DEX (X = chan = blk[0]-1); LDA SoundChannels,X; BNE endProcessing */
    chan = (uint8_t)(ch - 1u);
    if (SoundChannels[chan] != 0u) goto endProcessing;
    /* LDA #1; STA SoundChannels,X; TXA; TAY (unused); CLC; ADC #5; LDY #0; STA blk[0] */
    SoundChannels[chan] = 1u;
    blk[0] = (uint8_t)(chan + 5u);
    /* TXA; ASL; ASL; TAX (X = chan*4) */
    reg_base = (uint8_t)(chan << 2u);
    /* LDA #4; STA Sound_Temp; (Y=0 unchanged from LDY #0 above) */
    Sound_Temp = 4u;
    y = 0u;

writeToRegistersLoop:
    /* INY; LDA (ptr),Y → blk[Y]; STA SND_SQUARE1_REG,X; INX; DEC Sound_Temp; BNE loop */
    y++;
    apu_write((uint8_t)(reg_base + (uint8_t)(y - 1u)), blk[y]);
    Sound_Temp--;
    if (Sound_Temp != 0u) goto writeToRegistersLoop;

endProcessing:
    /* CLC/ADC #8/STA/BCC skip_2/INC — указатель; в C опущено. */
skip_2:
    /* INC Sound_CurrentSlot; CMP Sound_NumberOfSlots; BCC playLoop */
    Sound_CurrentSlot++;
    if (Sound_CurrentSlot < Sound_NumberOfSlots) goto playLoop;

    /* LDX #0 — переход к фазе тишины */
    x = 0u;

SilencingLoop:
    /* STX Sound_Temp; LDA SoundChannels,X; BNE skipSilencing */
    Sound_Temp = x;
    if (SoundChannels[x] != 0u) goto skipSilencing;
    /* TXA; ASL; ASL; TAX (X=x*4); ASL (A=x*8); AND #$10; EOR #$10; STA SND_SQUARE1_REG,X */
    a = (uint8_t)(((uint8_t)(x << 3u) & 0x10u) ^ 0x10u);
    apu_write((uint8_t)(x << 2u), a);

skipSilencing:
    /* LDX Sound_Temp; INX; CPX #4; BCC SilencingLoop */
    x = Sound_Temp;
    x++;
    if (x < 4u) goto SilencingLoop;

    /* LDY #0; STY Sound_CurrentSlot; ptr reset (C: индексация) */
    Sound_CurrentSlot = 0u;

mainProcessingLoop:
    /* LDX Sound_CurrentSlot; LDA Sound_PlaybackState,X; BEQ advanceToNextSlot */
    x = Sound_CurrentSlot;
    if (Sound_PlaybackState[x] == 0u) goto advanceToNextSlot;
    /* CMP #1; BNE handleDurationCountdown */
    if (Sound_PlaybackState[x] != 1u) goto handleDurationCountdown;
    /* INC Sound_PlaybackState,X (1→2); JMP initializeNewSound */
    Sound_PlaybackState[x]++;
    goto initializeNewSound;

advanceToNextSlot:
    /* CLC/ADC #8/... указатель в C не нужен */
nextSlot:
    /* INC Sound_CurrentSlot; CMP Sound_NumberOfSlots; BCC mainProcessingLoop; RTS */
    Sound_CurrentSlot++;
    if (Sound_CurrentSlot < Sound_NumberOfSlots) goto mainProcessingLoop;
    return;

handleDurationCountdown:
    /* LDY #7; LDA (ptr),Y; SEC; SBC #1; STA (ptr),Y; BEQ loadSoundPtr; BNE advanceToNextSlot */
    blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    blk[7]--;
    if (blk[7] == 0u) goto loadSoundPtr;
    goto advanceToNextSlot;

initializeNewSound:
    /* LDA #0; LDY #5; STA (ptr),Y → blk[5]=0 */
    blk = &Sound_DataBlocks[Sound_CurrentSlot * 8u];
    blk[5] = 0u;
    /* JSR Load_Snd_Ptr; четыре чтения в blk[0..2,4] */
    load_snd_ptr();
    blk[0] = sound_load_next_byte();
    blk[1] = sound_load_next_byte();
    blk[2] = sound_load_next_byte();
    blk[4] = sound_load_next_byte();
    /* LDY #0; LDA (ptr),Y; CMP #4; BNE readNextCommandByte */
    if (blk[0] != 4u) goto readNextCommandByte;
    /* JSR Sound_LoadNextByte; LDY #3; STA blk[3]; BPL readNextCommandByte (всегда) */
    blk[3] = sound_load_next_byte();
    goto readNextCommandByte;

loadSoundPtr:
    /* JSR Load_Snd_Ptr (fallthrough → readNextCommandByte) */
    load_snd_ptr();
    /* fallthrough */

readNextCommandByte:
    /* JSR Sound_LoadNextByte; CMP #$E8; BCS specialCommand */
    cmd = sound_load_next_byte();
    if (cmd >= 0xE8u) goto specialCommand;
    /* CMP #$60; BEQ handleNote; BCC processFrequencyLookup */
    if (cmd == 0x60u) goto handleNote;
    if (cmd < 0x60u) goto processFrequencyLookup;
    /* SBC #$60 (carry от CMP); LDY #6; STA blk[6]; JMP readNextCommandByte */
    blk[6] = (uint8_t)(cmd - 0x60u);
    goto readNextCommandByte;

processFrequencyLookup:
    /* PHA; AND #$F8; LSR; LSR; TAX → байтовый offset; C: индекс word'а = (cmd & 0xF8) >> 3 */
    idx = (uint8_t)((cmd & 0xF8u) >> 3u);
    val = Frequency_LUT[idx];
    /* LDA Frequency_LUT,X → Sound_Temp (low byte from ROM); LDA +1,X → Sound_Temp2 (high) */
    Sound_Temp  = (uint8_t)(val & 0xFFu);
    Sound_Temp2 = (uint8_t)(val >> 8u);
    /* PLA; AND #7; BEQ skip_3; TAX */
    shift = (uint8_t)(cmd & 7u);
    if (shift == 0u) goto skip_3;

loopSoundTemp:
    /* LSR Sound_Temp; ROR Sound_Temp2; DEX; BNE loopSoundTemp */
    carry = (uint8_t)(Sound_Temp & 1u);
    Sound_Temp  >>= 1u;
    Sound_Temp2  = (uint8_t)((Sound_Temp2 >> 1u) | (uint8_t)(carry << 7u));
    shift--;
    if (shift != 0u) goto loopSoundTemp;

skip_3:
    /* LDY #4; LDA (ptr),Y; AND #$F8; ORA Sound_Temp; STA (ptr),Y */
    blk[4] = (uint8_t)((blk[4] & 0xF8u) | Sound_Temp);
    /* LDA Sound_Temp2; DEY (Y=3); STA (ptr),Y */
    blk[3] = Sound_Temp2;
    /* LDY #0; LDA (ptr),Y; CMP #5; BCC handleNote */
    if (blk[0] < 5u) goto handleNote;
    /* SEC; SBC #4; STA (ptr),Y */
    blk[0] = (uint8_t)(blk[0] - 4u);
    /* fallthrough */

handleNote:
    /* LDY #6; LDA (ptr),Y; INY; STA (ptr),Y → blk[7] = blk[6]; JMP advanceToNextSlot */
    blk[7] = blk[6];
    goto advanceToNextSlot;

specialCommand:
    /* SBC #$E8; JSR Sound_DispatchCommand */
    if (!sound_dispatch_command((uint8_t)(cmd - 0xE8u))) goto advanceToNextSlot;
    goto readNextCommandByte;
}

