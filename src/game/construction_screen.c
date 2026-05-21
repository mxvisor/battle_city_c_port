#include "construction_screen.h"
#include "battle_tank.h"
#include "battle_tank_draw.h"
#include "draw.h"
#include "battle_hq.h"
#include "nmi.h"
#include "zeropage.h"

void draw_tsa_on_tank(void) {
    Block_X = Tank_X[0];
    Block_Y = Tank_Y[0];
    draw_tsa_block(TSA_BlockNumber & 0x0F);
}

/* ASM: Construction (349). Режим конструктора: танк ходит по полю,
 * A/B циклят TSA_BlockNumber, нажатие A+B/направления рисует блок под танком,
 * START выходит обратно в Title_Loaded. */
void construction(void) {
    /* LDA Construction_Flag; BNE Skip_LoadFrame */
    if (Construction_Flag != 0u) goto Skip_LoadFrame;
    screen_off();
    make_gray_frame();
    store_nt_buffer_in_vram();
    set_ppu();

Skip_LoadFrame:
    null_status();
    Tank_X[0] = 0x10u;
    Tank_Y[0] = 0x18u;       /* Initial tank position */
    Tank_Status[0] = 0x84u;  /* Barrel up */
    Tank_Type[0] = 0u;
    Spr_Attrib = 0u;
    Track_Pos[0] = 0u;
    BkgOccurence_Flag = 0u;
    Joypad_Delay = 0u;
    TSA_BlockNumber = 0u;
    Scroll_Byte = 0u;
    PPU_REG1_Stts = 0u;
    Player_Blink_Timer[0] = 0u;
    Player_Blink_Timer[1] = 0u;
    /* LDA Construction_Flag; BNE Construction_Loop */
    if (Construction_Flag != 0u) goto Construction_Loop;
    draw_normal_hq();

Construction_Loop:
    nmi_wait();
    move_tank(0);
    check_border_reach(0);
    /* LDA Frame_Counter; AND #$10; BEQ Skip_Status_Handle */
    if ((Frame_Counter & 0x10u) == 0u) goto Skip_Status_Handle;
    tanks_status_handle();

Skip_Status_Handle:
    /* LDA Joypad1_Buttons; AND #$F0; BNE tsaNotFf */
    if ((Joypad1_Buttons & 0xF0u) != 0u) goto tsaNotFf;
    /* LDA Joypad1_Differ; AND #1; BEQ loBitNotSet */
    if ((Joypad1_Differ & 1u) == 0u) goto loBitNotSet;
    /* LDA BkgOccurence_Flag; BNE occurenceNotZero */
    if (BkgOccurence_Flag != 0u) goto occurenceNotZero;
    BkgOccurence_Flag = (uint8_t)(BkgOccurence_Flag + 1u);
    goto Construct_Draw_TSA;

occurenceNotZero:
    TSA_BlockNumber = (uint8_t)(TSA_BlockNumber + 1u);
    if (TSA_BlockNumber != 0x0Eu) goto Construct_Draw_TSA;
    TSA_BlockNumber = 0u;
    goto Construct_Draw_TSA;

loBitNotSet:
    /* LDA Joypad1_Differ; AND #2; BEQ tsaNotFf */
    if ((Joypad1_Differ & 2u) == 0u) goto tsaNotFf;
    if (BkgOccurence_Flag != 0u) goto occurenceNotZero_2;
    BkgOccurence_Flag = (uint8_t)(BkgOccurence_Flag + 1u);
    goto Construct_Draw_TSA;

occurenceNotZero_2:
    TSA_BlockNumber = (uint8_t)(TSA_BlockNumber - 1u);
    if (TSA_BlockNumber != 0xFFu) goto Construct_Draw_TSA;
    TSA_BlockNumber = 0x0Du; /* $D — first empty block (wrap) */
    goto Construct_Draw_TSA;

tsaNotFf:
    /* LDA Joypad1_Buttons; AND #3; BEQ Construct_StartCheck */
    if ((Joypad1_Buttons & 3u) == 0u) goto Construct_StartCheck;

Construct_Draw_TSA:
    draw_tsa_on_tank();

Construct_StartCheck:
    /* LDA Joypad1_Differ; AND #8; BNE End_Construction */
    if ((Joypad1_Differ & 8u) != 0u) goto End_Construction;
    goto Construction_Loop;

End_Construction:
    Spr_Attrib = 0x20u;
    Construction_Flag = (uint8_t)(Construction_Flag + 1u);
    /* JMP Title_Loaded — моделируется возвратом в caller (begin.c). */
}

static const uint8_t Coord_X_Increment[4] = { 0, 0xFF, 0, 1 };
static const uint8_t Coord_Y_Increment[4] = { 0xFF, 0, 1, 0 };

/* ASM: Move_Tank (1286). Двигает танк игрока на 16 пикселей при нажатии
 * направления. Если кнопка удерживается $14 кадров — переходит в авто-режим
 * с задержкой $0F кадров (auto-repeat). slot всегда 0 (ASM работает с Tank_X
 * напрямую, без индекса). */
void move_tank(uint8_t slot) {
    uint8_t direction;

    /* LDA Joypad1_Buttons; AND #$F0; BEQ ArrowNotPressed */
    if ((Joypad1_Buttons & 0xF0u) == 0u) goto ArrowNotPressed;
    /* INC Joypad_Delay; LDA #0; STA BkgOccurence_Flag; JMP checkJoypad */
    Joypad_Delay = (uint8_t)(Joypad_Delay + 1u);
    BkgOccurence_Flag = 0u;
    goto checkJoypad;

ArrowNotPressed:
    Joypad_Delay = 0u;

checkJoypad:
    /* LDA Joypad_Delay; CMP #$14; BEQ delayEq14 */
    if (Joypad_Delay == 0x14u) goto delayEq14;
    /* LDA Joypad1_Differ; AND #$F0; BEQ End_Move_Tank */
    if ((Joypad1_Differ & 0xF0u) == 0u) goto End_Move_Tank;
    /* LDA Joypad1_Differ; JSR Button_To_DirectionIndex; BMI End_Move_Tank */
    direction = button_to_direction_index(Joypad1_Differ);
    if (direction == 0xFFu) goto End_Move_Tank;
    /* JMP moveTank */
    goto moveTank;

delayEq14:
    /* LDA #$F; STA Joypad_Delay */
    Joypad_Delay = 0x0Fu;
    /* LDA Joypad1_Buttons; JSR Button_To_DirectionIndex */
    direction = button_to_direction_index(Joypad1_Buttons);
    if (direction == 0xFFu) goto End_Move_Tank;

moveTank:
    /* TAY; LDA Coord_X_Increment,Y; ASL×4 — *16; ADC Tank_X; STA Tank_X */
    Tank_X[slot] = (uint8_t)(Tank_X[slot] + (uint8_t)(Coord_X_Increment[direction] << 4));
    Tank_Y[slot] = (uint8_t)(Tank_Y[slot] + (uint8_t)(Coord_Y_Increment[direction] << 4));

End_Move_Tank:
    return;
}
