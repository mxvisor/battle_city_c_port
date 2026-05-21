#ifndef PPU_SIM_H
#define PPU_SIM_H

#include <stdint.h>
#include <stddef.h>

/* Write a byte to emulated PPU VRAM with NES mirroring */
void ppu_sim_write(uint16_t addr, uint8_t val);

/* Game-facing PPU data write (implemented in nes layer) */
void ppu_data_write(uint16_t addr, uint8_t val);

/* PPU $2006 — 16-bit VRAM address latch. Двойная запись (hi, затем lo). */
void ppu_address_write(uint8_t val);

/* PPU $2007 read — буферизованное чтение из VRAM (первый запрос после
 * установки адреса возвращает старое значение буфера, как на реальном NES). */
uint8_t ppu_data_read(void);

/* Загружает CHR-данные в pattern-tables области VRAM ($0000-$1FFF). */
void ppu_load_chr(const uint8_t *data, size_t len);

/* Returns pointer to the 16KB emulated VRAM array */
uint8_t* ppu_get_vram_ptr(void);

/* Render one frame to a 256x240 RGB32 buffer */
void ppu_render(void *framebuffer);

#endif /* PPU_SIM_H */
