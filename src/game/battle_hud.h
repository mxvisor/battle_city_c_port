#ifndef BATTLE_HUD_H
#define BATTLE_HUD_H

#include <stdint.h>

/* ASM: Draw_Reinforcemets (1645) */
void draw_reinforcements(void);
/* ASM: PointAt_RightScrnColumn (1599) */
void point_at_right_scrn_column(uint8_t icon_index, uint8_t *x, uint8_t *y);
/* ASM: ReinforceToRAM (1618) */
void reinforce_to_ram(uint8_t icon_index);
/* ASM: Draw_LevelFlag (1566) */
void draw_level_flag(void);
/* ASM: Draw_IP (1536) */
void draw_ip(void);
/* ASM: Draw_Player_Lives (1472) */
void draw_player_lives(void);
/* ASM: Draw_EmptyTile (1632) */
void draw_empty_tile(void);

#endif // BATTLE_HUD_H
