#include "draw.h"
#include "nes/chr_load.h"
#include "zeropage.h"
#include "bss.h"
#include <string.h>
#include "coords.h"
#include "ppu_registers.h"
#include "nmi.h"
#include "strings.h"

static const uint8_t TSABlock_PalNumber[16] = {
    0, 0, 0, 0, 0, 3, 3, 3,
    3, 3, 1, 2, 3, 0, 0, 0
};

static const uint8_t TSA_data_start[64] = {
    0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x00, 0x0F, 0x0F,
    0x0F, 0x00, 0x0F, 0x00,
    0x0F, 0x0F, 0x00, 0x00,
    0x0F, 0x0F, 0x0F, 0x0F,
    0x20, 0x10, 0x20, 0x10,
    0x20, 0x20, 0x10, 0x10,
    0x10, 0x20, 0x10, 0x20,
    0x10, 0x10, 0x20, 0x20,
    0x10, 0x10, 0x10, 0x10,
    0x12, 0x12, 0x12, 0x12,
    0x22, 0x22, 0x22, 0x22,
    0x21, 0x21, 0x21, 0x21,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

void draw_tile(void) {
    uint16_t addr = LowPtr_Byte | (HighPtr_Byte << 8);

    /* STA (LowPtr_Byte),Y
       Original ASM stores A into the nametable address pointed by LowPtr_Byte/HighPtr_Byte.
       In the emulator this is represented by NT_Buffer. */
    NT_Buffer[addr & 0x3FF] = Spr_TileIndex;

    /* STX Spr_X - preserve incoming X register for caller state; not needed in C */
    
    /* LDX ScrBuffer_Pos */
    uint8_t write_pos = ScrBuffer_Pos;

    /* LDA HighPtr_Byte / CLC / ADC #$1C */
    Screen_Buffer[write_pos++] = HighPtr_Byte + PPU_Addr_Ptr;
    Screen_Buffer[write_pos++] = LowPtr_Byte;

    /* LDA (LowPtr_Byte),Y — read back the same nametable byte and append it to screen buffer */
    Screen_Buffer[write_pos++] = NT_Buffer[addr & 0x3FF];
    Screen_Buffer[write_pos++] = 0xFF;

    /* STX ScrBuffer_Pos */
    ScrBuffer_Pos = write_pos;

    /* LDX Spr_X - restore saved X register for caller state; not needed in C */
}

void draw_tsa_block(uint8_t block_num) {
    TSA_BlockNumber = block_num;
    Spr_X = Block_X >> 3;
    Spr_Y = Block_Y >> 3;
    
    TSA_Pal = TSABlock_PalNumber[block_num];
    attrib_to_scr_buffer();
    
    uint8_t y = Spr_Y & 0xFE;
    uint8_t x = Spr_X & 0xFE;
    coords_to_ram_pos(x, y);
    
    uint16_t tsa_idx = (uint16_t)block_num << 2;
    
    Spr_TileIndex = TSA_data_start[tsa_idx++];
    draw_tile();
    
    inc_ptr_on_a(1);
    Spr_TileIndex = TSA_data_start[tsa_idx++];
    draw_tile();
    
    inc_ptr_on_a(0x1F);
    Spr_TileIndex = TSA_data_start[tsa_idx++];
    draw_tile();
    
    inc_ptr_on_a(1);
    Spr_TileIndex = TSA_data_start[tsa_idx++];
    draw_tile();
}
void draw_ptr_tile(void) {
    // This is never used
    // LDA	Temp		; This is never used
    // ORA	(LowPtr_Byte),Y
    // JSR	Draw_Tile
    (void)Temp;
    draw_tile();
}
void inc_ptr_on_a(uint8_t a) {
    uint16_t ptr = LowPtr_Byte | (HighPtr_Byte << 8);
    ptr += a;
    LowPtr_Byte = ptr & 0xFF;
    HighPtr_Byte = ptr >> 8;
}

void draw_char(uint8_t char_index) {
    BrickChar_X = Block_X;
    BrickChar_Y = Block_Y + 0x20;

    LowPtr_Byte = 0;
    HighPtr_Byte = get_bg_bank_offset() >> 8;

    uint8_t x = char_index;
Add_10:
    x = (uint8_t)(x - 1u);
    if ((int8_t)x < 0) goto at_;
    inc_ptr_on_a(0x10u);
    goto Add_10;

at_:
    {
        uint8_t *chr = gets_chr_ptr();
        uint16_t chr_addr = LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
        uint8_t pattern[8];
        for (uint8_t i = 0; i < 8u; i++) pattern[i] = chr[chr_addr + i];
        /* ASM: 8 PHA pushes via PPU_DATA read loop — эмулируем как стек через pattern[]. */
        Counter = 8u;

    NextByte:
        Counter--;
        CHR_Byte = pattern[Counter];
        Mask_CHR_Byte = 0x80u;

    Next_Bit:
        get_spr_coord_in_tiles(BrickChar_X, BrickChar_Y);
        temp_coord_shl();
        if ((CHR_Byte & Mask_CHR_Byte) == 0u) goto Empty_Pixel;
        nt_buffer_process_or(0);
        goto pixelProcessed;

    Empty_Pixel:
        nt_buffer_process_xor(0);

    pixelProcessed:
        BrickChar_X = (uint8_t)(BrickChar_X + 4u);
        Mask_CHR_Byte = (uint8_t)(Mask_CHR_Byte >> 1u);
        if (Mask_CHR_Byte != 0u) goto Next_Bit;
        BrickChar_X = (uint8_t)(BrickChar_X - 0x20u);
        BrickChar_Y = (uint8_t)(BrickChar_Y - 4u);
        if (Counter != 0u) goto NextByte;
    }
}

void save_spr_to_spr_buffer(uint8_t x, uint8_t y) {
    if (SprBuffer_Position >= 252) return;

    /* TXA; STA Spr_X — Spr_X = raw input X */
    Spr_X = x;
    /* CLC; ADC #3; TAX — CPU X_reg ← x+3 (для GetCoord_InTiles) */
    /* TYA; SEC; SBC #8; STA Spr_Y — Spr_Y = y - 8 (display offset) */
    Spr_Y = (uint8_t)(y - 8u);
    /* JSR GetCoord_InTiles (direct, ASM @4367) — X_reg=(x+3), Y_reg=y (исходный) */
    get_coord_in_tiles_xy((uint8_t)(x + 3u), y);

    uint8_t tile = NT_Buffer[(LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8)) & 0x3FFu];
    if (tile == 0x22u) {
        TSA_Pal = (uint8_t)(TSA_Pal | Spr_Attrib); /* LDA TSA_Pal; ORA Spr_Attrib; STA TSA_Pal */
        goto Skip_Attrib;
    }

Skip_Attrib:
    SprBuffer[SprBuffer_Position + 0u] = Spr_Y;
    SprBuffer[SprBuffer_Position + 1u] = Spr_TileIndex;
    SprBuffer[SprBuffer_Position + 2u] = TSA_Pal;
    SprBuffer[SprBuffer_Position + 3u] = Spr_X;
    SprBuffer_Position = (uint8_t)(SprBuffer_Position + Gap);
}

void indexed_save_spr(uint8_t direction, uint8_t x, uint8_t y) {
    Spr_TileIndex += (direction & 0x03) * 2;
    save_spr_to_spr_buffer(x - 5, y);
}

void spr_tile_index_add(uint8_t direction) {
    Spr_TileIndex += (direction & 0x03) * 8;
}

void draw_whole_spr(void) {
    uint8_t tx = Temp_X;
    uint8_t ty = Temp_Y;
    
    save_spr_to_spr_buffer(tx - 8, ty);
    Spr_TileIndex += 2; // Interleaved format: skip bottom-left tile to get top-right
    save_spr_to_spr_buffer(tx, ty);
}

/* ASM: Draw_BrickStr — (LowStrPtr_Byte) заменён на параметр str (как в levels.c). */
void draw_brick_str(const uint8_t *str) {
    if (!str) return;
    uint8_t y = 0u;
    String_Position = y;

New_Char:
    {
        uint8_t a = str[y];
        if (a == 0xFFu) goto EOS;
        y++;
        String_Position = y;
        a = (uint8_t)(a + Char_Index_Base);
        draw_char(a);
        Block_X = (uint8_t)(Block_X + 0x20u);
        y = String_Position;
        goto New_Char;
    }

EOS:
    return;
}

void nt_buffer_process_xor(uint8_t value) {
    /* ASM: NT_Buffer_Process_XOR — uses (LowPtr_Byte),Y: clear Temp bits from tile.
     * C port: LowPtr/HighPtr already set by get_spr_coord_in_tiles. */
    uint16_t offset = LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    uint8_t tile = NT_Buffer[offset % 1024];
    if ((tile & 0xF0) == 0) {
        NT_Buffer[offset % 1024] = tile & (uint8_t)~Temp;
    }
}

void nt_buffer_process_or(uint8_t value) {
    /* ASM: NT_Buffer_Process_OR — uses (LowPtr_Byte),Y: set Temp bits in tile.
     * C port: LowPtr/HighPtr already set by get_spr_coord_in_tiles. */
    uint16_t offset = LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    uint8_t tile = NT_Buffer[offset % 1024];
    if ((tile & 0xF0) == 0) {
        NT_Buffer[offset % 1024] = tile | Temp;
    }
}
void fill_nt_buffer(uint8_t value) {
    memset(NT_Buffer, value, 1024);
}

void fill_nt_attrib_buffer(uint8_t value) {
    memset(&NT_Buffer[0x3C0], value, 64);
}
void null_upper_nt(void) {
    screen_off();
    BkgPal_Number = 3;
    PPU_Addr_Ptr = 0x20; // Correct NT0 address base
    null_nt_buffer();
    store_nt_buffer_in_vram();
    // Ensure palette is updated before we start complex drawing or scrolling
    nmi_wait(); 
}
void null_nt_buffer(void) {
    memset(NT_Buffer, 0, 1024);
}
void spr_invisible(void) {
    /* ASM: negates Gap in a register (not the zeropage var!), then walks down from
       SprBuffer_Position by that step filling Y=$F0 for every sprite slot until X==4.
       Slot 0 (bytes 0-3) is NOT touched. */
    uint8_t step = (uint8_t)(-(int8_t)Gap); /* local negation; Gap itself stays 4 */
    uint8_t x = (uint8_t)SprBuffer_Position;
    do {
        x = (uint8_t)(x + step);  /* wraps: 0+0xFC=252, 252+0xFC=248, ... */
        SprBuffer[x] = 0xF0;
    } while (x != 4);
    SprBuffer_Position = 4;
}
void clear_nt(void) {
    screen_off();
    null_nt_buffer();
    store_nt_buffer_in_vram();
    set_ppu();
}
void fill_scr_single_row(uint8_t row) {
    uint16_t addr = coord_to_ppu_address(0, row);
    HighPtr_Byte = addr >> 8;
    LowPtr_Byte = addr & 0xFF;
    
    uint8_t x = ScrBuffer_Pos;
    if (x > 128 - 35) return; // Prevention

    Screen_Buffer[x++] = HighPtr_Byte + PPU_Addr_Ptr;
    Screen_Buffer[x++] = LowPtr_Byte;
    
    for (int y = 0; y < 0x20; y++) {
        if (Iterative_Byte != 0) {
            Screen_Buffer[x++] = Iterative_Byte;
        } else {
            uint16_t nt_offset = (uint16_t)row * 32 + y;
            Screen_Buffer[x++] = NT_Buffer[nt_offset % 1024];
        }
    }
    
    Screen_Buffer[x++] = 0xFF;
    ScrBuffer_Pos = x;
}

void copy_attrib_to_scrn_buff(void) {
    uint8_t x = ScrBuffer_Pos;
    uint16_t addr = 0x23C0;
    Screen_Buffer[x++] = (uint8_t)(addr >> 8);  /* ASM: no ADC #$1C here */
    Screen_Buffer[x++] = (uint8_t)addr;
    for (int i = 0; i < 64; i++) {
        Screen_Buffer[x++] = NT_Buffer[0x3C0 + i];
    }
    Screen_Buffer[x++] = 0xFF;
    ScrBuffer_Pos = x;
}

uint8_t or_pal(uint8_t a) {
    a <<= 1;
    a <<= 1;
    return (uint8_t)(a | TSA_Pal);
}

uint8_t tsa_pal_ops(uint8_t x, uint8_t y) {
    uint8_t a = TSA_Pal;
    a = or_pal(a);
    a = or_pal(a);
    a = or_pal(a);
    CHR_Byte = a;

    uint8_t tmp_pal;
    if ((y & 2) != 0) {
        goto at_;
    }
    if ((x & 2) == 0) {
        goto at__;
    }
    tmp_pal = 0xF3;
    goto End_TSA_Pal_Ops;

at__:
    tmp_pal = 0xFC;
    goto End_TSA_Pal_Ops;

at_:
    if ((x & 2) == 0) {
        goto at___;
    }
    tmp_pal = 0x3F;
    goto End_TSA_Pal_Ops;

at___:
    tmp_pal = 0xCF;

End_TSA_Pal_Ops:
    Tmp_Pal = tmp_pal;

    uint8_t temp = (uint8_t)((uint8_t)(y << 1) & 0xF8);
    uint8_t attr_index = (uint8_t)((x >> 2) + temp);

    uint8_t inv_pal = (uint8_t)(tmp_pal ^ 0xFF);
    CHR_Byte &= inv_pal;

    uint8_t attr = NT_Buffer[0x3C0 + attr_index];
    attr = (uint8_t)((attr & tmp_pal) | CHR_Byte);
    NT_Buffer[0x3C0 + attr_index] = attr;

    return attr_index;
}

void fill_nt_with_grey(void) {
    Iterative_Byte = 0x11;
    Block_Y = 0;
    while (Block_Y < 0x10) {
        nmi_wait();
        fill_scr_single_row(Block_Y);
        fill_scr_single_row(0x1D - Block_Y);
        Block_Y++;
    }
}
void fill_nt_with_black(void) {
    Iterative_Byte = 0;
    Block_Y = 0xF;
    while (Block_Y != 0xFF) {
        nmi_wait();
        fill_scr_single_row(Block_Y);
        fill_scr_single_row(0x1D - Block_Y);
        Block_Y--;
    }
}
void draw_black_row(void) {
    uint16_t addr = LowPtr_Byte | (HighPtr_Byte << 8);
    for (int i = 0; i < Counter2; i++) {
        NT_Buffer[(addr + i) & 0x3FF] = 0;
    }
}

void draw_gray_frame(void) {
    fill_nt_buffer(0x11);
    fill_nt_attrib_buffer(0);
    
    uint16_t addr = coord_to_ppu_address(Block_X, Block_Y);
    LowPtr_Byte = addr & 0xFF;
    HighPtr_Byte = addr >> 8;
    
    while (Counter > 0) {
        draw_black_row();
        inc_ptr_on_a(0x20);
        Counter--;
    }
}

void make_gray_frame(void) {
    Block_X = 2;
    Block_Y = 2;
    Counter = 26;
    Counter2 = 26;
    draw_gray_frame();
}

void attrib_to_scr_buffer(void) {
    uint8_t y = Spr_Y & 0xFE;
    uint8_t x = Spr_X & 0xFE;
    uint8_t attr_index = tsa_pal_ops(x, y);

    uint8_t pos = ScrBuffer_Pos;
    Screen_Buffer[pos++] = 0x23; /* hi byte: NT page + $23 offset (attrib row of NT) */
    Screen_Buffer[pos++] = (uint8_t)(0xC0u + attr_index); /* lo byte */
    Screen_Buffer[pos++] = NT_Buffer[0x3C0 + attr_index];
    Screen_Buffer[pos++] = 0xFFu;
    ScrBuffer_Pos = pos;
}

// Set_PPU (ASM): JSR VBlank_Wait; LDA #$B0; STA PPU_CTRL_REG1
// Does NOT clear VRAM — vram[] is zero-initialized as a static array.
void set_ppu(void) {
    vblank_wait();
    PPU_CTRL_REG1 = 0xB0;
}



// Store_NT_Buffer_InVRAM
void store_nt_buffer_in_vram(void) {
    LowPtr_Byte = 0;            // LDA #0; STA LowPtr_Byte; TAY
    HighPtr_Byte = 4;           // LDA #4; STA HighPtr_Byte
at_:
    save_to_vram();             // JSR Save_to_VRAM
    inc_ptr_on_a(1);            // LDA #1; JSR Inc_Ptr_on_A
    if (HighPtr_Byte != 8) goto at_; // LDA HighPtr_Byte; CMP #8; BNE @_
}

void save_to_vram(void) {
    uint16_t vram_addr = ((uint16_t)(PPU_Addr_Ptr + HighPtr_Byte) << 8) | LowPtr_Byte;
    uint16_t ram_index = ((HighPtr_Byte - 4) << 8) | LowPtr_Byte;
    if (ram_index < 1024) {
        ppu_data_write(vram_addr, NT_Buffer[ram_index]);
    }
}



void screen_off(void) {
    nmi_wait();
    PPU_CTRL_REG1 = 0x10; // BG=Bank 1
    PPU_CTRL_REG2 = 0x06;
}

uint8_t ppu_status_read(void) {
    return atomic_fetch_and_explicit(&PPU_STATUS, (uint8_t)~0x80, memory_order_acq_rel);
}

void ppu_set_vblank_flag(void) {
    atomic_fetch_or_explicit(&PPU_STATUS, 0x80, memory_order_release);
}






static inline void update_nt_buffer_from_ppu_address(uint16_t ppu_addr, uint8_t hi, uint8_t lo, uint8_t val) {
    uint16_t nt_index = (((uint16_t)hi << 8) | lo) - 0x0400;
    NT_Buffer[nt_index] = val;
}

void string_to_screen_buffer(uint8_t x, uint8_t y, const uint8_t *str) {
    if (!str) return;

    uint16_t addr = coord_to_ppu_address(x, y);
    uint8_t hi = (uint8_t)(addr >> 8);
    uint8_t lo = (uint8_t)addr;
    uint8_t buf_hi = (uint8_t)(hi + PPU_Addr_Ptr);
    uint8_t pos = ScrBuffer_Pos;

    Screen_Buffer[pos++] = buf_hi;
    Screen_Buffer[pos++] = lo;

    /* ASM: STA HighStrPtr_Byte идёт ДО ADC PPU_Addr_Ptr — это raw hi. */
    HighStrPtr_Byte = hi;
    LowStrPtr_Byte = lo;

    uint16_t ppu_addr = ((uint16_t)buf_hi << 8) | lo;
    uint8_t index = 0;

at_:
    uint8_t value = str[index];
    Screen_Buffer[pos++] = value;
    if (value == 0xFF) goto at__;

    update_nt_buffer_from_ppu_address(ppu_addr, hi, lo, value);
    ppu_addr++;
    lo++;
    if (lo == 0u) {
        hi++;
    }
    index++;
    goto at_;

at__:
    ScrBuffer_Pos = pos;
}

void save_str_to_scr_buffer(uint8_t x, uint8_t y, const uint8_t *str) {
    if (!str) return;

    uint16_t addr = coord_to_ppu_address(x, y);
    uint8_t hi = (uint8_t)(addr >> 8);
    uint8_t lo = (uint8_t)addr;
    uint8_t buf_hi = (uint8_t)(hi + PPU_Addr_Ptr);
    uint8_t pos = ScrBuffer_Pos;
    const uint8_t *src = str;

    Screen_Buffer[pos++] = buf_hi;
    Screen_Buffer[pos++] = lo;

at_:
    uint8_t val = *src;
    if ((int8_t)val < 0) goto at__;
    val = (uint8_t)(val + Char_Index_Base);

at__:
    Screen_Buffer[pos++] = val;
    if (val == 0xFF) goto at___;

    src++;
    goto at_;

at___:
    ScrBuffer_Pos = pos;
}

void save_aligned_str_to_scr_buffer(uint8_t col, uint8_t row, uint8_t *base_str) {
    if (!base_str) return;
    uint8_t *p = ptr_to_nonzero_str_elem(base_str);
    uint8_t skip = (uint8_t)(p - base_str);
    save_str_to_scr_buffer((uint8_t)(col + skip), row, p);
}

uint8_t* ptr_to_nonzero_str_elem(uint8_t *str) {
    if (!str) return NULL;
    // Skip leading zeroes, but keep at least one digit.
    // Only the 0xFF terminator stops the scan.
    while (*str == 0 && *(str + 1) != 0xFF) {
        str++;
    }
    return str;
}
void null_8bytes_string(uint8_t *str) {
    if (!str) return;
    memset(str, 0, 7);
    str[7] = 0xFF;
}

/* ASM: Num_To_NumString (4291) */
/* ASM: Num_To_NumString (4291).
 * Value is treated as a packed pair of BCD digits (high nibble, low nibble):
 *   value=$10 → "10", value=$25 → "25", etc.  Special case: value==0
 *   stores '1' at Num_String[3] to render "1000" (the bonus points payload).
 */
void num_to_num_string(uint8_t value) {
    null_8bytes_string(Num_String);
    if (value == 0u) {
        Num_String[3] = 1u; /* "1000" bonus marker */
        return;
    }
    Num_String[5] = (uint8_t)(value & 0x0Fu);
    Num_String[4] = (uint8_t)((value >> 4u) & 0x0Fu);
}

/* ASM: ByteTo_Num_String (4333) — decimal split for arbitrary byte (0-99). */
void byte_to_num_string(uint8_t value) {
    null_8bytes_string(Num_String);
    while (value >= 10) {
        value = (uint8_t)(value - 10u);
        Num_String[5]++;
    }
    Num_String[6] = value;
}

void zero_page_viewer(void) {
    /* ASM: Zero_Page_Viewer (1395) - debug utility */
    num_to_num_string(ZeroPage_Offset);
    Char_Index_Base = 0x30u;
    save_str_to_scr_buffer(9u, 2u, Num_String + 4u);
    num_to_num_string(0u);   /* no real zero-page RAM in C port */
    Char_Index_Base = 0u;
    save_str_to_scr_buffer(0x0Du, 2u, Num_String + 4u);
    if ((Joypad1_Differ & 4u) != 0u) ZeroPage_Offset++;
    if ((Joypad1_Differ & 2u) != 0u) ZeroPage_Offset--;
    if ((Joypad1_Differ & 1u) != 0u) ZeroPage_Offset = (uint8_t)(ZeroPage_Offset + 0x10u);
}