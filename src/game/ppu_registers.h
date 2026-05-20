#ifndef PPU_REGISTERS_H
#define PPU_REGISTERS_H

#include <stdint.h>
#include <stdatomic.h>

extern uint8_t PPU_CTRL_REG1;       /* PPU Control Register #1 (W) */
extern uint8_t PPU_CTRL_REG2;       /* PPU Control Register #2 (W) */
extern atomic_uchar PPU_STATUS;     /* PPU Status Register (R) */
extern uint8_t PPU_SPR_ADDR;        /* SPR-RAM Address Register (W) */
extern uint8_t PPU_SPR_DATA;        /* SPR-RAM I/O Register (W) */
extern uint8_t PPU_SCROLL_REG;      /* VRAM Address Register #1 (W2) */
extern uint8_t PPU_ADDRESS;         /* VRAM Address Register #2 (W2) */
extern uint8_t PPU_DATA;            /* VRAM I/O Register (RW) */

#endif // PPU_REGISTERS_H
