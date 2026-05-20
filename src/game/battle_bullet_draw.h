#ifndef BATTLE_BULLET_DRAW_H
#define BATTLE_BULLET_DRAW_H

#include <stdint.h>

/* ASM: Draw_All_BulletGFX (5669) */
void draw_all_bullet_gfx(void);
/* ASM: Draw_BulletGFX (5685) */
void draw_bullet_gfx(uint8_t slot);
/* ASM: Update_Ricochet (5721) */
void update_ricochet(uint8_t slot);

#endif // BATTLE_BULLET_DRAW_H
