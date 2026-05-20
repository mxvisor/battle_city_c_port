#ifndef PPU_SIM_H
#define PPU_SIM_H

#include <stdint.h>

/* Write a byte to emulated PPU VRAM with NES mirroring */
void ppu_sim_write(uint16_t addr, uint8_t val);

/* Game-facing PPU data write (implemented in nes layer) */
void ppu_data_write(uint16_t addr, uint8_t val);

/* Returns pointer to the 16KB emulated VRAM array */
uint8_t* ppu_get_vram_ptr(void);

/* Render one frame to a 256x240 RGB32 buffer */
void ppu_render(void *framebuffer);

#endif /* PPU_SIM_H */
