#include "battle_collide.h"
#include "zeropage.h"
#include "bss.h"
#include "draw.h"
#include "coords.h"
#include "battle_hq.h"
#include "score.h"
#include "battle_bonus.h"
#include "strings.h"

static const uint8_t EnemyKill_Score[4] = { 0x10, 0x20, 0x30, 0x40 };


uint8_t check_object(void) {
    uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    uint8_t tile = NT_Buffer[addr & 0x3FFu];
    return (uint8_t)((Temp | 0xF0u) & tile);
}

/* ASM: Draw_Destroyed_Brick (3753)
   Clears the Temp-masked quarter-bits from the tile, then schedules tile draw. */
void draw_destroyed_brick(void) {
    uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    addr &= 0x3FFu;
    uint8_t tile = (uint8_t)(NT_Buffer[addr] & ~Temp);
    Spr_TileIndex = tile;       /* ASM: A = modified tile, STA (LowPtr),Y via Draw_Tile */
    draw_tile();                /* writes Spr_TileIndex back to NT_Buffer and Screen_Buffer */
}

/* ASM: BulletToTank_Impact_Handle (6731) */
void bullet_to_tank_impact_handle(void) {
    uint8_t tank_slot;
    uint8_t bslot;
    uint8_t ts;
    uint8_t dx;
    uint8_t dy;
    uint8_t type_idx;

    Counter = 1u; /* First process players (enemy hits player) */

loop: /* ASM: @loop */
    tank_slot = Counter;
    ts = Tank_Status[tank_slot];
    if ((int8_t)ts >= 0) {
        goto jump_Next_Player_Tank_Impact;
    }
    if (ts < 0xE0u) {
        goto notRespawn;
    }

jump_Next_Player_Tank_Impact: /* ASM: @jump_Next_Player_Tank_Impact */
    goto next_Player_Tank_Impact;

notRespawn: /* ASM: @notRespawn */
    Counter2 = 7u; /* enemy bullets */

loop_2: /* ASM: @loop_2 */
    bslot = Counter2;
    if ((Bullet_Status[bslot] & 0xF0u) != 0x40u) {
        goto next_Bullet_Tank_Impact;
    }

    dx = (uint8_t)(Bullet_X[bslot] - Tank_X[tank_slot]);
    if ((int8_t)dx < 0) {
        dx = (uint8_t)(0u - dx);
    }

checkMinX_TankImpact: /* ASM: @checkMinX_TankImpact */
    if (dx >= 0x0Au) {
        goto next_Bullet_Tank_Impact;
    }

    dy = (uint8_t)(Bullet_Y[bslot] - Tank_Y[tank_slot]);
    if ((int8_t)dy < 0) {
        dy = (uint8_t)(0u - dy);
    }

checkMinY_TankImpact: /* ASM: @checkMinY_TankImpact */
    if (dy >= 0x0Au) {
        goto next_Bullet_Tank_Impact;
    }

    Bullet_Status[bslot] = 0x33u;
    if (Invisible_Timer[tank_slot] != 0u) {
        Bullet_Status[bslot] = 0u;
        goto next_Bullet_Tank_Impact;
    }

explode_Player_Tank_Impact: /* ASM: @explode_Player_Tank_Impact */
    Tank_Status[tank_slot] = 0x73u;
    Snd_PlayerExplode = 1u;
    Player_Type[tank_slot] = 0u;
    Tank_Type[tank_slot] = 0u;
    goto next_Player_Tank_Impact;

next_Bullet_Tank_Impact: /* ASM: @next_Bullet_Tank_Impact */
    Counter2--;
    if (Counter2 != 1u) {
        goto loop_2;
    }

next_Player_Tank_Impact: /* ASM: @next_Player_Tank_Impact */
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto loop;
    }

    Counter = 7u; /* now process enemies (player hits enemy) */

loop_3: /* ASM: @loop_3 */
    tank_slot = Counter;
    ts = Tank_Status[tank_slot];
    if ((int8_t)ts >= 0) {
        goto jumpNext_Enemy_Tank_Impact;
    }
    if (ts < 0xE0u) {
        goto notExploded;
    }

jumpNext_Enemy_Tank_Impact: /* ASM: @jumpNext_Enemy_Tank_Impact */
    goto next_Enemy_Tank_Impact;

notExploded: /* ASM: @notExploded */
    Counter2 = 9u; /* 10 bullets */

loop_4: /* ASM: @loop_4 */
    if ((Counter2 & 6u) == 0u) {
        goto conterEqual;
    }
    goto next_Bullet2_Tank_Impact;

conterEqual: /* ASM: @conterEqual */
    bslot = Counter2;
    if ((Bullet_Status[bslot] & 0xF0u) == 0x40u) {
        goto load_X_TankImpact;
    }
    goto next_Bullet2_Tank_Impact;

load_X_TankImpact: /* ASM: @load_X_TankImpact */
    dx = (uint8_t)(Bullet_X[bslot] - Tank_X[tank_slot]);
    if ((int8_t)dx < 0) {
        dx = (uint8_t)(0u - dx);
    }

checkMinX2_TankImpact: /* ASM: @checkMinX2_TankImpact */
    if (dx >= 0x0Au) {
        goto next_Bullet2_Tank_Impact;
    }

    dy = (uint8_t)(Bullet_Y[bslot] - Tank_Y[tank_slot]);
    if ((int8_t)dy < 0) {
        dy = (uint8_t)(0u - dy);
    }

checkMinY2_TankImpact: /* ASM: @checkMinY2_TankImpact */
    if (dy >= 0x0Au) {
        goto next_Bullet2_Tank_Impact;
    }

    Bullet_Status[bslot] = 0x33u;
    if ((Tank_Type[tank_slot] & 4u) == 0u) {
        goto skip_BonusHandle_TankImpact;
    }
    bonus_appear_handle();
    if (Tank_Type[tank_slot] != 0xE4u) {
        goto skip_BonusHandle_TankImpact;
    }
    Tank_Type[tank_slot]--;

skip_BonusHandle_TankImpact: /* ASM: @skip_BonusHandle_TankImpact */
    if ((Tank_Type[tank_slot] & 3u) == 0u) {
        goto explode_Enemy_Tank_Impact;
    }
    Tank_Type[tank_slot]--;
    Snd_ArmourRicochetTank = 1u;
    goto next_Bullet2_Tank_Impact;

explode_Enemy_Tank_Impact: /* ASM: @explode_Enemy_Tank_Impact */
    Tank_Status[tank_slot] = 0x73u;
    Snd_EnemyExplode = 1u;

    type_idx = (uint8_t)((Tank_Type[tank_slot] >> 5u) - 4u);
    Spr_X = (uint8_t)(Counter2 & 1u);
    if (Spr_X != 0u) {
        goto scndPlayerKll_Tank_Impact;
    }
    Enmy_KlledBy1P_Count[type_idx]++;
    goto score_TankImpact;

scndPlayerKll_Tank_Impact: /* ASM: @scndPlayerKll_Tank_Impact */
    Enmy_KlledBy2P_Count[type_idx]++;

score_TankImpact: /* ASM: @score_TankImpact */
    if (Level_Mode == 2u) {
        goto next_Enemy_Tank_Impact;
    }
    num_to_num_string(EnemyKill_Score[type_idx]);
    add_score(Spr_X);
    add_life(Spr_X);
    goto next_Enemy_Tank_Impact;

next_Bullet2_Tank_Impact: /* ASM: @next_Bullet2_Tank_Impact */
    Counter2--;
    if ((int8_t)Counter2 < 0) {
        goto next_Enemy_Tank_Impact;
    }
    goto loop_4;

next_Enemy_Tank_Impact: /* ASM: @next_Enemy_Tank_Impact */
    Counter--;
    if (Counter == 1u) {
        goto pvp;
    }
    goto loop_3;

pvp: /* ASM: @pvp */
    Counter = 1u; /* now player hits player */

loop_5: /* ASM: @loop_5 */
    tank_slot = Counter;
    ts = Tank_Status[tank_slot];
    if ((int8_t)ts >= 0) {
        goto jump_Next_Player2_Tank_Impact;
    }
    if (ts < 0xE0u) {
        goto notRespawn_2;
    }

jump_Next_Player2_Tank_Impact: /* ASM: @jump_Next_Player2_Tank_Impact */
    goto next_Player2_Tank_Impact;

notRespawn_2: /* ASM: @notRespawn_2 */
    Counter2 = 9u;

loop_6: /* ASM: @loop_6 */
    if ((Counter2 & 6u) != 0u) {
        goto next_Bullet3_Tank_Impact;
    }
    bslot = Counter2;
    if ((Bullet_Status[bslot] & 0xF0u) != 0x40u) {
        goto next_Bullet3_Tank_Impact;
    }
    if (((uint8_t)(Counter ^ Counter2) & 1u) == 0u) {
        goto next_Bullet3_Tank_Impact;
    }

    dx = (uint8_t)(Bullet_X[bslot] - Tank_X[tank_slot]);
    if ((int8_t)dx < 0) {
        dx = (uint8_t)(0u - dx);
    }

checkMinX3_TankImpact: /* ASM: @checkMinX3_TankImpact */
    if (dx >= 0x0Au) {
        goto next_Bullet3_Tank_Impact;
    }

    dy = (uint8_t)(Bullet_Y[bslot] - Tank_Y[tank_slot]);
    if ((int8_t)dy < 0) {
        dy = (uint8_t)(0u - dy);
    }

checkMinY3_TankImpact: /* ASM: @checkMinY3_TankImpact */
    if (dy >= 0x0Au) {
        goto next_Bullet3_Tank_Impact;
    }

    Bullet_Status[bslot] = 0x33u;
    if (Invisible_Timer[tank_slot] == 0u) {
        goto checkBlink_TankImpact;
    }
    Bullet_Status[bslot] = 0u;
    goto next_Bullet3_Tank_Impact;

checkBlink_TankImpact: /* ASM: @checkBlink_TankImpact */
    if (Player_Blink_Timer[tank_slot] != 0u) {
        goto next_Bullet3_Tank_Impact;
    }
    if (Level_Mode == 2u) {
        goto next_Bullet3_Tank_Impact;
    }
    Player_Blink_Timer[tank_slot] = 0xC8u;
    goto next_Player2_Tank_Impact;

next_Bullet3_Tank_Impact: /* ASM: @next_Bullet3_Tank_Impact */
    Counter2--;
    if ((int8_t)Counter2 >= 0) {
        goto loop_6;
    }

next_Player2_Tank_Impact: /* ASM: @next_Player2_Tank_Impact */
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto loop_5;
    }
    return;
}

/* ASM: BulletToBullet_Impact_Handle (7069) */
void bullet_to_bullet_impact_handle(void) {
    uint8_t slot_x;
    uint8_t slot_y;
    uint8_t dx;
    uint8_t dy;

    Counter = 9u; /* 10 bullets */

loop: /* ASM: @loop */
    if ((Counter & 6u) != 0u) {
        goto next_Bullet_Bulllet_Impact;
    }
    slot_x = Counter;
    if ((Bullet_Status[slot_x] & 0xF0u) != 0x40u) {
        goto next_Bullet_Bulllet_Impact;
    }

    Counter2 = 9u; /* 10 bullets */

loop_2: /* ASM: @loop_2 */
    slot_y = Counter2;
    Temp = (uint8_t)(slot_y & 7u);
    if ((Counter & 7u) == Temp) {
        goto next_Bullet2_Bulllet_Impact;
    }
    if ((Bullet_Status[slot_y] & 0xF0u) != 0x40u) {
        goto next_Bullet2_Bulllet_Impact;
    }

    dx = (uint8_t)(Bullet_X[slot_y] - Bullet_X[slot_x]);
    if ((int8_t)dx < 0) {
        dx = (uint8_t)(0u - dx);
    }

checkMinX_BulletImpact: /* ASM: @checkMinX_BulletImpact */
    if (dx >= 6u) {
        goto next_Bullet2_Bulllet_Impact;
    }

    dy = (uint8_t)(Bullet_Y[slot_y] - Bullet_Y[slot_x]);
    if ((int8_t)dy < 0) {
        dy = (uint8_t)(0u - dy);
    }

checkMinY_BulletImpact: /* ASM: @checkMinY_BulletImpact */
    if (dy >= 6u) {
        goto next_Bullet2_Bulllet_Impact;
    }

    Bullet_Status[slot_x] = 0u;
    Bullet_Status[slot_y] = 0u;

next_Bullet2_Bulllet_Impact: /* ASM: @next_Bullet2_Bulllet_Impact */
    Counter2--;
    if ((int8_t)Counter2 >= 0) {
        goto loop_2;
    }

next_Bullet_Bulllet_Impact: /* ASM: @next_Bullet_Bulllet_Impact */
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto loop;
    }
    return;
}

/* ASM: BulletToObject_Impact_Handle (6662) */
uint8_t bullet_to_object_impact_handle(uint8_t slot) {
    temp_coord_shl();
    if (check_object() == 0u) {
        goto BulletToObject_Return0;
    }

    {
        uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
        uint8_t tile = NT_Buffer[addr & 0x3FFu];

        if ((tile & 0xFCu) == 0xC8u) {
            if (HQ_Status == 0u) {
                goto at_;
            }
            HQ_Status = 0x27u;
            Snd_HQExplode = 1u;
            Snd_PlayerExplode = 1u;
            draw_destroyed_hq();
            Bullet_Status[slot] = 0x33u;
            goto BulletToObject_Return0;
        }

at_:
        if (tile >= 0x12u) {
            goto BulletToObject_Return0;
        }

        Bullet_Status[slot] = 0x33u;
        if (tile == 0x11u) {
            goto Armored_Wall;
        }

        if ((Bullet_Property[slot] & 0x02u) != 0u) {
            Spr_TileIndex = 0u;
            draw_tile();
            Snd_Brick_Ricochet = 1u;
            goto BulletToObject_Return0;
        }

at__:
        if (tile == 0x10u) {
            goto Armored_Wall;
        }
        if (slot >= 2u) {
            goto BulletToObject_Return1;
        }
        Snd_Brick_Ricochet = 1u;
    }

BulletToObject_Return1:
    draw_destroyed_brick();
    return 1;

Armored_Wall:
    if (slot >= 2u) {
        goto BulletToObject_Return0;
    }
    Snd_ArmourRicochetWall = 1u;

BulletToObject_Return0:
    return 0;
}

