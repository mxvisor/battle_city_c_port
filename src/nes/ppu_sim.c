#include "ppu_sim.h"
#include "config.h"
#include "chr_load.h"
#include "game/ppu_registers.h"
#include "game/bss.h"
#include <stdbool.h>
#include <string.h>

static uint8_t vram[0x4000]; /* 16KB emulated PPU VRAM: $0000-$1FFF = CHR, $2000-$3EFF = NT, $3F00-$3FFF = palette */

/* PPU $2006 latch state + $2007 read buffer (поведение реального NES). */
static uint16_t ppu_addr;
static uint8_t ppu_addr_latch; /* 0 = ожидаем high byte, 1 = ожидаем low */
static uint8_t ppu_read_buf;

void ppu_address_write(uint8_t val) {
    if (ppu_addr_latch == 0u) {
        ppu_addr = (uint16_t)(((uint16_t)val << 8) | (ppu_addr & 0x00FFu));
        ppu_addr_latch = 1u;
    } else {
        ppu_addr = (uint16_t)((ppu_addr & 0xFF00u) | val);
        ppu_addr_latch = 0u;
    }
}

uint8_t ppu_data_read(void) {
    uint16_t addr = (uint16_t)(ppu_addr & 0x3FFFu);
    uint8_t result;
    if (addr < 0x3F00u) {
        /* Буферизованное чтение: возвращаем предыдущий буфер, заполняем новым. */
        result = ppu_read_buf;
        ppu_read_buf = vram[addr];
    } else {
        /* Палитра читается напрямую; буфер заполняется из зеркала NT. */
        result = vram[addr];
        ppu_read_buf = vram[addr - 0x1000u];
    }
    /* Auto-increment (PPU_CTRL_REG1 bit 2 = 0 → +1, =1 → +32). */
    uint16_t inc = (PPU_CTRL_REG1 & 0x04u) ? 32u : 1u;
    ppu_addr = (uint16_t)((ppu_addr + inc) & 0x3FFFu);
    return result;
}

void ppu_load_chr(const uint8_t *data, size_t len) {
    if (len > 0x2000u) len = 0x2000u;
    memcpy(vram, data, len);
}

void ppu_sim_write(uint16_t addr, uint8_t val) {
    addr &= 0x3FFF;
    if (addr >= 0x2000 && addr < 0x3F00) {
        vram[addr] = val;
        if      (addr >= 0x2000 && addr < 0x2400) vram[addr + 0x0400] = val;
        else if (addr >= 0x2400 && addr < 0x2800) vram[addr - 0x0400] = val;
        else if (addr >= 0x2800 && addr < 0x2C00) vram[addr + 0x0400] = val;
        else if (addr >= 0x2C00 && addr < 0x3000) vram[addr - 0x0400] = val;
    } else if (addr >= 0x3F00) {
        uint16_t pal_addr = addr & 0x001F;
        if ((pal_addr & 0x03) == 0) {
            uint8_t base_bg = pal_addr & 0x0C;
            vram[0x3F00 | base_bg] = val;
            vram[0x3F10 | base_bg] = val;
        } else {
            vram[0x3F00 | pal_addr] = val;
        }
    }
}

void ppu_data_write(uint16_t addr, uint8_t val) {
    ppu_sim_write(addr, val);
}

uint8_t* ppu_get_vram_ptr(void) {
    return vram;
}

void ppu_render(void *framebuffer) {
    uint32_t *fb = (uint32_t *)framebuffer;
    uint8_t *vram = ppu_get_vram_ptr();
    uint8_t *chr = vram; /* CHR теперь живёт в vram[$0000-$1FFF] */
    bool show_bg = (PPU_CTRL_REG2 & 0x08) != 0;
    bool show_spr = (PPU_CTRL_REG2 & 0x10) != 0;

    if (!show_bg) {
        uint32_t color = nes_palette[vram[0x3F00] & 63];
        for (int i = 0; i < NES_SCREEN_TOTAL; i++) {
            fb[i] = color;
        }
    }

    uint16_t nt_base = 0x2000 + ((PPU_CTRL_REG1 & 0x03) * 0x400);
    uint16_t pt_base = (PPU_CTRL_REG1 & 0x10) ? 0x1000 : 0x0000;
    int scroll_y = PPU_SCROLL_REG;
    
    if (show_bg) {
        for (int y = 0; y < NES_SCREEN_H; y++) {
            int total_y = y + scroll_y;
            int fy = total_y % 480;
            int ty = fy / 8;
            int py = fy % 8;

            uint16_t current_nt = (ty >= 30) ? (nt_base ^ 0x800) : nt_base;
            int row_in_nt = ty >= 30 ? ty - 30 : ty;

            for (int tx = 0; tx < 32; tx++) {
                uint16_t nt_addr = current_nt + row_in_nt * 32 + tx;
                uint8_t tile_idx = vram[nt_addr];

                uint16_t attr_addr = nt_base + 0x3C0 + (ty / 4) * 8 + (tx / 4);
                uint8_t attr_byte = vram[attr_addr];
                int shift = ((ty & 2) << 1) | (tx & 2);
                uint8_t palette_idx = (attr_byte >> shift) & 0x03;

                uint8_t p0 = chr[pt_base + (tile_idx * 16) + py];
                uint8_t p1 = chr[pt_base + (tile_idx * 16) + py + 8];
                for (int px = 0; px < 8; px++) {
                    uint8_t col_idx = ((p0 >> (7 - px)) & 1) | (((p1 >> (7 - px)) & 1) << 1);
                    uint32_t color;
                    if (col_idx == 0) {
                        color = nes_palette[vram[0x3F00] & 63];
                    } else {
                        color = nes_palette[vram[0x3F00 + palette_idx * 4 + col_idx] & 63];
                    }
                    int screen_x = tx * 8 + px;
                    if (screen_x < NES_SCREEN_W) {
                        fb[y * NES_SCREEN_W + screen_x] = color;
                    }
                }
            }
        }
    }

    if (show_spr) {
        for (int i = 63; i >= 0; i--) {
            uint8_t y = SprBuffer[i * 4] + 1;
            uint8_t tile = SprBuffer[i * 4 + 1];
            uint8_t attr = SprBuffer[i * 4 + 2];
            uint8_t x = SprBuffer[i * 4 + 3];

            if (y >= NES_SCREEN_H) continue;

            uint16_t pattern_table = (tile & 1) ? 0x1000 : 0x0000;
            uint8_t tile_index = tile & 0xFE;
            uint8_t palette_idx = (attr & 0x03) + 4;
            bool flip_v = attr & 0x80;
            bool flip_h = attr & 0x40;

            for (int py = 0; py < 16; py++) {
                int sprite_py = flip_v ? (15 - py) : py;
                uint16_t tile_addr = pattern_table + (tile_index + (sprite_py / 8)) * 16 + (sprite_py % 8);
                uint8_t p0 = chr[tile_addr];
                uint8_t p1 = chr[tile_addr + 8];

                for (int px = 0; px < 8; px++) {
                    int sprite_px = flip_h ? (7 - px) : px;
                    uint8_t col_idx = ((p0 >> (7 - sprite_px)) & 1) | (((p1 >> (7 - sprite_px)) & 1) << 1);

                    if (col_idx == 0) continue;

                    uint32_t color = nes_palette[vram[0x3F10 + (palette_idx - 4) * 4 + col_idx] & 63];
                    int screen_x = x + px;
                    int screen_y = y + py;
                    if (screen_x < NES_SCREEN_W && screen_y < NES_SCREEN_H) {
                        fb[screen_y * NES_SCREEN_W + screen_x] = color;
                    }
                }
            }
        }
    }
}
