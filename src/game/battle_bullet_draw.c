#include "battle_bullet_draw.h"
#include "battle_bullet.h"
#include "zeropage.h"
#include "draw.h"

typedef void (*BulletFunc)(uint8_t);

/* ASM BulletGFX_JumpTable (6464):
 *   .WORD End_Ice_Move    ; index 0 — простой RTS
 *   .WORD Update_Ricochet ; index 1
 *   .WORD Update_Ricochet ; index 2
 *   .WORD Update_Ricochet ; index 3
 *   .WORD Draw_Bullet     ; index 4
 * End_Ice_Move в C имеет void(void), а здесь нужен void(uint8_t),
 * поэтому делаем тонкий адаптер с тем же ASM-именем. */
static void bullet_end_ice_move(uint8_t slot) {
    (void)slot; /* ASM End_Ice_Move — просто RTS */
}

static const BulletFunc BulletGFX_JumpTable[5] = {
    bullet_end_ice_move,  /* 0 = End_Ice_Move */
    update_ricochet,      /* 1 */
    update_ricochet,      /* 2 */
    update_ricochet,      /* 3 */
    draw_bullet           /* 4 */
};


/* ASM: Draw_All_BulletGFX (5669) */
void draw_all_bullet_gfx(void) {
    /* LDA #9; STA Counter */
    Counter = 9u;

at_:
    /* LDX Counter; JSR Draw_BulletGFX */
    draw_bullet_gfx(Counter);
    /* DEC Counter; BPL @_ */
    Counter--;
    if ((int8_t)Counter >= 0) goto at_;
}

/* ASM: Draw_BulletGFX (5685) */
void draw_bullet_gfx(uint8_t slot) {
    uint8_t y;
    uint8_t idx;

    /* LDA Bullet_Status,X; LSR;LSR;LSR; AND #$FE; TAY — Y = байтовый offset */
    y = (uint8_t)((Bullet_Status[slot] >> 3u) & 0xFEu);
    /* LDA BulletGFX_JumpTable,Y / +1,Y; JMP (LowPtr) — в C индекс = Y/2 */
    idx = (uint8_t)(y >> 1u);
    if (idx < 5u) BulletGFX_JumpTable[idx](slot);
}

/* ASM: Update_Ricochet (5721) */
void update_ricochet(uint8_t slot) {
    Temp_X = Bullet_X[slot];
    Temp_Y = Bullet_Y[slot];
    uint8_t a = (uint8_t)(Bullet_Status[slot] + 0x40u);
    draw_bullet_ricochet(a);
}

/* ASM: Draw_Bullet (5702) */
void draw_bullet(uint8_t slot) {
    uint8_t direction;
    /* LDA Bullet_Status,X; AND #3; PHA — извлекаем направление и сохраняем */
    direction = (uint8_t)(Bullet_Status[slot] & 3u);
    /* LDY Bullet_Y,X; LDA Bullet_X,X; TAX — X = Bullet_X, Y = Bullet_Y */
    /* (в C это аргументы к Indexed_SaveSpr) */
    /* LDA #2; STA TSA_Pal */
    TSA_Pal = 2u;
    /* LDA #$B1; STA Spr_TileIndex */
    Spr_TileIndex = 0xB1u;
    /* PLA; JSR Indexed_SaveSpr */
    indexed_save_spr(direction, Bullet_X[slot], Bullet_Y[slot]);
}

/* ASM: Draw_Bullet_Ricochet (5269) определён в battle_tank_draw.c — это
 * fallthrough в Draw_Ricochet (тоже там). См. battle_tank_draw.c. */
