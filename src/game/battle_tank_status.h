#ifndef BATTLE_TANK_STATUS_H
#define BATTLE_TANK_STATUS_H

#include <stdint.h>


/* ASM: Status_Core (4751) */
void status_core(uint8_t slot);
/* ASM: Explode_Handle (5062) */
void explode_handle(uint8_t slot);
/* ASM: Misc_Status_Handle (4768) */
void misc_status_handle(uint8_t slot);
/* ASM: Get_RandomStatus (4920) */
void get_random_status(uint8_t slot);
/* ASM: Check_TileReach (5022) */
void check_tile_reach(uint8_t slot);
/* ASM: Compare_Block_X */
uint8_t compare_block_x(uint8_t a, uint8_t b);
/* ASM: Compare_Block_Y */
uint8_t compare_block_y(uint8_t a, uint8_t b);
/* ASM: Check_Obj (4809) */
void check_obj(uint8_t slot);
/* ASM: Save_AI_ToStatus (4999) */
void save_ai_to_status(uint8_t slot);
/* ASM: Aim_HQ (5117) */
void aim_hq(uint8_t slot);
/* ASM: Aim_ScndPlayer (5144) */
void aim_scnd_player(uint8_t slot);
/* ASM: Aim_FirstPlayer (4979) */
void aim_first_player(uint8_t slot);
/* ASM: Load_Tank (5156) */
void load_tank(uint8_t slot);
/* ASM: Set_Respawn (5140) */
void set_respawn(uint8_t slot);
/* ASM: Get_RandomAim (5172) */
void get_random_aim(void);
/* ASM: Relation_To_Byte (4471) */
uint8_t relation_to_byte(uint8_t a);
/* ASM: Load_AI_Status (5008) */
uint8_t load_ai_status(uint8_t slot);


#endif // BATTLE_TANK_STATUS_H
