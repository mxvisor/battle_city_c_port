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

SkipRiseBit_Explode_Handle:
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

/* ASM: Get_RandomStatus (4910). 50% — переходит в Get_RandomAim. Иначе с равной
 * вероятностью INC/DEC направления и установка статуса в $A0..$A3 (диспетч в
 * check_tile_reach на следующем тике). КРИТИЧНО: ORA #Tank_Status — это
 * `ORA #$A0` (zp-адрес Tank_Status = $A0), а не сохранение старших бит. */
void get_random_status(uint8_t slot) {
    uint8_t a;
    /* JSR Get_Random_A; AND #1; BEQ End_Get_RandomStatus */
    if ((get_random_a() & 1u) == 0u) goto End_Get_RandomStatus;
    /* JSR Get_Random_A; AND #1; BEQ Sbc_Get_RandomStatus */
    if ((get_random_a() & 1u) == 0u) goto Sbc_Get_RandomStatus;
    /* LDA Tank_Status,X; CLC; ADC #1 — direction++ */
    a = (uint8_t)(Tank_Status[slot] + 1u);
    /* JMP Save_Get_RandomStatus */
    goto Save_Get_RandomStatus;

Sbc_Get_RandomStatus:
    /* LDA Tank_Status,X; SEC; SBC #1 — direction-- */
    a = (uint8_t)(Tank_Status[slot] - 1u);

Save_Get_RandomStatus:
    /* AND #3; ORA #Tank_Status (= $A0); STA Tank_Status,X */
    Tank_Status[slot] = (uint8_t)((a & 3u) | 0xA0u);
    return;

End_Get_RandomStatus:
    /* JSR Get_RandomAim */
    get_random_aim(slot);
}

void save_ai_to_status(uint8_t slot) {
    /* ASM: Save_AI_ToStatus (4999)
     * JSR Load_AI_Status; STA Tank_Status,X */
    Tank_Status[slot] = load_ai_status(slot);
}

/* ASM: Load_AI_Status (5008). Все внутренние ASM-метки сохранены. */
uint8_t load_ai_status(uint8_t slot) {
    uint8_t a;
    uint8_t y_index;

    /* LDA AI_X_Aim; SEC; SBC Tank_X,X — distance по X (carry от SBC даёт знак)
     * JSR Relation_To_Byte → 1 (lhs>rhs), 0 (equal), $FF (lhs<rhs); +1 → 2/1/0 */
    AI_X_DifferFlag = (uint8_t)(relation_to_byte(AI_X_Aim, Tank_X[slot]) + 1u);

    /* LDA AI_Y_Aim; SEC; SBC Tank_Y,X; JSR Relation_To_Byte; CLC; ADC #1; STA AI_Y_DifferFlag */
    AI_Y_DifferFlag = (uint8_t)(relation_to_byte(AI_Y_Aim, Tank_Y[slot]) + 1u);

    /* ASL A; CLC; ADC AI_Y_DifferFlag; CLC; ADC AI_X_DifferFlag; STA AI_X_DifferFlag
     * A = (Y*2) + Y + X = Y*3 + X — индекс в AI_Status, перезаписывается в AI_X_DifferFlag */
    a = (uint8_t)((AI_Y_DifferFlag << 1) + AI_Y_DifferFlag + AI_X_DifferFlag);
    AI_X_DifferFlag = a;

    /* CPX #2; BCS Load_AIStatus_GetRandom — для вражеских танков идём в случайную ветку */
    if (slot >= 2u) goto Load_AIStatus_GetRandom;
    /* TXA; ASL A; EOR Seconds_Counter; AND #2; BEQ checkDifferFlag */
    a = (uint8_t)((((uint8_t)(slot << 1)) ^ Seconds_Counter) & 2u);
    if (a == 0u) goto checkDifferFlag;
    /* JMP LoadSecondPart */
    goto LoadSecondPart;

Load_AIStatus_GetRandom:
    /* JSR Get_Random_A; AND #1; BEQ checkDifferFlag */
    if ((get_random_a() & 1u) == 0u) goto checkDifferFlag;
    /* fallthrough → LoadSecondPart */
    goto LoadSecondPart;

LoadSecondPart:
    /* LDA #9; CLC; ADC AI_X_DifferFlag; TAY; JMP End_Load_AIStatus */
    y_index = (uint8_t)(9u + AI_X_DifferFlag);
    goto End_Load_AIStatus;

checkDifferFlag:
    /* LDY AI_X_DifferFlag */
    y_index = AI_X_DifferFlag;
    /* fallthrough */

End_Load_AIStatus:
    /* LDA AI_Status,Y; RTS */
    return AI_Status[y_index];
}

/* ASM: Compare_Block_X (4953). CMP Block_X; BCC @_; SEC; SBC #1; @_: RTS.
 * Возвращает a-1 если a >= Block_X, иначе a. */
uint8_t compare_block_x(uint8_t a, uint8_t b) {
    /* CMP Block_X; BCC @_ */
    if (a < b) goto at_;
    /* SEC; SBC #1 */
    a = (uint8_t)(a - 1u);
at_: /* ASM: @_ */
    return a;
}

/* ASM: Compare_Block_Y (4966) — аналогично для Block_Y. */
uint8_t compare_block_y(uint8_t a, uint8_t b) {
    if (a < b) goto at_;
    a = (uint8_t)(a - 1u);
at_: /* ASM: @_ */
    return a;
}

/* ASM: Check_Obj (4809). Проверяет ОБА передних угла танка по направлению
 * движения. Угол 1: (Block + dx8 + dy8). Угол 2: (Block + dx8 - dy8,
 * Block + dy8 - dx8). Если оба тайла проходимы (==0 или >= $20, без bit 7) —
 * двигаем танк. Если хотя бы один блокирующий — переходим в GetRnd_CheckObj
 * (для врагов случайно: либо сменить направление, либо застрять). */
void check_obj(uint8_t slot) {
    /* LDA Tank_Status,X; AND #3; TAY */
    uint8_t dir = (uint8_t)(Tank_Status[slot] & 3u);
    static const int8_t bullet_x_inc[4] = { 0, -1, 0, 1 };
    static const int8_t bullet_y_inc[4] = { -1, 0, 1, 0 };
    int8_t dx = bullet_x_inc[dir];
    int8_t dy = bullet_y_inc[dir];
    /* ASL×3 — *8 */
    int8_t dx8 = (int8_t)(dx << 3);
    int8_t dy8 = (int8_t)(dy << 3);
    Tmp_Status1 = (uint8_t)dx8;
    Tmp_Status2 = (uint8_t)dy8;
    /* ADC Tank_Y,X; STA Block_Y; ADC Tank_X,X; STA Block_X */
    Block_X = (uint8_t)(Tank_X[slot] + (uint8_t)dx);
    Block_Y = (uint8_t)(Tank_Y[slot] + (uint8_t)dy);

    uint8_t check_x, check_y, tile;

    /* Первый угол: Block_? + Tmp_Status1 + Tmp_Status2 */
    check_x = compare_block_x((uint8_t)(Block_X + (uint8_t)dx8 + (uint8_t)dy8), Block_X);
    check_y = compare_block_y((uint8_t)(Block_Y + (uint8_t)dx8 + (uint8_t)dy8), Block_Y);
    get_coord_in_tiles_xy(check_x, check_y);
    tile = NT_Buffer[(LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8)) & 0x3FFu];
    /* LDA (LowPtr_Byte),Y; BMI GetRnd_CheckObj */
    if ((tile & 0x80u) != 0u) goto GetRnd_CheckObj;
    /* BEQ CheckX_Check_Obj — tile==0: пропустить CMP #$20 (иначе BCC ошибочно) */
    if (tile == 0u) goto CheckX_Check_Obj;
    /* CMP #$20; BCC GetRnd_CheckObj */
    if (tile < 0x20u) goto GetRnd_CheckObj;
    /* fallthrough в CheckX_Check_Obj */

CheckX_Check_Obj:
    /* Второй угол: Block_X + Tmp_Status1 - Tmp_Status2, Block_Y + Tmp_Status2 - Tmp_Status1 */
    check_x = compare_block_x((uint8_t)(Block_X + (uint8_t)dx8 - (uint8_t)dy8), Block_X);
    check_y = compare_block_y((uint8_t)(Block_Y + (uint8_t)dy8 - (uint8_t)dx8), Block_Y);
    get_coord_in_tiles_xy(check_x, check_y);
    tile = NT_Buffer[(LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8)) & 0x3FFu];
    if ((tile & 0x80u) != 0u) goto GetRnd_CheckObj;
    if (tile == 0u) goto SaveCoord_Check_Obj;
    if (tile < 0x20u) goto GetRnd_CheckObj;
    /* fallthrough в SaveCoord_Check_Obj */

SaveCoord_Check_Obj:
    /* STA Tank_X,X; STA Tank_Y,X */
    Tank_X[slot] = Block_X;
    Tank_Y[slot] = Block_Y;
    goto TrackHandle_CheckObj;

GetRnd_CheckObj:
    /* CPX #2; BCC TrackHandle_CheckObj — игроки не получают AI-смену направления */
    if (slot < 2u) goto TrackHandle_CheckObj;
    /* JSR Get_Random_A; AND #3; BEQ CheckTile_Check_Obj */
    if ((get_random_a() & 3u) == 0u) goto CheckTile_Check_Obj;
    /* LDA #$80; JSR Rise_TankStatus_Bit; LDA #8; ORA Tank_Status,X */
    Temp = 0x80u;
    rise_tank_status_bit(slot);
    Tank_Status[slot] = (uint8_t)(Tank_Status[slot] | 0x08u);
    /* fallthrough в TrackHandle_CheckObj */

TrackHandle_CheckObj:
    /* LDA Track_Pos,X; EOR #4; STA Track_Pos,X */
    Track_Pos[slot] = (uint8_t)(Track_Pos[slot] ^ 4u);
    return;

CheckTile_Check_Obj:
    /* LDA Tank_X,X; AND #7; BNE Change_Direction */
    if ((Tank_X[slot] & 7u) != 0u) goto Change_Direction_Check_Obj;
    /* LDA Tank_Y,X; AND #7; BNE Change_Direction */
    if ((Tank_Y[slot] & 7u) != 0u) goto Change_Direction_Check_Obj;
    /* LDA #$90; JSR Rise_TankStatus_Bit */
    Temp = 0x90u;
    rise_tank_status_bit(slot);
    /* fallthrough */

Change_Direction_Check_Obj:
    /* LDA #2; EOR Tank_Status,X; STA Tank_Status,X — флип бита 1 направления */
    Tank_Status[slot] = (uint8_t)(Tank_Status[slot] ^ 2u);
}



/* ASM: Check_TileReach (4794). Враг движется до препятствия; на границе тайла
 * с малой вероятностью (1/16) меняет цель. В ASM `BCC Check_Obj` —
 * branch-to-function: после `Check_Obj` RTS возвращает в caller текущей функции.
 * В C — прямые вызовы `check_obj(slot); return;`. */
void check_tile_reach(uint8_t slot) {
    /* CPX #2; BCC Check_Obj — игроки сразу в Check_Obj */
    if (slot < 2u) { check_obj(slot); return; }
    /* LDA Tank_X,X; AND #7; BNE Check_Obj */
    if ((Tank_X[slot] & 7u) != 0u) { check_obj(slot); return; }
    /* LDA Tank_Y,X; AND #7; BNE Check_Obj */
    if ((Tank_Y[slot] & 7u) != 0u) { check_obj(slot); return; }
    /* JSR Get_Random_A; AND #$F; BNE Check_Obj */
    if ((get_random_a() & 0x0Fu) != 0u) { check_obj(slot); return; }
    /* JSR Get_RandomAim */
    get_random_aim(slot);
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


/* ASM: Get_RandomAim (5172). Tier 1 (Respawn_Delay/4 < Seconds_Counter) →
 * HQ-атака ($B0). Tier 2 (delay/8 >= sec) → случайное направление ($A0|rng&3).
 * Tier 3 — атака 1P ($D0) или 2P ($C0) в зависимости от живости игроков. */
void get_random_aim(uint8_t slot) {
    uint8_t a;
    /* LDA Respawn_Delay; LSR; LSR — A = delay/4 */
    a = (uint8_t)(Respawn_Delay >> 2u);
    /* CMP Seconds_Counter; BCS @____ */
    if (a >= Seconds_Counter) goto at____;
    /* LDA #$B0; JMP End_Get_RandomDirection */
    a = 0xB0u;
    goto End_Get_RandomDirection;

at____: /* ASM: @____ */
    /* LSR A — A = delay/8 */
    a = (uint8_t)(a >> 1u);
    /* CMP Seconds_Counter; BCC @_ */
    if (a < Seconds_Counter) goto at_;
    /* JSR Get_Random_A; AND #3; ORA #$A0; STA Tank_Status,X */
    Tank_Status[slot] = (uint8_t)((get_random_a() & 3u) | 0xA0u);
    return;

at_: /* ASM: @_ */
    /* LDA Tank_Status; BEQ @__ — если 1P мёртв, атакуем 2P */
    if (Tank_Status[0] == 0u) goto at__;
    /* TXA; AND #1; BEQ @___ — чётные слоты атакуют 1P */
    if ((slot & 1u) == 0u) goto at___;
    /* LDA Tank_Status+1; BEQ @___ — если 2P мёртв, атакуем 1P */
    if (Tank_Status[1] == 0u) goto at___;

at__: /* ASM: @__ — атака 2P */
    /* LDA #$C0; JMP End_Get_RandomDirection */
    a = 0xC0u;
    goto End_Get_RandomDirection;

at___: /* ASM: @___ — атака 1P */
    /* LDA #$D0 */
    a = 0xD0u;

End_Get_RandomDirection:
    /* JSR Rise_TankStatus_Bit */
    Temp = a;
    rise_tank_status_bit(slot);
}

/* ASM: Relation_To_Byte (4471). В C флаги Z/C от ASM SBC не сохраняются между
 * функциями — передаём оба операнда. Возвращает 0 при равенстве, 1 при lhs>=rhs,
 * $FF при lhs<rhs. */
uint8_t relation_to_byte(uint8_t lhs, uint8_t rhs) {
    uint8_t a;
    /* BEQ End_RelationToByte — Z после SBC означает равенство */
    if (lhs == rhs) {
        a = 0u; /* (A в ASM уже == 0 после SBC) */
        goto End_RelationToByte;
    }
    /* BCS @_ — Carry set после SBC означает lhs >= rhs */
    if (lhs > rhs) goto at_;
    /* LDA #$FF */
    a = 0xFFu;
    /* JMP End_RelationToByte */
    goto End_RelationToByte;

at_: /* ASM: @_ */
    /* LDA #1 */
    a = 1u;

End_RelationToByte:
    return a;
}

