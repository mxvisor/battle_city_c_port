#include "stage_select_screen.h"
#include "zeropage.h"
#include "bss.h"
#include "nmi.h"
#include "sound_engine.h"
#include "draw.h"
#include "battle_screen.h"
#include "battle_tank_draw.h"
#include "game_over_screen.h"
#include "hiscore_screen.h"
#include "pts_screen.h"
#include "battle_hq.h"
#include "levels.h"
#include "strings.h"
#include "coords.h"
#include "battle_bonus.h"
#include "battle_bullet.h"
#include "battle_tank_status.h"
#include "title_screen.h"

/* ASM: Draw_StageNumString (1977). Прямой набор тайлов "STAGE __" в Screen_Buffer
 * + два разряда номера уровня через ByteTo_Num_String + Save_Str_To_ScrBuffer. */
void draw_stage_num_string(void) {
    nmi_wait();
    /* LDX #$C; LDY #$E; JSR CoordTo_PPUaddress — A=hi, Y=lo */
    uint16_t addr = coord_to_ppu_address(0x0Cu, 0x0Eu);

    uint8_t x = ScrBuffer_Pos;
    /* CLC; ADC #$1C; STA Screen_Buffer,X */
    Screen_Buffer[x++] = (uint8_t)((addr >> 8) + PPU_Addr_Ptr);
    Screen_Buffer[x++] = (uint8_t)addr;
    
    Screen_Buffer[x++] = 0x23;
    Screen_Buffer[x++] = 0x24;
    Screen_Buffer[x++] = 0x25;
    Screen_Buffer[x++] = 0x26;
    Screen_Buffer[x++] = 0x27;
    Screen_Buffer[x++] = 0x11;
    Screen_Buffer[x++] = 0x11;
    Screen_Buffer[x++] = 0xFF;
    
    ScrBuffer_Pos = x;
    
    Char_Index_Base = 0x6E;
    byte_to_num_string(Level_Number);
    uint8_t *stage_num_str = ptr_to_nonzero_str_elem(Num_String + 1);
    uint8_t x_coord = 0x0E + (uint8_t)(stage_num_str - (Num_String + 1));
    save_str_to_scr_buffer(x_coord, 0x0E, stage_num_str);
    Char_Index_Base = 0;
}

void start_stage_sel_scrn(void) {
    nmi_wait();

Start_StageSelScrn:
    sound_stop();
    PPU_Addr_Ptr = 0x1C;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    Pause_Flag = 0;
    BkgPal_Number = 4;
    fill_nt_with_grey();

StageSelect_Loop:
    draw_stage_num_string();
    
    if (EnterGame_Flag != 0) {
        goto Start_Level;
    }
    
    if (Joypad1_Differ & 8) {
        goto Start_Level;
    }

    if (Joypad1_Differ & 1) {
        goto Inc_LevelNum;
    }

    if ((Joypad1_Buttons & 1) != 0 && (Frame_Counter & 7) == 0) {
        goto Inc_LevelNum;
    }

    if (Joypad1_Differ & 2) {
        goto Dec_LevelNum;
    }

    if ((Joypad1_Buttons & 2) != 0 && (Frame_Counter & 7) == 0) {
        goto Dec_LevelNum;
    }

    goto StageSelect_Loop;

Inc_LevelNum:
    Frame_Counter = 0;
    Level_Number++;
    if (Level_Number >= 36) {
        Level_Number = 35;
    }
    goto StageSelect_Loop;

Dec_LevelNum:
    Frame_Counter = 0;
    Level_Number--;
    if (Level_Number <= 0) {
        Level_Number = 1;
    }
    goto StageSelect_Loop;

Start_Level:
    Sound_PlaybackState[1] = 1;
    Sound_PlaybackState[2] = 1;
    Sound_PlaybackState[3] = 1;

    if (Construction_Flag != 0) {
        goto Skip_Lvl_Load;
    }

    make_gray_frame();
    load_level(Level_Number);
    draw_normal_hq();
    goto BuildScreen;

Skip_Lvl_Load:
    draw_naked_hq();

BuildScreen:
    ScrBuffer_Pos = 0;
    copy_attrib_to_scrn_buff();
    fill_nt_with_black();
    BkgPal_Number = 0;
    nmi_wait();
    setup_level_vars();

Battle_Engine:
    /* JSR NMI_Wait */
    nmi_wait();
    /* LDA Pause_Flag; BNE Skip_Battle_Loop */
    if (Pause_Flag != 0u) goto Skip_Battle_Loop;
    /* JSR Battle_Loop */
    battle_loop();

Skip_Battle_Loop:
    /* JSR Bonus_Draw; JSR Draw_All_BulletGFX; JSR TanksStatus_Handle */
    bonus_draw();
    draw_all_bullet_gfx();
    tanks_status_handle();
    /* LDA Joypad1_Differ; AND #8; BEQ Skip_Pause_Switch — START toggle */
    if ((Joypad1_Differ & 8u) == 0u) goto Skip_Pause_Switch;
    /* LDA #1; EOR Pause_Flag; STA Pause_Flag; STA Sound_PlaybackState */
    Pause_Flag = (uint8_t)(Pause_Flag ^ 1u);
    Sound_PlaybackState[0] = Pause_Flag;

Skip_Pause_Switch:
    /* JSR Draw_Pause */
    draw_pause();
    /* JSR LevelEnd_Check; BEQ Battle_Engine — level не закончен → продолжаем */
    if (level_end_check() == 0u) goto Battle_Engine;
    /* LDA #0; STA Seconds_Counter / Frame_Counter / Snd_Move / Snd_Engine */
    Seconds_Counter = 0u;
    Frame_Counter = 0u;
    Snd_Move = 0u;
    Snd_Engine = 0u;
    /* LDA GameOverStr_Timer; BEQ AfterDeath_BattleRun */
    if (GameOverStr_Timer == 0u) goto AfterDeath_BattleRun;
    /* LDA #$FE; STA Seconds_Counter */
    Seconds_Counter = 0xFEu;

AfterDeath_BattleRun:
    /* JSR NMI_Wait */
    nmi_wait();
    /* JSR FreezePlayer_OnHQDestroy */
    freeze_player_on_hq_destroy();
    /* JSR Battle_Loop; Bonus_Draw; TanksStatus_Handle; Draw_All_BulletGFX; Swap_Pal_Colors */
    battle_loop();
    bonus_draw();
    tanks_status_handle();
    draw_all_bullet_gfx();
    swap_pal_colors();
    /* LDA Seconds_Counter; CMP #2; BNE AfterDeath_BattleRun */
    if (Seconds_Counter != 2u) goto AfterDeath_BattleRun;

    /* JSR Sound_Stop; JSR Draw_Pts_Screen */
    sound_stop();
    draw_pts_screen();
    /* INC Level_Number; LDA Level_Number; CMP #71; BNE Ckeck_FirstFinish */
    Level_Number++;
    if (Level_Number != 71u) goto Ckeck_FirstFinish;
    /* LDA #1; STA Level_Number; LDA #0; STA Level_Mode */
    Level_Number = 1u;
    Level_Mode = 0u;

Ckeck_FirstFinish:
    /* LDA Level_Number; CMP #36; BNE Check_GameOver */
    if (Level_Number != 36u) goto Check_GameOver;
    /* LDA #1; STA Level_Mode */
    Level_Mode = 1u;

Check_GameOver:
    /* LDA Player1_Lives; CLC; ADC Player2_Lives; BEQ Make_GameOver */
    if ((uint8_t)(Player1_Lives + Player2_Lives) == 0u) goto Make_GameOver;
    /* LDA HQ_Status; CMP #$80; BNE Make_GameOver */
    if (HQ_Status != 0x80u) goto Make_GameOver;
    /* JMP Start_StageSelScrn — победа на уровне */
    goto Start_StageSelScrn;

Make_GameOver:
    /* JSR Draw_Brick_GameOver */
    draw_brick_game_over();
    /* JSR Update_HiScore; TYA; BEQ Skip_RecordShow */
    if (update_hi_score() == 0) goto Skip_RecordShow;
    /* JSR Draw_Record_HiScore; JSR Clear_NT */
    draw_record_hi_score();
    clear_nt();

Skip_RecordShow:
    /* JMP BEGIN — моделируется return'ом; caller (selected_*player → title_screen_loop)
     * вернёт код 3, и begin.c сделает `goto begin_label`. */
    return;
}
