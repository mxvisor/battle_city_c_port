#ifndef BATTLE_TANK_DRAW_H
#define BATTLE_TANK_DRAW_H

#include <stdint.h>

/* ASM: TanksStatus_Handle (5215) */
void tanks_status_handle(void);
/* ASM: SingleTankStatus_Handle (5233) */
void single_tank_status_handle(uint8_t slot);
/* ASM: Draw_Kill_Points (5296) */
void draw_kill_points(uint8_t enemy_slot);
/* ASM: Draw_Small_Explode1 (5336) */
void draw_small_explode1(uint8_t slot);
/* ASM: Draw_Small_Explode2 (5250) */
void draw_small_explode2(uint8_t slot);
/* ASM: Draw_Big_Explode (5352) */
void draw_big_explode(uint8_t slot);
/* ASM: Set_SprIndex (5408) */
void set_spr_index(uint8_t value);
/* ASM: OperatingTank (5432) */
void operating_tank(uint8_t slot);
/* ASM: Respawn (5491) */
void respawn(uint8_t slot);
/* ASM: Draw_Ricochet (5283) — fallthrough из Draw_Bullet_Ricochet. */
void draw_ricochet(uint8_t tile_offset);
#endif // BATTLE_TANK_DRAW_H
