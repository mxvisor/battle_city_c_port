#ifndef BATTLE_H
#define BATTLE_H

#include <stdint.h>



/* ASM: GameOver_Str_Move_Handle (???) */
void gameover_str_move_handle(void);
/* ASM: Init_Level_VARs (666) */
void init_level_vars(void);
/* ASM: Battle_Loop (695) */
void battle_loop(void);
/* ASM: Swap_Pal_Colors (721) */
void swap_pal_colors(void);
/* ASM: SetUp_LevelVARs (742) */
void setup_level_vars(void);
/* ASM: Null_KilledEnms_Count (1344) */
void null_killed_enms_count(void);
/* ASM: LevelEnd_Check (1360) */
uint8_t level_end_check(void);
/* ASM: FreezePlayer_OnHQDestroy (639) */
void freeze_player_on_hq_destroy(void);
/* ASM: Draw_Pause (1698) */
void draw_pause(void);
/* ASM: Draw_Fixed_GameOver () */
void draw_fixed_game_over(void);
/* ASM: Play_Snd_Move (???) */
void play_snd_move(void);

#endif // BATTLE_H
