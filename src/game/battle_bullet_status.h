#ifndef BATTLE_BULLET_STATUS_H
#define BATTLE_BULLET_STATUS_H

#include <stdint.h>

/* ASM: AllBulletsStatus_Handle (5520) */
void all_bullets_status_handle(void);
/* ASM: BulletStatus_Handle (5536) */
void bullet_status_handle(uint8_t slot);
/* ASM: Make_Ricochet (5589) */
void make_ricochet(uint8_t slot);
/* ASM: Bullet_Move (5553) */
void bullet_move(uint8_t slot);

#endif // BATTLE_BULLET_STATUS_H
