#ifndef BATTLE_RESPAWN_H
#define BATTLE_RESPAWN_H

#include <stdint.h>

/* ASM: Respawn_Handle (4573) */
void respawn_handle(uint8_t slot);
/* ASM: Make_Respawn (6185) */
void make_respawn(uint8_t slot);
/* ASM: Load_New_Tank (6242) */
void load_new_tank(uint8_t slot);
/* ASM: Load_Enemy_Count (6343) */
void load_enemy_count(void);


#endif // BATTLE_RESPAWN_H
