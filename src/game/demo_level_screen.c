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


/* ASM: Tank_Direction (1268) — направления танков в demo-уровне в формате джойстика */
static const uint8_t Tank_Direction[4] = { 0x13, 0x43, 0x23, 0x83 };

/* ASM-данные aBattle/aCity дублируются здесь и в title_screen.c (в оригинале —
 * общая ROM-таблица, обоими функциями используется как 0xFF-terminated). */
static const uint8_t aBattle[] = { 'B','A','T','T','L','E', 0xFFu };
static const uint8_t aCity[]   = { 'C','I','T','Y', 0xFFu };

/* ASM: Load_DemoLevel (820) — нет внутренних меток, прямая последовательность */
void load_demo_level(void) {
    /* LDA #1; STA Pause_Flag */
    Pause_Flag = 1u;
    /* LDA #0; STA BkgPal_Number */
    BkgPal_Number = 0u;
    /* JSR Init_Level_VARs */
    init_level_vars();
    /* LDA #3; STA Player2_Lives — независимо от выбора игрока, 2-й танк есть */
    Player2_Lives = 3u;
    /* LDA #0; STA Scroll_Byte / PPU_REG1_Stts / Seconds_Counter / Frame_Counter */
    Scroll_Byte = 0u;
    PPU_REG1_Stts = 0u;
    Seconds_Counter = 0u;
    Frame_Counter = 0u;
    /* JSR Make_GrayFrame */
    make_gray_frame();
    /* LDA #$FF; STA Level_Number; JSR Load_Level */
    Level_Number = 0xFFu;
    load_level(Level_Number);
    /* LDA #30; STA Level_Number — в правом углу bonus level показывает №30 */
    Level_Number = 30u;
    /* LDA #2; STA Level_Mode */
    Level_Mode = 2u;
    /* JSR Screen_Off */
    screen_off();

    /* LDX #$1A; STX Block_X; LDY #$46; STY Block_Y; JSR Draw_BrickStr("BATTLE") */
    Block_X = 0x1Au;
    Block_Y = 0x46u;
    draw_brick_str(aBattle);

    /* LDX #$3C; STX Block_X; LDY #$78; STY Block_Y; JSR Draw_BrickStr("CITY") */
    Block_X = 0x3Cu;
    Block_Y = 0x78u;
    draw_brick_str(aCity);

    /* JSR Store_NT_Buffer_InVRAM */
    store_nt_buffer_in_vram();
    /* JSR Set_PPU */
    set_ppu();
    /* JSR SetUp_LevelVARs */
    setup_level_vars();
    /* JSR DraW_Normal_HQ */
    draw_normal_hq();
    /* JSR NMI_Wait */
    nmi_wait();
    /* LDA #5; STA TanksOnScreen */
    TanksOnScreen = 5u;
}

/* ASM: Demo_AI (1187). Все внутренние ASM-метки сохранены. */
void demo_ai(void) {
    uint8_t demo_status = 0u;
    uint8_t status;
    uint8_t button;

    /* LDA #1; STA Counter — обработать обоих игроков */
    Counter = 1u;

loop:
    /* LDX Counter; LDA Bonus_X; BEQ @noBonus */
    if (Bonus_X == 0u) goto noBonus;
    /* LDA BonusPts_TimeCounter; BNE @noBonus */
    if (BonusPts_TimeCounter != 0u) goto noBonus;
    /* ASM fallthrough → @take_Bonus; explicit goto делает метку used */
    goto take_Bonus;

take_Bonus:
    /* LDA Bonus_X; STA AI_X_Aim */
    AI_X_Aim = Bonus_X;
    /* LDA Bonus_Y; STA AI_Y_Aim */
    AI_Y_Aim = Bonus_Y;
    /* JSR Load_AI_Status */
    demo_status = load_ai_status(Counter);
    /* JMP @load_Direction_DemoAI */
    goto load_Direction_DemoAI;

noBonus:
    /* LDA Tank_Status+2,X; BPL @__ */
    status = Tank_Status[2u + Counter];
    if ((int8_t)status >= 0) goto at__;
    /* CMP #$E0; BCS @__ */
    if (status >= 0xE0u) goto at__;
    /* LDA Tank_X+2,X; STA AI_X_Aim */
    AI_X_Aim = Tank_X[2u + Counter];
    /* LDA Tank_Y+2,X; STA AI_Y_Aim */
    AI_Y_Aim = Tank_Y[2u + Counter];
    /* JSR Load_AI_Status; JMP @load_Direction_DemoAI */
    demo_status = load_ai_status(Counter);
    goto load_Direction_DemoAI;

at__:
    /* LDA Tank_Status+4,X; BPL @___ */
    status = Tank_Status[4u + Counter];
    if ((int8_t)status >= 0) goto at___;
    /* CMP #$E0; BCS @___ */
    if (status >= 0xE0u) goto at___;
    AI_X_Aim = Tank_X[4u + Counter];
    AI_Y_Aim = Tank_Y[4u + Counter];
    demo_status = load_ai_status(Counter);
    goto load_Direction_DemoAI;

at___:
    /* LDA Tank_Status+3,X; BPL @enemiesNotActing */
    status = Tank_Status[3u + Counter];
    if ((int8_t)status >= 0) goto enemiesNotActing;
    /* CMP #$E0; BCS @enemiesNotActing */
    if (status >= 0xE0u) goto enemiesNotActing;
    AI_X_Aim = Tank_X[3u + Counter];
    AI_Y_Aim = Tank_Y[3u + Counter];
    demo_status = load_ai_status(Counter);
    goto load_Direction_DemoAI;

enemiesNotActing:
    /* LDA #0; JMP @saveButton_DemoAI — A=0 идёт через saveButton как button */
    button = 0u;
    goto saveButton_DemoAI;

load_Direction_DemoAI:
    /* AND #3; TAY; LDA Tank_Direction,Y — A теперь content of Tank_Direction */
    button = Tank_Direction[demo_status & 3u];
    /* fallthrough → @saveButton_DemoAI */

saveButton_DemoAI:
    /* LDX Counter; STA Joypad1_Buttons,X; STA Joypad1_Differ,X */
    if (Counter == 0u) {
        Joypad1_Buttons = button;
        Joypad1_Differ  = button;
    } else {
        Joypad2_Buttons = button;
        Joypad2_Differ  = button;
    }
    /* LDA Tank_Y,X; CMP #$C8; BCC @next_Demo_AI */
    if (Tank_Y[Counter] < 0xC8u) goto next_Demo_AI;
    /* LDA Joypad1_Differ,X; AND #$F0; STA Joypad1_Differ,X */
    if (Counter == 0u) {
        Joypad1_Differ = (uint8_t)(Joypad1_Differ & 0xF0u);
    } else {
        Joypad2_Differ = (uint8_t)(Joypad2_Differ & 0xF0u);
    }

next_Demo_AI:
    /* DEC Counter; BPL @loop */
    Counter--;
    if ((int8_t)Counter >= 0) goto loop;
}

/* ASM: BonusLevel_ButtonCheck (873).
 * Все ASM-метки сохранены (`DemoLevel_Loop`, `End_Demo`, `Button_Pressed`).
 * Возврат:
 *   0 — нормальный выход (End_Demo): caller продолжит с New_Scroll.
 *   1 — `Button_Pressed` (был нажат SELECT/START): ASM делает `PLA PLA; JMP
 *       Title_Loaded`, что в C моделируется кодом возврата → caller сделает
 *       `goto title_loaded`. */
int bonus_level_button_check(void) {
BonusLevel_ButtonCheck:
    /* JSR NMI_Wait */
    nmi_wait();
    /* LDA Joypad1_Differ; AND #%1100; BNE Button_Pressed */
    if ((Joypad1_Differ & 0x0Cu) != 0u) goto Button_Pressed;
    /* ASM fallthrough → DemoLevel_Loop */
    goto DemoLevel_Loop;

DemoLevel_Loop:
    /* JSR Demo_AI */
    demo_ai();
    /* JSR Battle_Loop */
    battle_loop();
    /* JSR Bonus_Draw */
    bonus_draw();
    /* JSR TanksStatus_Handle */
    tanks_status_handle();
    /* JSR Draw_All_BulletGFX */
    draw_all_bullet_gfx();
    /* JSR LevelEnd_Check; BEQ BonusLevel_ButtonCheck — возврат к началу функции.
     * В ASM это безусловный branch без push stack; в C — явный goto. PLA PLA
     * в Button_Pressed аналога не требует, т.к. C использует return codes. */
    if (level_end_check() == 0u) goto BonusLevel_ButtonCheck;
    /* ASM fallthrough → End_Demo */
    goto End_Demo;

End_Demo:
    /* LDA #0; STA ScrBuffer_Pos; RTS */
    ScrBuffer_Pos = 0u;
    return 0;

Button_Pressed:
    /* PLA; PLA — сброс caller's return address; в C моделируется return 1 */
    /* LDA #0; STA ScrBuffer_Pos */
    ScrBuffer_Pos = 0u;
    /* JSR Null_Upper_NT */
    null_upper_nt();
    /* JMP Title_Loaded — caller (begin.c) сделает `goto title_loaded` */
    return 1;
}
