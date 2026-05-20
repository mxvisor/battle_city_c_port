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

void construction(void) {
    if (Construction_Flag == 0) {
        screen_off();
        make_gray_frame();
        PPU_Addr_Ptr = 0x1C;
        store_nt_buffer_in_vram();
        set_ppu();
    }

    null_status();

    Tank_X[0] = 0x10;
    Tank_Y[0] = 0x18;
    Tank_Status[0] = 0x84;
    Tank_Type[0] = 0;
    Spr_Attrib = 0;
    Track_Pos[0] = 0;
    BkgOccurence_Flag = 0;
    Joypad_Delay = 0;
    TSA_BlockNumber = 0;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    BkgPal_Number = 0;
    Player_Blink_Timer[0] = 0;
    Player_Blink_Timer[1] = 0;

    if (Construction_Flag == 0) {
        draw_normal_hq();
    }

    while (1) {
        nmi_wait();
        move_tank(0);
        check_border_reach(0);

        if ((Frame_Counter & 0x10) != 0) {
            tanks_status_handle();
        }

        if ((Joypad1_Buttons & 0xF0) == 0) {
            if ((Joypad1_Differ & 1) != 0) {
                if (BkgOccurence_Flag != 0) {
                    TSA_BlockNumber += 1;
                    if (TSA_BlockNumber == 0x0E) {
                        TSA_BlockNumber = 0;
                    }
                } else {
                    BkgOccurence_Flag += 1;
                }
            } else if ((Joypad1_Differ & 2) != 0) {
                if (BkgOccurence_Flag != 0) {
                    TSA_BlockNumber -= 1;
                    if (TSA_BlockNumber == 0xFF) {
                        TSA_BlockNumber = 0x0D;
                    }
                } else {
                    BkgOccurence_Flag += 1;
                }
            }
        }

        if ((Joypad1_Buttons & 0x03) != 0) {
            draw_tsa_on_tank();
        }

        if ((Joypad1_Differ & 0x08) != 0) {
            break;
        }
    }

    Spr_Attrib = 0x20;
    Construction_Flag += 1;
}

static const uint8_t Coord_X_Increment[4] = { 0, 0xFF, 0, 1 };
static const uint8_t Coord_Y_Increment[4] = { 0xFF, 0, 1, 0 };

void move_tank(uint8_t slot) {
    uint8_t direction;

    if ((Joypad1_Buttons & 0xF0u) != 0u) {
        Joypad_Delay++;
        BkgOccurence_Flag = 0;
        goto checkJoypad;
    }

    Joypad_Delay = 0;

checkJoypad:
    if (Joypad_Delay == 0x14u) {
        Joypad_Delay = 0x0Fu;
        direction = button_to_direction_index(Joypad1_Buttons);
        if (direction == 0xFFu) {
            goto End_Move_Tank;
        }
        goto moveTank;
    }

    if ((Joypad1_Differ & 0xF0u) == 0u) {
        goto End_Move_Tank;
    }

    direction = button_to_direction_index(Joypad1_Differ);
    if (direction == 0xFFu) {
        goto End_Move_Tank;
    }

moveTank:
    {
        uint8_t dx = Coord_X_Increment[direction];
        uint8_t dy = Coord_Y_Increment[direction];
        uint8_t x = Tank_X[slot];
        uint8_t y = Tank_Y[slot];

        x = (uint8_t)(x + (uint8_t)(dx << 4));
        y = (uint8_t)(y + (uint8_t)(dy << 4));
        Tank_X[slot] = x;
        Tank_Y[slot] = y;
    }

End_Move_Tank:
    return;
}
