#ifndef BATTLE_BONUS_H
#define BATTLE_BONUS_H

#include <stdint.h>

/* ASM: Bonus_Appear_Handle (7012) */
void bonus_appear_handle(void);
/* ASM: Bonus_Draw (5947) — Draw_Bonus (5978) включён как goto-метка внутри */
void bonus_draw(void);
/* ASM: Multiply_Bonus_Coord (7051) */
uint8_t multiply_bonus_coord(uint8_t a);
/* ASM: Bonus_Handle (7138) */
void bonus_handle(void);
/* ASM: Bonus_Helmet (7148) */
void bonus_helmet(void);
/* ASM: Bonus_Watch (7164) */
void bonus_watch(void);
/* ASM: Bonus_Shovel (7176) */
void bonus_shovel(void);
/* ASM: Bonus_Star (7188) */
void bonus_star(void);
/* ASM: Bonus_Grenade (7202) */
void bonus_grenade(void);
/* ASM: Bonus_Life (7240) */
void bonus_life(void);
/* ASM: Bonus_Pistol (7256) */
void bonus_pistol(void);

#endif // BATTLE_BONUS_H
