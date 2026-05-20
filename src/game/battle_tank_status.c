#include "battle_tank_status.h"
#include "zeropage.h"
#include "bss.h"
#include "battle_tank.h"
#include "battle_respawn.h"
#include "random.h"
#include "coords.h"

void init_game_over_properties(void) {
    GameOverStr_Timer = 0x0D;
    GameOverStr_Y = 0xD8;
    Frame_Counter = 0;
}


static const uint8_t AI_Status[18] = {
    0xA0, 0xA0, 0xA0, 0xA1, 0xA0, 0xA3, 0xA2, 0xA2, 0xA2,
    0xA1, 0xA0, 0xA3, 0xA1, 0xA0, 0xA3, 0xA1, 0xA2, 0xA3
};



typedef void (*TankFunc)(uint8_t);

/* placeholder */
static void status_noop(uint8_t slot) {
    (void)slot;
}


static const TankFunc TankStatus_JumpTable[] = {
    status_noop,         // 0
    explode_handle,      // 2
    explode_handle,      // 4
    explode_handle,      // 6
    explode_handle,      // 8
    explode_handle,      // 10
    explode_handle,      // 12
    explode_handle,      // 14
    misc_status_handle,  // 16
    get_random_status,   // 18
    check_tile_reach,    // 20
    aim_hq,              // 22
    aim_scnd_player,     // 24
    aim_first_player,    // 26
    load_tank,           // 28
    set_respawn          // 30
};



void status_core(uint8_t slot) {
    uint8_t status = Tank_Status[slot];
    uint8_t index = (status >> 3) & 0xFE;
    TankStatus_JumpTable[index >> 1](slot);
}

void explode_handle(uint8_t slot) {
    Tank_Status[slot] -= 1;
    uint8_t status = Tank_Status[slot];
    if ((status & 0x0F) != 0) {
        goto End_Explode_Handle;
    }

    uint8_t a = (uint8_t)(status - 0x10);
    if (a == 0) {
        goto Skip_Explode_Handle;
    }

    if (a == 0x10) {
        a |= 6;
        goto SaveStts_Explode_Handle;
    }

    a |= 3;

SaveStts_Explode_Handle:
    Tank_Status[slot] = a;
    return;

Skip_Explode_Handle:
    Tank_Status[slot] = a; /* a == 0 */
    if (slot >= 2) {
        goto Dec_Enemy_Explode_Handle;
    }

    if (slot == 0) {
        Player1_Lives--;
        if (Player1_Lives == 0) { goto CheckHQ_Explode_Handle; }
    } else {
        Player2_Lives--;
        if (Player2_Lives == 0) { goto CheckHQ_Explode_Handle; }
    }

    make_respawn(slot);
    return;

Dec_Enemy_Explode_Handle:
    Enemy_Counter -= 1;
    return;

CheckHQ_Explode_Handle:
    if (HQ_Status != 0x80) {
        goto End_Explode_Handle;
    }
    if (slot == 1) {
        goto Check1pLives_Explode_Handle;
    }
    if (Player2_Lives == 0) {
        goto End_Explode_Handle;
    }
    GameOverScroll_Type = 3;
    GameOverStr_X = 0x20;
    init_game_over_properties();
    return;

Check1pLives_Explode_Handle:
    if (Player1_Lives == 0) {
        goto End_Explode_Handle;
    }
    GameOverScroll_Type = 1;
    GameOverStr_X = 0xC0;
    init_game_over_properties();

End_Explode_Handle:
    return;
}

void misc_status_handle(uint8_t slot) {
    if (slot >= 2u) {
        goto LoadStts_Misc_Status_Handle;
    }

    {
        uint8_t ice_status = Player_Ice_Status[slot];
        if ((int8_t)ice_status >= 0) {
            goto LoadStts_Misc_Status_Handle;
        }

        if ((ice_status & 0x7Fu) == 0u) {
            goto LoadStts_Misc_Status_Handle;
        }

        Player_Ice_Status[slot] = (uint8_t)(ice_status - 1u);
        Track_Pos[slot] ^= 4u;
        check_obj(slot);
        return;
    }

LoadStts_Misc_Status_Handle:
    {
        uint8_t status = Tank_Status[slot];
        status = (uint8_t)(status - 4u);
        Tank_Status[slot] = status;
        if ((status & 0x0Cu) != 0u) {
            goto End_Misc_Status_Handle;
        }
        Temp = 0xA0u; /* ASM: LDA #Tank_Status — ZP address of Tank_Status = $A0
                          resets status to 0xA0|dir = check_tile_reach (normal move) */
        rise_tank_status_bit(slot);
    }

End_Misc_Status_Handle:
    return;
}

void get_random_status(uint8_t slot) {
    if ((get_random_a() & 1u) == 0u) {
        goto End_Get_RandomStatus;
    }

    if ((get_random_a() & 1u) == 0u) {
        goto Sbc_Get_RandomStatus;
    }

    {
        uint8_t status = Tank_Status[slot];
        uint8_t new_dir = (uint8_t)((status + 1u) & 3u);
        Tank_Status[slot] = (uint8_t)((status & ~3u) | new_dir);
        return;
    }

Sbc_Get_RandomStatus:
    {
        uint8_t status = Tank_Status[slot];
        uint8_t new_dir = (uint8_t)((status - 1u) & 3u);
        Tank_Status[slot] = (uint8_t)((status & ~3u) | new_dir);
        return;
    }

End_Get_RandomStatus:
    get_random_aim();
    return;
}

void save_ai_to_status(uint8_t slot) {
    /* ASM: Save_AI_ToStatus (4999)
     * JSR Load_AI_Status; STA Tank_Status,X */
    Tank_Status[slot] = load_ai_status(slot);
}

uint8_t load_ai_status(uint8_t slot) {
    uint8_t x_diff = (uint8_t)(AI_X_Aim - Tank_X[slot]);
    AI_X_DifferFlag = (uint8_t)(relation_to_byte(x_diff) + 1);

    uint8_t y_diff = (uint8_t)(AI_Y_Aim - Tank_Y[slot]);
    AI_Y_DifferFlag = (uint8_t)(relation_to_byte(y_diff) + 1);

    uint8_t index = (uint8_t)(AI_Y_DifferFlag * 3u + AI_X_DifferFlag);
    uint8_t status_index;

    if (slot >= 2) {
        if ((get_random_a() & 1u) == 0u) {
            status_index = index;
        } else {
            status_index = (uint8_t)(9u + index);
        }
    } else {
        if (slot == 0 || Tank_Status[1] == 0u) {
            status_index = index;
        } else {
            status_index = (uint8_t)(9u + index);
        }
    }

    return AI_Status[status_index];
}

uint8_t compare_block_x(uint8_t a, uint8_t b) {
    if (a >= b) {
        return (uint8_t)(a - 1);
    }
    return a;
}

uint8_t compare_block_y(uint8_t a, uint8_t b) {
    if (a >= b) {
        return (uint8_t)(a - 1);
    }
    return a;
}

void check_obj(uint8_t slot) {
    uint8_t dir = Tank_Status[slot] & 3;
    int8_t dx;
    int8_t dy;

    switch (dir) {
        case 0:
            dx = 0;
            dy = -1;
            break;
        case 1:
            dx = -1;
            dy = 0;
            break;
        case 2:
            dx = 0;
            dy = 1;
            break;
        default:
            dx = 1;
            dy = 0;
            break;
    }
    int8_t dx8 = dx << 3;
    int8_t dy8 = dy << 3;

    Tmp_Status1 = (uint8_t)dx8;
    Tmp_Status2 = (uint8_t)dy8;
    Block_X = (uint8_t)(Tank_X[slot] + dx);
    Block_Y = (uint8_t)(Tank_Y[slot] + dy);

    uint8_t check_x = compare_block_x((uint8_t)(Block_X + dx8 + dy8), Block_X);
    uint8_t check_y = compare_block_y((uint8_t)(Block_Y + dx8 + dy8), Block_Y);
    /* ASM @4844: TAX / TAY / JSR GetCoord_InTiles (direct — Spr_X/Spr_Y не трогаем) */
    get_coord_in_tiles_xy(check_x, check_y);
    uint8_t tile = NT_Buffer[(LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8)) & 0x3FF];

    if ((tile & 0x80) || (tile != 0 && tile < 0x20)) {
        goto GetRnd_CheckObj;
    }

    if (tile == 0) {
        check_x = compare_block_x((uint8_t)(Block_X + dx8 - dy8), Block_X);
        check_y = compare_block_y((uint8_t)(Block_Y + dy8 - dx8), Block_Y);
        /* ASM @4866: TAX / TAY / JSR GetCoord_InTiles (direct) */
        get_coord_in_tiles_xy(check_x, check_y);
        tile = NT_Buffer[(LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8)) & 0x3FF];
        if ((tile & 0x80) || (tile != 0 && tile < 0x20)) {
            goto GetRnd_CheckObj;
        }
    }

    Tank_X[slot] = Block_X;
    Tank_Y[slot] = Block_Y;
    goto TrackHandle_CheckObj;

GetRnd_CheckObj:
    if (slot < 2) {
        goto TrackHandle_CheckObj;
    }

    if ((get_random_a() & 3u) != 0u) {
        Temp = 0x80u;
        rise_tank_status_bit(slot);
        Tank_Status[slot] |= 0x08u;
        goto TrackHandle_CheckObj;
    }

    if ((Tank_X[slot] & 7u) != 0u || (Tank_Y[slot] & 7u) != 0u) {
        goto Change_Direction_Check_Obj;
    }

    Temp = 0x90u;
    rise_tank_status_bit(slot);

Change_Direction_Check_Obj:
    Tank_Status[slot] ^= 2u;
    return;

TrackHandle_CheckObj:
    Track_Pos[slot] ^= 4u;
    return;
}

void check_tile_reach(uint8_t slot) {
    if (slot < 2u) {
        goto Check_Obj;
    }

    if ((Tank_X[slot] & 7u) != 0u) {
        goto Check_Obj;
    }

    if ((Tank_Y[slot] & 7u) != 0u) {
        goto Check_Obj;
    }

    if ((get_random_a() & 0x0Fu) != 0u) {
        goto Check_Obj;
    }

    get_random_aim();
    return;

Check_Obj:
    check_obj(slot);
    return;
}

void aim_hq(uint8_t slot) {
    AI_X_Aim = 0x78;
    AI_Y_Aim = 0xD8;
    save_ai_to_status(slot);
}

void aim_scnd_player(uint8_t slot) {
    AI_X_Aim = Tank_X[1];
    AI_Y_Aim = Tank_Y[1];
    save_ai_to_status(slot);
}

void aim_first_player(uint8_t slot) {
    AI_X_Aim = Tank_X[0];
    AI_Y_Aim = Tank_Y[0];
    save_ai_to_status(slot);
}

void load_tank(uint8_t slot) {
    Tank_Status[slot] += 1;
    if ((Tank_Status[slot] & 0x0F) != 0x0E) {
        goto End_Load_Tank;
    }
    load_new_tank(slot);

End_Load_Tank:
    return;
}

void set_respawn(uint8_t slot) {
    Tank_Status[slot] += 1;
    if ((Tank_Status[slot] & 0x0F) != 0x0E) {
        goto End_Set_Respawn;
    }

    Tank_Status[slot] = 0xE0;

End_Set_Respawn:
    return;
}


void get_random_aim(void) {
    /* ASM: Get_RandomAim (5172) — sets direction/target in Tank_Status[Counter].
     * Tier 1 (Respawn_Delay/4  < Seconds_Counter) → attack HQ ($B0).
     * Tier 2 (Respawn_Delay/8 >= Seconds_Counter) → random direction ($A0|rng&3).
     * Tier 3                                      → attack player 1 ($D0) or 2 ($C0). */
    uint8_t slot = Counter;
    uint8_t delay_4 = (uint8_t)(Respawn_Delay >> 2u);
    if (delay_4 < Seconds_Counter) {
        Temp = 0xB0u;
        rise_tank_status_bit(slot);
        return;
    }
    uint8_t delay_8 = (uint8_t)(delay_4 >> 1u);
    if (delay_8 >= Seconds_Counter) {
        Tank_Status[slot] = (uint8_t)((get_random_a() & 3u) | 0xA0u);
        return;
    }
    uint8_t a;
    if (Tank_Status[0] == 0u) {
        a = 0xC0u;
    } else if ((slot & 1u) == 0u) {
        a = 0xD0u;
    } else if (Tank_Status[1] == 0u) {
        a = 0xD0u;
    } else {
        a = 0xC0u;
    }
    Temp = a;
    rise_tank_status_bit(slot);
}

uint8_t relation_to_byte(uint8_t a) {
    if (a == 0) {
        return 0;
    }
    if (a & 0x80) {
        return 0xFF;
    }
    return 1;
}

