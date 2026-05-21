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


/* ASM: Hide_All_Bullets (6315). Обнуляет Bullet_Status[0..9]. */
void hide_all_bullets(void) {
    uint8_t x = 9u;
at_:
    Bullet_Status[x] = 0u;
    x = (uint8_t)(x - 1u);
    if ((int8_t)x >= 0) goto at_;
}

/* ASM: Make_Shot (5612). Если Bullet_Status == 0, выпускает новую пулю
 * с направлением из Tank_Status и стартовой позицией Tank+8*direction.
 * Свойства зависят от Tank_Type. */
void make_shot(uint8_t x) {
    /* LDA Bullet_Status,X; BNE @exit */
    if (Bullet_Status[x] != 0u) goto exit_;
    /* CPX #2; BCS @skip */
    if (x >= 2u) goto skip_;
    Snd_Shoot = 1u;

skip_: /* ASM: @skip */
    {
        /* LDA Tank_Status,X; AND #3; TAY; ORA #$40; STA Bullet_Status,X */
        uint8_t dir = (uint8_t)(Tank_Status[x] & 3u);
        Bullet_Status[x] = (uint8_t)(dir | 0x40u);
        /* LDA Bullet_Coord_X_Increment_1,Y; ASL; ASL; ASL; ADC Tank_X,X */
        Bullet_X[x] = (uint8_t)(Tank_X[x] + (int8_t)(Bullet_Coord_X_Increment_1[dir] * 8));
        Bullet_Y[x] = (uint8_t)(Tank_Y[x] + (int8_t)(Bullet_Coord_Y_Increment_1[dir] * 8));
        Bullet_Property[x] = 0u;
        /* LDA Tank_Type,X; AND #$F0; BEQ @exit */
        uint8_t type_hi = (uint8_t)(Tank_Type[x] & 0xF0u);
        if (type_hi == 0u) goto exit_;
        /* CMP #$C0; BEQ @quickBullet_End_Make_Shot */
        if (type_hi == 0xC0u) goto quickBullet_End_Make_Shot;
        /* CMP #$60; BEQ @lastType */
        if (type_hi == 0x60u) goto lastType;
        /* AND #$80; BNE @exit */
        if ((type_hi & 0x80u) != 0u) goto exit_;
    }

quickBullet_End_Make_Shot: /* ASM: @quickBullet_End_Make_Shot */
    Bullet_Property[x] = 1u;
    return;

lastType: /* ASM: @lastType */
    Bullet_Property[x] = 3u;

exit_: /* ASM: @exit */
    return;
}

/* ASM: Make_Player_Shot (5738). Проходит 2 игрока, если нажат огонь и пуля
 * не активна — стреляет. Бонусный танк (Tank_Type & $C0 == $40) может выпустить
 * вторую пулю в +8 слот. */
void make_player_shot(uint8_t unused) {
    (void)unused;
    Counter = 1u;

loop_: /* ASM: @loop */
    {
        uint8_t i = Counter;
        /* LDA Tank_Status,X; BPL @next_Jump_Make_Shot */
        if ((int8_t)Tank_Status[i] >= 0) goto next_Jump_Make_Shot;
        /* CMP #$E0; BCS @next_Jump_Make_Shot */
        if (Tank_Status[i] >= 0xE0u) goto next_Jump_Make_Shot;
        /* LDA Joypad1_Differ,X; AND #3; BEQ @next_Jump_Make_Shot */
        {
            uint8_t joypad = (i == 0u) ? Joypad1_Differ : Joypad2_Differ;
            if ((joypad & 0x03u) == 0u) goto next_Jump_Make_Shot;
        }
        /* LDA Tank_Type,X; AND #$C0; CMP #$40; BNE @__ */
        if ((Tank_Type[i] & 0xC0u) != 0x40u) goto at__;
        /* LDA Bullet_Status,X; BEQ @__ */
        if (Bullet_Status[i] == 0u) goto at__;
        /* LDA Bullet_Status+8,X; BNE @next_Jump_Make_Shot */
        if (Bullet_Status[i + 8u] != 0u) goto next_Jump_Make_Shot;
        /* Копируем в +8 слот, обнуляем основной */
        Bullet_Status[i + 8u]   = Bullet_Status[i];
        Bullet_X[i + 8u]        = Bullet_X[i];
        Bullet_Y[i + 8u]        = Bullet_Y[i];
        Bullet_Property[i + 8u] = Bullet_Property[i];
        Bullet_Status[i] = 0u;

at__: /* ASM: @__ */
        make_shot(i);
    }

next_Jump_Make_Shot: /* ASM: @next_Jump_Make_Shot */
    Counter = (uint8_t)(Counter - 1u);
    if ((int8_t)Counter >= 0) goto loop_;
}

/* ASM: Make_Enemy_Shot (5785). Если EnemyFreeze_Timer==0, для каждого враждебного
 * танка (X=7..2): с вероятностью 1/32 (random & $1F == 0) выпускает пулю. */
void make_enemy_shot(uint8_t unused) {
    (void)unused;
    /* LDA EnemyFreeze_Timer; BNE @exit */
    if (EnemyFreeze_Timer != 0u) goto exit_;
    uint8_t x = 7u;

loop_: /* ASM: @loop */
    /* LDA Tank_Status,X; BPL @next_Make_Enemy_Shot */
    if ((int8_t)Tank_Status[x] >= 0) goto next_Make_Enemy_Shot;
    /* CMP #$E0; BCS @next_Make_Enemy_Shot */
    if (Tank_Status[x] >= 0xE0u) goto next_Make_Enemy_Shot;
    /* JSR Get_Random_A; AND #$1F; BNE @next */
    if ((get_random_a() & 0x1Fu) != 0u) goto next_Make_Enemy_Shot;
    make_shot(x);

next_Make_Enemy_Shot: /* ASM: @next_Make_Enemy_Shot */
    x = (uint8_t)(x - 1u);
    /* CPX #1; BNE @loop */
    if (x != 1u) goto loop_;

exit_: /* ASM: @exit */
    return;
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

    /* LDA Bullet_Property,X; BNE @__ */
    if (Bullet_Property[slot] != 0u) goto at__;
    /* TXA; EOR Frame_Counter; AND #1; BEQ @next_Bullet_Fly_Handle */
    if ((((uint8_t)(slot ^ Frame_Counter)) & 1u) == 0u) goto next_Bullet_Fly_Handle;

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
