#ifndef BATTLE_BULLET_H
#define BATTLE_BULLET_H

#include <stdint.h>

/* ASM: AllBulletsStatus_Handle (5520) */
void all_bullets_status_handle(void);
/* ASM: BulletStatus_Handle (5536) */
void bullet_status_handle(uint8_t slot);



/* ASM: Bullet_Fly_Handle (6553) */
void bullet_fly_handle(void);
/* ASM: Make_Shot (5612) */
void make_shot(uint8_t slot);
/* ASM: Make_Player_Shot (5738) */
void make_player_shot(uint8_t player_slot);
/* ASM: Make_Enemy_Shot (5785) */
void make_enemy_shot(uint8_t enemy_slot);
/* ASM: Make_Ricochet (5589) */
void make_ricochet(uint8_t slot);
/* ASM: Update_Ricochet (5721) */
void update_ricochet(uint8_t slot);
/* ASM: Hide_All_Bullets (6301) */
void hide_all_bullets(void);
/* ASM: Change_BulletCoord (5571) */
void change_bullet_coord(uint8_t slot, uint8_t direction);


/* ASM: Draw_All_BulletGFX (5669) */
void draw_all_bullet_gfx(void);
/* ASM: Draw_BulletGFX (5685) */
void draw_bullet_gfx(uint8_t slot);
/* ASM: Draw_Bullet (5702) */
void draw_bullet(uint8_t slot);
/* ASM: Draw_Bullet_Ricochet (5269) */
void draw_bullet_ricochet(uint8_t a_val);

#endif // BATTLE_BULLET_H
