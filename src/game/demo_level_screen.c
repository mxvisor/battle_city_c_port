#include "demo_level_screen.h"
#include "zeropage.h"
#include "battle_screen.h"
#include "battle_tank_draw.h"
#include "battle_tank_status.h"
#include "battle_bullet.h"
#include "battle_bonus.h"
#include "battle_hq.h"
#include "levels.h"
#include "draw.h"
#include "nmi.h"


static const uint8_t Tank_Direction[4] = { 0x13, 0x43, 0x23, 0x83 };

void load_demo_level(void) {
    Pause_Flag = 1;
    BkgPal_Number = 0;
    init_level_vars();
    Player2_Lives = 3;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    Seconds_Counter = 0;
    Frame_Counter = 0;
    make_gray_frame();
    Level_Number = 0xFF;
    load_level(Level_Number);
    Level_Number = 30;
    Level_Mode = 2;
    screen_off();

    Block_X = 0x1A;
    Block_Y = 0x46;
    draw_brick_str((const uint8_t*)"BATTLE");

    Block_X = 0x3C;
    Block_Y = 0x78;
    draw_brick_str((const uint8_t*)"CITY");

    store_nt_buffer_in_vram();
    set_ppu();
    setup_level_vars();
    draw_normal_hq();
    nmi_wait();
    TanksOnScreen = 5;
}

static void demo_ai(void) {
    Counter = 1;
    uint8_t demo_status = 0;

loop: /* ASM: @loop */
    if (Bonus_X == 0) {
        goto noBonus;
    }

    if (BonusPts_TimeCounter != 0) {
        goto noBonus;
    }

    goto take_Bonus;

noBonus: /* ASM: @noBonus */
    {
        uint8_t status = Tank_Status[2 + Counter];
        if ((status & 0x80u) == 0u) {
            goto __;
        }
        if (status >= 0xE0u) {
            goto __;
        }

        AI_X_Aim = Tank_X[2 + Counter];
        AI_Y_Aim = Tank_Y[2 + Counter];
        demo_status = load_ai_status(Counter);
        goto load_Direction_DemoAI;
    }

__: /* ASM: @__ */
    {
        uint8_t status = Tank_Status[4 + Counter];
        if ((status & 0x80u) == 0u) {
            goto ___;
        }
        if (status >= 0xE0u) {
            goto ___;
        }

        AI_X_Aim = Tank_X[4 + Counter];
        AI_Y_Aim = Tank_Y[4 + Counter];
        demo_status = load_ai_status(Counter);
        goto load_Direction_DemoAI;
    }

___: /* ASM: @___ */
    {
        uint8_t status = Tank_Status[3 + Counter];
        if ((status & 0x80u) == 0u) {
            goto enemiesNotActing;
        }
        if (status >= 0xE0u) {
            goto enemiesNotActing;
        }

        AI_X_Aim = Tank_X[3 + Counter];
        AI_Y_Aim = Tank_Y[3 + Counter];
        demo_status = load_ai_status(Counter);
        goto load_Direction_DemoAI;
    }

enemiesNotActing: /* ASM: @enemiesNotActing */
    {
        uint8_t button = 0;
        if (Counter == 0) {
            Joypad1_Buttons = button;
            Joypad1_Differ = button;
        } else {
            Joypad2_Buttons = button;
            Joypad2_Differ = button;
        }
        goto saveButton_DemoAI;
    }

take_Bonus: /* ASM: @take_Bonus */
    AI_X_Aim = Bonus_X;
    AI_Y_Aim = Bonus_Y;
    demo_status = load_ai_status(Counter);
    goto load_Direction_DemoAI;

load_Direction_DemoAI: /* ASM: @load_Direction_DemoAI */
    {
        uint8_t direction_index = demo_status & 3u;
        uint8_t button = Tank_Direction[direction_index];
        if (Counter == 0) {
            Joypad1_Buttons = button;
            Joypad1_Differ = button;
        } else {
            Joypad2_Buttons = button;
            Joypad2_Differ = button;
        }
    }

saveButton_DemoAI: /* ASM: @saveButton_DemoAI */
    {
        uint8_t y = Tank_Y[Counter];
        if (y >= 0xC8u) {
            if (Counter == 0) {
                Joypad1_Differ &= 0xF0u;
            } else {
                Joypad2_Differ &= 0xF0u;
            }
        }
    }

next_Demo_AI: /* ASM: @next_Demo_AI */
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto loop;
    }

    return;
}

void bonus_level_button_check(void) {
    while (1) {
        nmi_wait();
        if (Joypad1_Differ & 0x0C) { // SELECT or START
            ScrBuffer_Pos = 0;
            null_upper_nt();
            return;
        }

        demo_ai();
        battle_loop();
        bonus_draw();
        tanks_status_handle();
        draw_all_bullet_gfx();
        if (level_end_check()) {
            ScrBuffer_Pos = 0;
            return;
        }
    }
}
