#include "battle_screen.h"
#include "zeropage.h"
#include "bss.h"
#include "battle_respawn.h"
#include "battle_hud.h"
#include "battle_bullet.h"
#include "battle_tank.h"
#include "battle_bonus.h"
#include "battle_collide.h"
#include "battle_hq.h"
#include "nmi.h"
#include "draw.h"
#include <string.h>

static const uint8_t Coord_X_Increment[4] = { 0, 0xFF, 0, 1 };
static const uint8_t Coord_Y_Increment[4] = { 0xFF, 0, 1, 0 };

/* ASM: GameOver_Str_Move_Handle (???) */
void gameover_str_move_handle(void) {
    if (GameOverStr_Timer == 0) {
        goto End_GameOver_Str_Move;
    }

    if (Level_Mode == 2) {
        goto End_GameOver_Str_Move;
    }

    if ((Frame_Counter & 0x0F) != 0) {
        goto Check_Motion;
    }

    GameOverStr_Timer -= 1;
    if (GameOverStr_Timer == 0) {
        GameOverStr_Y = 0xF0;
    }

Check_Motion:
    if (GameOverStr_Timer < 10) {
        goto Stopped_Motion;
    }

    {
        uint8_t index = GameOverScroll_Type;
        GameOverStr_X = GameOverStr_X + Coord_X_Increment[index];
        GameOverStr_Y = GameOverStr_Y + Coord_Y_Increment[index];
    }

Stopped_Motion:
    draw_fixed_game_over();

End_GameOver_Str_Move:
    return;
}

void init_level_vars(void) {
    Player_Type[0] = 0;
    Player_Type[1] = 0;
    AddLife_Flag[0] = 0;
    AddLife_Flag[1] = 0;
    EnterGame_Flag = 0;
    Player1_Lives = 3;
    Player2_Lives = 3;
    EnemyRespawn_PlaceIndex = 3;
    if (CursorPos == 0) {
        Player2_Lives = 0;
    }
    Level_Number = 1;
    Level_Mode = 0;
}


void battle_loop(void) {
    ice_detect(0);//Processes player if on ice
    ice_move(0);//Performs sliding if tank moves on ice
    motion_handle();//Freezes enemies if needed (movement processing)
    hide_hi_bit_under_tank(0);//
    all_bullets_status_handle();// Processes statuses of	all bullets
    hq_handle();//Processes HQ status and armor
    invisible_timer_handle(0);//Draws force field if needed
    make_player_shot(0);//Performs player shot	if button is pressed
    make_enemy_shot(0);//Fires	using random numbers
    respawn_handle(0);//
    bullet_fly_handle();//Processes bullet flight (collision etc.)
    bullet_to_bullet_impact_handle();//Processes collision of two bullets if present
    bullet_to_tank_impact_handle();//Processes bullet-tank collision
    bonus_handle();//Processes bonus pickup if present
    gameover_str_move_handle();//Outputs Game	Over when required
    play_snd_move();//Plays	and stops movement sound when needed
    draw_player_lives();//Draws IP/IIP and lives count in upper-right	corner
    swap_pal_colors();//Periodic blinking - water effect on palette 01
}


/* ASM: Play_Snd_Move (4523) */
void play_snd_move(void) {
    /* LDA Snd_Move; BEQ No_MoveSound */
    if (Snd_Move == 0u) goto No_MoveSound;

    /* Snd_Move != 0 (звук играет): не пора ли заглушить? */
    /* LDX #0; JSR Detect_Motion; BNE End_Play_Snd_Move */
    if (detect_motion(0) != 0u) goto End_Play_Snd_Move;
    /* LDX #1; JSR Detect_Motion; BNE End_Play_Snd_Move */
    if (detect_motion(1) != 0u) goto End_Play_Snd_Move;
    /* LDA #0; STA Snd_Move; RTS */
    Snd_Move = 0u;
    return;

No_MoveSound:
    /* Snd_Move == 0 (тишина): не пора ли запустить? */
    /* LDX #0; JSR Detect_Motion; BNE @_ */
    if (detect_motion(0) != 0u) goto at_;
    /* LDX #1; JSR Detect_Motion; BEQ End_Play_Snd_Move */
    if (detect_motion(1) == 0u) goto End_Play_Snd_Move;

at_:
    Snd_Move = 1u;

End_Play_Snd_Move:
    return;
}


void swap_pal_colors(void) {
    uint8_t frame = Frame_Counter & 0x3F;

    if (frame == 0x20) {
        BkgPal_Number = 1;
        return;
    }

    if (frame == 0) {
        BkgPal_Number = 2;
    }
}

void setup_level_vars(void) {
    hide_all_bullets();
    null_status();
    GameOverStr_Y = 0xF0;
    GameOverStr_Timer = 0;

    if (Player1_Lives == 0) {
        goto Player1_Skip;
    }
    make_respawn(0);
Player1_Skip:

    if (Player2_Lives == 0) {
        goto Set_VARs;
    }
    make_respawn(1);

Set_VARs:
    Enemy_Reinforce_Count = 20;
    Enemy_Counter = 20;
    Enemy_TypeNumber = 0;
    Seconds_Counter = 0;
    Construction_Flag = 0;
    HQArmour_Timer = 0;
    Player_Blink_Timer[0] = 0;
    Player_Blink_Timer[1] = 0;
    Invisible_Timer[0] = 0;
    Invisible_Timer[1] = 0;
    Tmp_Unused = 0;
    Respawn_Timer = 0;
    Bonus_X = 0;
    EnemyFreeze_Timer = 0;
    EnemyRespawn_PlaceIndex = 0;

    null_killed_enms_count();
    draw_reinforcements();
    nmi_wait();
    draw_ip();
    draw_level_flag();
    load_enemy_count();

    HQ_Status = 0x80;
    Snd_Engine = 1;
    EnterGame_Flag = 1;

    if (Level_Mode == 1) {
        Temp = 35;
        goto Respawn_Delay_Calc;
    }

    Temp = Level_Number;

/* ASM: Respawn_Delay_Calc (798) */    
Respawn_Delay_Calc:
    Temp = Temp << 1;
    Temp = Temp << 1;
    Respawn_Delay = 190 - Temp;

    if (CursorPos == 0) {
        return;
    }

    Respawn_Delay -= 20;
}

void null_killed_enms_count(void) {
    memset(Enmy_KlledBy1P_Count, 0, sizeof(Enmy_KlledBy1P_Count));
    memset(Enmy_KlledBy2P_Count, 0, sizeof(Enmy_KlledBy2P_Count));
    TotalEnmy_KilledBy1P = 0;
    TotalEnmy_KilledBy2P = 0;
}

/* ASM: LevelEnd_Check (1360) */
uint8_t level_end_check(void) {
    if (HQ_Status == 0) {
        goto Init_GameOverStr;
    }

    if (Enemy_Counter == 0) {
        goto ExitLevel;
    }

    if ((uint8_t)(Player1_Lives + Player2_Lives) != 0) {
        goto PlayLevel;
    }

Init_GameOverStr:
    GameOverStr_X = 0x70;
    GameOverStr_Y = 0xF0;
    GameOverScroll_Type = 0;
    GameOverStr_Timer = 0x11;
    Frame_Counter = 0;

ExitLevel:
    return 1;

PlayLevel:
    return 0;
}

void freeze_player_on_hq_destroy(void) {
    if (HQ_Status == 0x80) {
        goto at_; /* ASM: @_ */
    }

    Joypad1_Buttons = 0;
    Joypad2_Buttons = 0;
    Joypad1_Differ = 0;
    Joypad2_Differ = 0;

at_: /* ASM: @_ */
    return;
}

void draw_fixed_game_over(void) {
    TSA_Pal = 3;
    Spr_Attrib = 0;

    Temp_X = GameOverStr_X;
    Temp_Y = GameOverStr_Y;
    Spr_TileIndex = 0x79;
    draw_whole_spr();

    Temp_X = (uint8_t)(GameOverStr_X + 0x10);
    Temp_Y = GameOverStr_Y;
    Spr_TileIndex = 0x7D;
    draw_whole_spr();

    Spr_Attrib = 0x20;
}

void draw_pause(void) {
    if (Pause_Flag == 0) {
        return;
    }

    if ((Frame_Counter & 0x10) == 0) {
        return;
    }

    TSA_Pal = 3;
    Spr_Attrib = 0;

    Spr_TileIndex = 0x17;
    save_spr_to_spr_buffer(0x64, 0x80);

    Spr_TileIndex = 0x19;
    save_spr_to_spr_buffer(0x6C, 0x80);

    Spr_TileIndex = 0x1B;
    save_spr_to_spr_buffer(0x74, 0x80);

    Spr_TileIndex = 0x1D;
    save_spr_to_spr_buffer(0x7C, 0x80);

    Spr_TileIndex = 0x1F;
    save_spr_to_spr_buffer(0x84, 0x80);

    Spr_Attrib = 0x20;
}
