#ifndef APU_REGISTERS_H
#define APU_REGISTERS_H

#include <stdint.h>

extern uint8_t SND_SQUARE1_REG;                     /* pAPU Pulse #1 Control Register (W) */
extern uint8_t pAPU_Pulse1_Ramp_Control_Reg;        /* pAPU Pulse #1 Ramp Control Register (W) */
extern uint8_t pAPU_Pulse1__FT__Reg;                /* pAPU Pulse #1 Fine Tune (FT) Register (W) */
extern uint8_t pAPU_Pulse1__CT__Reg;                /* pAPU Pulse #1 Coarse Tune (CT) Register (W) */
extern uint8_t SND_SQUARE2_REG;                     /* pAPU Pulse #2 Control Register (W) */
extern uint8_t pAPU_Pulse2_Ramp_Control_Reg;        /* pAPU Pulse #2 Ramp Control Register (W) */
extern uint8_t pAPU_Pulse2__FT__Reg;                /* pAPU Pulse #2 Fine Tune Register (W) */
extern uint8_t pAPU_Pulse2__CT__Reg;                /* pAPU Pulse #2 Coarse Tune Register (W) */
extern uint8_t SND_TRIANGLE_REG;                    /* pAPU Triangle Control Register #1 (W) */
extern uint8_t pAPU_Triangle_Control_Reg2;          /* pAPU Triangle Control Register #2 (?) */
extern uint8_t pAPU_Triangle_Frequency_Reg1;        /* pAPU Triangle Frequency Register #1 (W) */
extern uint8_t pAPU_Triangle_Frequency_Reg2;        /* pAPU Triangle Frequency Register #2 (W) */
extern uint8_t SND_NOISE_REG;                       /* pAPU Noise Control Register #1 (W) */
extern uint8_t Unused;                              /* Unused (???) */
extern uint8_t pAPU_Noise_Frequency_Reg1;           /* pAPU Noise Frequency Register #1 (W) */
extern uint8_t pAPU_Noise_Frequency_Reg2;           /* pAPU Noise Frequency Register #2 (W) */
extern uint8_t SND_DELTA_REG;                       /* pAPU Delta Modulation Control Register (W) */
extern uint8_t pAPU_Delta_Modulation_DA_Reg;        /* pAPU Delta Modulation D/A Register (W) */
extern uint8_t pAPU_Delta_Modulation_Address_Reg;   /* pAPU Delta Modulation Address Register (W) */
extern uint8_t pAPU_Delta_Modulation_Data_Length_Reg; /* pAPU Delta Modulation Data Length Register (W) */
extern uint8_t SPR_DMA;                             /* Sprite DMA Register (W) */
extern uint8_t SND_MASTERCTRL_REG;                  /* pAPU Sound/Vertical Clock Signal Register (R) */
extern uint8_t JOYPAD_PORT1;                        /* Joypad #1 (RW) */
extern uint8_t JOYPAD_PORT2;                        /* Joypad #2/SOFTCLK (RW) */

#endif // APU_REGISTERS_H
