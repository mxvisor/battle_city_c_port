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



void all_bullets_status_handle(void) {
    /* ASM: processes 10 bullets from index 9 down to 0 */
    for (int8_t i = 9; i >= 0; i--) {
        bullet_status_handle((uint8_t)i);
    }
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

void make_ricochet(uint8_t slot) {
    /* ASM: Make_Ricochet (5589)
       Decrements frame counter in low nibble; when zero, advances to
       next ricochet frame (decrement upper nibble by 1) with 3 new frames. */
    Bullet_Status[slot]--;
    uint8_t status = Bullet_Status[slot];
    if ((status & 0x0Fu) != 0u) {
        return; /* BNE @exit — still counting down */
    }
    uint8_t upper = (uint8_t)((status & 0xF0u) - 0x10u); /* SEC SBC #$10 */
    if (upper != 0u) {
        upper |= 0x03u; /* ORA #3 — set 3-frame counter for new frame */
    }
    Bullet_Status[slot] = upper; /* BEQ @skip stores 0; else stores next frame */
}

void bullet_move(uint8_t slot) {
    /* ASM: Bullet_Move (5553) */
    uint8_t direction = (uint8_t)(Bullet_Status[slot] & 3u);
    change_bullet_coord(slot, direction);
    if ((Bullet_Property[slot] & 0x01u) == 0u) {
        goto End_Bullet_Move;
    }
    change_bullet_coord(slot, direction);

End_Bullet_Move:
    /* ASM: End_Bullet_Move */
    return;
}



