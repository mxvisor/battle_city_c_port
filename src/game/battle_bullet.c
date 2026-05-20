#include "battle_bullet.h"
#include "zeropage.h"
#include "random.h"
#include "bss.h"
#include "coords.h"
#include "battle_collide.h"


/* ASM: Bullet_Coord_X_Increment_1/_2 and Bullet_Coord_Y_Increment_1/_2 */
static const int8_t Bullet_Coord_X_Increment_1[4] = { 0, -1, 0, 1 };
static const int8_t Bullet_Coord_Y_Increment_1[4] = { -1, 0, 1, 0 };

void change_bullet_coord(uint8_t slot, uint8_t direction) {
    /* ASM: Change_BulletCoord (5571)
       LDA increment; ASL A; CLC; ADC Bullet_{X|Y},X */
    uint8_t dir = (uint8_t)(direction & 3u);
    Bullet_X[slot] = (uint8_t)(Bullet_X[slot] + (int8_t)(Bullet_Coord_X_Increment_1[dir] * 2));
    Bullet_Y[slot] = (uint8_t)(Bullet_Y[slot] + (int8_t)(Bullet_Coord_Y_Increment_1[dir] * 2));
}


void hide_all_bullets(void) {
    /* ASM: Hide_All_Bullets (6301) */
    for (int8_t i = 9; i >= 0; i--) {
        Bullet_Status[(uint8_t)i] = 0u;
    }
}

void make_shot(uint8_t slot) {
    /* ASM: Make_Shot (5612) */
    if (Bullet_Status[slot] != 0u) { return; }
    if (slot < 2u) { Snd_Shoot = 1u; }
    uint8_t dir = Tank_Status[slot] & 3u;
    Bullet_Status[slot] = (uint8_t)(dir | 0x40u);
    Bullet_X[slot] = (uint8_t)(Tank_X[slot] + (int8_t)(Bullet_Coord_X_Increment_1[dir] * 8));
    Bullet_Y[slot] = (uint8_t)(Tank_Y[slot] + (int8_t)(Bullet_Coord_Y_Increment_1[dir] * 8));
    Bullet_Property[slot] = 0u;

    uint8_t type_hi = Tank_Type[slot] & 0xF0u;
    if (type_hi == 0u)    { return; }
    if (type_hi == 0xC0u) { Bullet_Property[slot] = 1u; return; }
    if (type_hi == 0x60u) { Bullet_Property[slot] = 3u; return; }
    if ((type_hi & 0x80u) != 0u) { return; }
    Bullet_Property[slot] = 1u;
}

void make_player_shot(uint8_t player_slot) {
    /* ASM: Make_Player_Shot (5738) */
    (void)player_slot;
    Counter = 1u;
    goto mps_loop;

mps_next:
    if (Counter-- != 0u) { goto mps_loop; }
    return;

mps_loop:
    {
        uint8_t i = Counter;
        uint8_t status = Tank_Status[i];
        if ((int8_t)status >= 0)  { goto mps_next; }
        if (status >= 0xE0u)      { goto mps_next; }
        uint8_t joypad = (i == 0u) ? Joypad1_Differ : Joypad2_Differ;
        if ((joypad & 0x03u) == 0u) { goto mps_next; }
        if ((Tank_Type[i] & 0xC0u) == 0x40u) {
            if (Bullet_Status[i] != 0u) {
                if (Bullet_Status[i + 8u] != 0u) { goto mps_next; }
                Bullet_Status[i + 8u]   = Bullet_Status[i];
                Bullet_X[i + 8u]        = Bullet_X[i];
                Bullet_Y[i + 8u]        = Bullet_Y[i];
                Bullet_Property[i + 8u] = Bullet_Property[i];
                Bullet_Status[i] = 0u;
            }
        }
        make_shot(i);
    }
    goto mps_next;
}

void make_enemy_shot(uint8_t enemy_slot) {
    /* ASM: Make_Enemy_Shot (5785) */
    (void)enemy_slot;
    if (EnemyFreeze_Timer != 0u) { return; }
    uint8_t slot = 7u;
    goto mes_loop;

mes_next:
    slot--;
    if (slot != 1u) { goto mes_loop; }
    return;

mes_loop:
    {
        uint8_t status = Tank_Status[slot];
        if ((int8_t)status >= 0)  { goto mes_next; }
        if (status >= 0xE0u)      { goto mes_next; }
        if ((get_random_a() & 0x1Fu) == 0u) { make_shot(slot); }
    }
    goto mes_next;
}


/* ASM: Bullet_Fly_Handle (6553) */
void bullet_fly_handle(void) {
    uint8_t slot;
    uint8_t status;
    uint8_t dir;
    int8_t inc;
    uint8_t hit;

    Counter = 9u; /* Process 10 bullets */

loop: /* ASM: @loop */
    slot = Counter;
    status = Bullet_Status[slot];
    if ((status & 0xF0u) != 0x40u) {
        goto next_Bullet_Fly_Handle;
    }

    if (Bullet_Property[slot] == 0u) {
        if ((((uint8_t)(slot ^ Frame_Counter)) & 1u) == 0u) {
            goto next_Bullet_Fly_Handle;
        }
    }

at__: /* ASM: @__ */
    dir = status & 3u;
    inc = Bullet_Coord_X_Increment_1[dir];
    if (inc >= 0) {
        goto at___;
    }
    inc = (int8_t)(-inc);

at___: /* ASM: @___ */
    Temp_X = (uint8_t)inc;
    AI_X_DifferFlag = (uint8_t)(Temp_X << 2u);

    inc = Bullet_Coord_Y_Increment_1[dir];
    if (inc >= 0) {
        goto at____;
    }
    inc = (int8_t)(-inc);

at____: /* ASM: @____ */
    Temp_Y = (uint8_t)inc;
    AI_Y_DifferFlag = (uint8_t)(Temp_Y << 2u);

    /* ASM @6599: LDY Bullet_Y,X; LDA Bullet_X,X; TAX; JSR GetSprCoord_InTiles
     * (STX Spr_X; STY Spr_Y; JSR GetCoord_InTiles + fallthrough BulletToObject_Impact_Handle) */
    get_spr_coord_in_tiles(Bullet_X[slot], Bullet_Y[slot]);
    hit = bullet_to_object_impact_handle(slot);
    if (hit == 0u) {
        goto getCoord_Bullet_Fly_Handle;
    }

    /* Second check: JSR BulletToObject_Impact_Handle (explicit, после Spr_X/Spr_Y update) */
    slot = Counter;
    Spr_Y = (uint8_t)(Bullet_Y[slot] + AI_X_DifferFlag);
    Spr_X = (uint8_t)(Bullet_X[slot] + AI_Y_DifferFlag);
    bullet_to_object_impact_handle(slot);

getCoord_Bullet_Fly_Handle: /* ASM: @getCoord_Bullet_Fly_Handle */
    /* ASM @6622: LDA Bullet_Y,X; SEC; SBC Temp_X; TAY; LDA Bullet_X,X; SEC; SBC Temp_Y; TAX;
     * JSR GetSprCoord_InTiles (+ fallthrough BulletToObject_Impact_Handle) */
    slot = Counter;
    get_spr_coord_in_tiles(
        (uint8_t)(Bullet_X[slot] - Temp_Y),
        (uint8_t)(Bullet_Y[slot] - Temp_X));
    hit = bullet_to_object_impact_handle(slot);
    if (hit == 0u) {
        goto next_Bullet_Fly_Handle;
    }

    /* Fourth check: JSR BulletToObject_Impact_Handle */
    slot = Counter;
    Spr_Y = (uint8_t)(Bullet_Y[slot] - AI_X_DifferFlag - Temp_X);
    Spr_X = (uint8_t)(Bullet_X[slot] - AI_Y_DifferFlag - Temp_Y);
    bullet_to_object_impact_handle(slot);

next_Bullet_Fly_Handle: /* ASM: @next_Bullet_Fly_Handle */
    Counter--;
    if ((int8_t)Counter < 0) {
        goto End_Bullet_Fly_Handle;
    }
    goto loop;

End_Bullet_Fly_Handle:
    return;
}
