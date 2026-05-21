#include "battle_bullet_status.h"
#include "battle_bullet.h"
#include "zeropage.h"

typedef void (*BulletFunc)(uint8_t);

/* placeholder */
static void status_noop(uint8_t slot) {
    (void)slot;
}

static const BulletFunc Bullet_Status_JumpTable[] = {
    status_noop,    // 0 — upper nibble 0: no bullet
    make_ricochet,  // 1 — upper nibble 1
    make_ricochet,  // 2 — upper nibble 2
    make_ricochet,  // 3 — upper nibble 3
    bullet_move     // 4 — upper nibble 4: flying
};



/* ASM: AllBulletsStatus_Handle (5520). Обрабатывает 10 пуль (8 + 2 доп.) сверху вниз. */
void all_bullets_status_handle(void) {
    Counter = 9u;
at_:
    bullet_status_handle(Counter);
    Counter = (uint8_t)(Counter - 1u);
    if ((int8_t)Counter >= 0) goto at_;
}

void bullet_status_handle(uint8_t slot) {
    /* ASM: LSR LSR LSR AND #$FE — upper nibble of Bullet_Status selects
       table entry; AND #$FE clears the lowest bit of the >>3 result so
       that bit 3 of status is ignored (only bits 7..4 matter). */
    uint8_t status = Bullet_Status[slot];
    uint8_t table_idx = (uint8_t)((status >> 3u) & 0xFEu) >> 1u;
    if (table_idx < 5u) {
        Bullet_Status_JumpTable[table_idx](slot);
    }
}

/* ASM: Make_Ricochet (5589). Считает кадры в младшем ниббле; на 0 — следующий
 * кадр рикошета (старший ниббл −$10) с новым счётчиком $03. */
void make_ricochet(uint8_t slot) {
    /* DEC Bullet_Status,X */
    Bullet_Status[slot] = (uint8_t)(Bullet_Status[slot] - 1u);
    /* LDA Bullet_Status,X; AND #$F; BNE @exit */
    uint8_t a = Bullet_Status[slot];
    if ((a & 0x0Fu) != 0u) goto exit_;
    /* LDA Bullet_Status,X; AND #$F0; SEC; SBC #$10; BEQ @skip */
    a = (uint8_t)((a & 0xF0u) - 0x10u);
    if (a == 0u) goto skip_;
    /* ORA #3 */
    a = (uint8_t)(a | 0x03u);

skip_: /* ASM: @skip */
    Bullet_Status[slot] = a;

exit_: /* ASM: @exit */
    return;
}

/* ASM: Bullet_Move (5563). Сдвигает пулю по направлению, дважды если flag speed. */
void bullet_move(uint8_t slot) {
    /* LDA Bullet_Status,X; AND #3; TAY */
    uint8_t direction = (uint8_t)(Bullet_Status[slot] & 3u);
    change_bullet_coord(slot, direction);
    /* LDA Bullet_Property,X; AND #1; BEQ End_Bullet_Move */
    if ((Bullet_Property[slot] & 0x01u) == 0u) goto End_Bullet_Move;
    change_bullet_coord(slot, direction);

End_Bullet_Move:
    return;
}



