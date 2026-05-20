#ifndef BATTLE_COLLIDE_H
#define BATTLE_COLLIDE_H

#include <stdint.h>

/* ASM: BulletToTank_Impact_Handle (6731) */
void bullet_to_tank_impact_handle(void);
/* ASM: BulletToBullet_Impact_Handle (7069) */
void bullet_to_bullet_impact_handle(void);
/* ASM: BulletToObject_Impact_Handle (6662) */
uint8_t bullet_to_object_impact_handle(uint8_t bullet_slot);
/* ASM: Draw_Destroyed_Brick (3753) */
void draw_destroyed_brick(void);
/* ASM: Check_Object (3742) */
uint8_t check_object(void);

#endif // BATTLE_COLLIDE_H
