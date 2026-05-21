#ifndef BATTLE_HQ_H
#define BATTLE_HQ_H

#include <stdint.h>

/* ASM: DraW_Normal_HQ (2034) */
void draw_normal_hq(void);
/* ASM: Draw_Naked_HQ (2089) */
void draw_naked_hq(void);
/* ASM: Draw_ArmourHQ (2128) */
void draw_armour_hq(void);
/* ASM: Draw_Destroyed_HQ (2185) */
void draw_destroyed_hq(void);
/* ASM: HQ_Handle (6032) */
void hq_handle(void);
/* ASM: FirstExplode_Pic */
void first_explode_pic(void);
/* ASM: SecondExplode_Pic */
void second_explode_pic(void);
/* ASM: ThirdExplode_Pic */
void third_explode_pic(void);
/* ASM: FourthExplode_Pic */
void fourth_explode_pic(void);
/* ASM: FifthExplode_Pic */
void fifth_explode_pic(void);
/* ASM: Draw_HQSmallExplode (6115) */
void draw_hq_small_explode(uint8_t tile);
/* ASM: Draw_BigExplode (6162). HQExplode_SprBase устанавливается caller'ом до вызова. */
void draw_hq_big_explode(void);
/* ASM: Add_ExplodeSprBase (6128) */
void add_explode_spr_base(uint8_t delta);
/* ASM: Draw_SmallExplode (6121) */
void draw_small_explode(uint8_t tile);

#endif // BATTLE_HQ_H
