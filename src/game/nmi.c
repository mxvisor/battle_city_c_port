#include "nmi.h"
#include "zeropage.h"
#include "ppu_registers.h"
#include "bss.h"
#include "draw.h"
#include "sound_engine.h"

atomic_int game_in_nmi_wait = 0;
jmp_buf game_exit_buf;

void nmi(void) {
      // PHA, TXA, PHA, TYA, PHA, PHP handled by C prologue/epilogue

    // LDA #0; STA PPU_SPR_ADDR
    // Initialization for writing to zero address SPR OAM
    // In this port, we don't have a direct variable for this register yet.

    // LDA #2; STA SPR_DMA
    // Sprite buffer will be at address $200 (SprBuffer in globals)
    
    // LDA PPU_STATUS; Reset VBlank Occurance
    ppu_status_read();
    
    // JSR Update_Screen; Transfer from Screen_Buffer to PPU memory
    update_screen();

    // LDA BkgPal_Number
    if (!(BkgPal_Number & 0x80)) {
        // JSR Load_Bkg_Pal
        load_bkg_pal();
    }

    // LDA PPU_REG1_Stts; ORA #$B0; STA PPU_CTRL_REG1
    // bit 7=1 (NMI), bit 5=1 (Sprite 8x16), bit 4=1 (BG Table $1000)
    PPU_CTRL_REG1 = PPU_REG1_Stts | 0xB0;

    // LDA #0; STA PPU_SCROLL_REG  (X scroll = 0)
    // LDA Scroll_Byte; STA PPU_SCROLL_REG  (Y scroll)
    // Render reads PPU_SCROLL_REG as Y scroll — must always sync.
    PPU_SCROLL_REG = Scroll_Byte;

    // LDA #$1E; STA PPU_CTRL_REG2; Enable background and sprites
    PPU_CTRL_REG2 = 0x1E;

    // JSR Read_Joypads
    read_joypads();

    // JSR Spr_Invisible; Outputs sprite Y-coordinates to $F0
    spr_invisible();

    // JSR Play_Sound; Similar to Play in NSF format
    play_sound();

    // INC Frame_Counter
    Frame_Counter++;

    // LDA Frame_Counter; AND #63; BNE End_Interrupt
    if ((Frame_Counter & 63) == 0) {
        // INC Seconds_Counter; 64 frames in one second?
        Seconds_Counter++;
    }

    // End_Interrupt:
    // PLP, PLA, TAY, PLA, TAX, PLA, RTI handled by C prologue/epilogue
}

/* ASM: Read_Joypads (3571).
 * Hardware-strobe `STX/STY JOYPAD_PORT1` и 8-битный цикл побитового чтения
 * `LDA JOYPAD_PORT1,X; AND #3; CMP #1; ROR Temp` (@_) физически невозможны
 * в C — заменены на вызов SDL-опроса, который возвращает все 8 бит сразу.
 * Внешний цикл (@__) по X=1→0 (P2 затем P1) и формула
 * `Differ = ~prev_buttons & new_buttons` сохранены 1-в-1 с ASM. */
void read_joypads(void) {
    uint8_t temp;
    uint8_t x;

    /* LDX #1; STX JOYPAD_PORT1 / LDY #0; STY JOYPAD_PORT1 — strobe N/A в C */
    x = 1u;

at__:
    /* @_: 8-итераций hardware-чтения → заменено на одну SDL-функцию */
    temp = (x == 0u) ? plat_poll_buttons_p1()
                     : plat_poll_buttons_p2();

    /* LDA Joypad1_Buttons,X; EOR #$FF; AND Temp; STA Joypad1_Differ,X */
    /* LDA Temp; STA Joypad1_Buttons,X */
    if (x == 0u) {
        Joypad1_Differ  = (uint8_t)((uint8_t)~Joypad1_Buttons & temp);
        Joypad1_Buttons = temp;
    } else {
        Joypad2_Differ  = (uint8_t)((uint8_t)~Joypad2_Buttons & temp);
        Joypad2_Buttons = temp;
    }

    /* DEX; BPL @__ — X идёт 1, 0, далее BPL не берётся */
    if (x == 0u) return;
    x--;
    goto at__;
}

void nmi_wait(void) {
    atomic_store_explicit(&game_in_nmi_wait, 1, memory_order_release);
    plat_sem_wait(plat_get_wake_sem());
    atomic_store_explicit(&game_in_nmi_wait, 0, memory_order_release);
    
    if (!plat_running) {
        longjmp(game_exit_buf, 1);
    }
}

void vblank_wait(void) {
    while (plat_running) {
        if (ppu_status_read() & 0x80) {
            break;
        }
        plat_sem_wait(plat_get_vblank_sem());
    }

    if (!plat_running) {
        longjmp(game_exit_buf, 1);
    }
}

void update_screen(void) {
    if (ScrBuffer_Pos == 0) return;

    Screen_Buffer[ScrBuffer_Pos] = 0;
    uint8_t x = 0;
    uint16_t addr = 0;
    uint8_t val;

at_:
    if (x == ScrBuffer_Pos) goto Update_Screen_End;
    addr = ((uint16_t)Screen_Buffer[x++] << 8) | Screen_Buffer[x++];

at__:
    val = Screen_Buffer[x++];
    if (val == 0xFF) goto at_;

at___:
    ppu_data_write(addr++, val);
    goto at__;

Update_Screen_End:
    ScrBuffer_Pos = 0;
}

static const uint8_t SpritePalette[] = {
    0x0F, 0x18, 0x27, 0x38,
    0x0F, 0x0A, 0x1B, 0x3B,
    0x0F, 0x0C, 0x10, 0x20,
    0x0F, 0x04, 0x16, 0x20,
};

static const uint8_t PaletteFrame2[] = {
    0x0F, 0x17, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x12,
    0x0F, 0x29, 0x09, 0x0B, 0x0F, 0x00, 0x10, 0x20,
};

static const uint8_t LevelPalette[] = {
    0x0F, 0x17, 0x06, 0x00, 0x0F, 0x3C, 0x12, 0x12,
    0x0F, 0x29, 0x09, 0x0B, 0x0F, 0x00, 0x10, 0x20,
};

static const uint8_t PaletteFrame1[] = {
    0x0F, 0x17, 0x06, 0x00, 0x0F, 0x12, 0x3C, 0x12,
    0x0F, 0x29, 0x09, 0x0B, 0x0F, 0x00, 0x10, 0x20,
};

static const uint8_t TitleScrPalette[] = {
    0x0F, 0x16, 0x16, 0x30, 0x0F, 0x3C, 0x10, 0x16,
    0x0F, 0x29, 0x09, 0x27, 0x0F, 0x00, 0x10, 0x20,
};

static const uint8_t LevelSelPalette[] = {
    0x0F, 0x17, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x00,
    0x0F, 0x29, 0x09, 0x00, 0x0F, 0x00, 0x10, 0x00,
};

/* ASM line 3441 — anonymous palette between LevelSelPalette and PaletteMisc1. */
static const uint8_t PaletteFlash1[] = {
    0x0F, 0x0F, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x00,
    0x0F, 0x29, 0x09, 0x00, 0x0F, 0x00, 0x10, 0x00,
};

static const uint8_t PaletteMisc1[] = {
    0x0F, 0x12, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x00,
    0x0F, 0x29, 0x09, 0x00, 0x0F, 0x00, 0x10, 0x00,
};

/* ASM line 3444 — anonymous palette between PaletteMisc1 and PaletteMisc2. */
static const uint8_t PaletteFlash2[] = {
    0x0F, 0x00, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x00,
    0x0F, 0x29, 0x09, 0x00, 0x0F, 0x00, 0x10, 0x00,
};

static const uint8_t PaletteMisc2[] = {
    0x0F, 0x30, 0x06, 0x00, 0x0F, 0x3C, 0x10, 0x00,
    0x0F, 0x29, 0x09, 0x00, 0x0F, 0x00, 0x10, 0x00,
};

static const uint8_t *const BkgPaletteTable[] = {
    PaletteFrame2,      /* 0 */
    LevelPalette,       /* 1 */
    PaletteFrame1,      /* 2 */
    TitleScrPalette,    /* 3 */
    LevelSelPalette,    /* 4 */
    PaletteFlash1,      /* 5 — hi-score flash */
    PaletteMisc1,       /* 6 */
    PaletteFlash2,      /* 7 — hi-score flash */
    PaletteMisc2,       /* 8 */
};

// Spr_Pal_Load
void spr_pal_load(void) {
    uint8_t x = 0;              // LDX #0
    uint8_t y = 0x10;           // LDY #$10
    uint16_t ppu_addr = 0x3F10; // LDA #$3F / STA PPU_ADDRESS; STY PPU_ADDRESS (=$10)
at_:
    ppu_data_write(ppu_addr++, SpritePalette[x]); // LDA SpritePalette,X / STA PPU_DATA
    x++;                        // INX
    y--;                        // DEY
    if (y != 0) goto at_;       // BNE @_
}

// Load_Bkg_Pal
void load_bkg_pal(void) {
    uint8_t a = BkgPal_Number;  // (caller: LDA BkgPal_Number before JSR Load_Bkg_Pal)
    a <<= 4;                    // ASL A / ASL A / ASL A / ASL A  ; A*16
    uint8_t x = a;              // TAX
    uint8_t y = 0x10;           // LDY #$10
    uint16_t ppu_addr = 0x3F00; // LDA #$3F / STA PPU_ADDRESS; LDA #0 / STA PPU_ADDRESS
at_:
    ppu_data_write(ppu_addr++, BkgPaletteTable[x >> 4][x & 0x0F]); // LDA PaletteFrame2,X / STA PPU_DATA
    x++;                        // INX
    y--;                        // DEY
    if (y != 0) goto at_;       // BNE @_
    BkgPal_Number = 0xFF;       // LDA #$FF / STA BkgPal_Number
    // LDA #$3F / STA PPU_ADDRESS x3 — reset PPU address (no-op in C)
}

