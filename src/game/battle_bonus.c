#include "battle_bonus.h"
#include "zeropage.h"
#include "bss.h"
#include "battle_hq.h"
#include "score.h"
#include "draw.h"
#include "random.h"

static void (*const Bonus_JumpTable[])(void) = {
    bonus_helmet,
    bonus_watch,
    bonus_shovel,
    bonus_star,
    bonus_grenade,
    bonus_life,
    bonus_pistol,
};


/* ASM: Bonus_Appear_Handle (7012) */
void bonus_appear_handle(void) {
    /* BonusNumber_ROM_Array (ASM:7046): 6,7 заменены на 4,3 для большей частоты бонусов */
    static const uint8_t BonusNumber_ROM_Array[8] = {0, 1, 2, 3, 4, 5, 4, 3};

    /* LDA #1; STA Snd_BonusAppears */
    Snd_BonusAppears = 1u;

at_:
    /* JSR Get_Random_A; AND #3; JSR Multiply_Bonus_Coord; STA Bonus_X */
    Bonus_X = multiply_bonus_coord((uint8_t)(get_random_a() & 3u));
    /* JSR Get_Random_A; AND #3; JSR Multiply_Bonus_Coord; STA Bonus_Y */
    Bonus_Y = multiply_bonus_coord((uint8_t)(get_random_a() & 3u));
    /* LDA #$FF; STA Bonus_Number */
    Bonus_Number = 0xFFu;
    /* LDA #0; STA BonusPts_TimeCounter */
    BonusPts_TimeCounter = 0u;
    /* JSR Bonus_Handle */
    bonus_handle();
    /* LDA BonusPts_TimeCounter; BNE @_ */
    if (BonusPts_TimeCounter != 0u) goto at_;
    /* JSR Get_Random_A; AND #7; TAY; LDA BonusNumber_ROM_Array,Y */
    Bonus_Number = BonusNumber_ROM_Array[get_random_a() & 7u];
    /* LDA #0; STA BonusPts_TimeCounter */
    BonusPts_TimeCounter = 0u;
    /* LDX Counter; LDY Counter2 — восстановление регистров; в C не требуется */
}


/* ASM: Bonus_Draw (5947) — все внутренние метки сохранены как goto-метки */
void bonus_draw(void) {
    /* LDA Bonus_X; BEQ End_Bonus_Draw */
    if (Bonus_X == 0u) goto End_Bonus_Draw;

    /* LDA BonusPts_TimeCounter; BEQ Bonus_NotTaken */
    if (BonusPts_TimeCounter == 0u) goto Bonus_NotTaken;

    /* DEC BonusPts_TimeCounter; BNE NotZeroCounter */
    BonusPts_TimeCounter--;
    if (BonusPts_TimeCounter != 0u) goto NotZeroCounter;

    /* LDA #0; STA Bonus_X; JMP End_Bonus_Draw */
    Bonus_X = 0u;
    goto End_Bonus_Draw;

NotZeroCounter:
    TSA_Pal = 2u;          /* points use sprite palette 2 */
    Spr_TileIndex = 0x3Bu; /* bonus points "500" tile */
    goto Draw_Bonus;

Bonus_NotTaken:
    /* LDA Frame_Counter; AND #8; BEQ End_Bonus_Draw */
    if ((Frame_Counter & 8u) == 0u) goto End_Bonus_Draw;
    TSA_Pal = 2u;
    /* LDA Bonus_Number; ASL; ASL; CLC; ADC #$81 */
    Spr_TileIndex = (uint8_t)(((uint8_t)(Bonus_Number << 2)) + 0x81u);
    /* fallthrough → Draw_Bonus */

Draw_Bonus:
    /* LDX Bonus_X; LDY Bonus_Y → Draw_WholeSpr uses Temp_X/Temp_Y in this port */
    Temp_X = Bonus_X;
    Temp_Y = Bonus_Y;
    Spr_Attrib = 0u;
    draw_whole_spr();
    Spr_Attrib = 0x20u;

End_Bonus_Draw:
    return;
}

uint8_t multiply_bonus_coord(uint8_t a) {
    /* ASM: Multiply_Bonus_Coord (7051): ((a*6)+6)*8 (all 8-bit) */
    uint8_t temp = a;
    a = (uint8_t)(a + a);       /* a*2 */
    a = (uint8_t)(a + temp);    /* a*3 */
    a = (uint8_t)(a + a);       /* a*6 */
    a = (uint8_t)(a + 6u);      /* a*6+6 */
    a = (uint8_t)(a + a);       /* *2 */
    a = (uint8_t)(a + a);       /* *4 */
    a = (uint8_t)(a + a);       /* *8 */
    return a;
}
/* ASM: Bonus_Handle (7138) */
void bonus_handle(void) {
    uint8_t status;
    uint8_t dx;
    uint8_t dy;

    /* LDA Bonus_X; BEQ @exit */
    if (Bonus_X == 0u) goto exit;
    /* LDA BonusPts_TimeCounter; BNE @exit */
    if (BonusPts_TimeCounter != 0u) goto exit;
    /* LDA #1; STA Tank_Num — стартуем со 2-го игрока */
    Tank_Num = 1u;

loop:
    /* LDX Tank_Num; LDA Tank_Status,X; BPL @decNumber */
    status = Tank_Status[Tank_Num];
    if ((int8_t)status >= 0) goto decNumber;
    /* CMP #$E0; BCS @decNumber — exploding/respawning танки игнорируются */
    if (status >= 0xE0u) goto decNumber;

    /* LDA Tank_X,X; SEC; SBC Bonus_X; BPL @skip */
    dx = (uint8_t)(Tank_X[Tank_Num] - Bonus_X);
    if ((int8_t)dx >= 0) goto skip;
    /* EOR #$FF; CLC; ADC #1 — signed-abs через two's complement */
    dx = (uint8_t)((uint8_t)~dx + 1u);
skip:
    /* CMP #$C; BCS @decNumber */
    if (dx >= 0x0Cu) goto decNumber;

    /* LDA Tank_Y,X; SEC; SBC Bonus_Y; BPL @skip_2 */
    dy = (uint8_t)(Tank_Y[Tank_Num] - Bonus_Y);
    if ((int8_t)dy >= 0) goto skip_2;
    dy = (uint8_t)((uint8_t)~dy + 1u);
skip_2:
    /* CMP #$C; BCS @decNumber */
    if (dy >= 0x0Cu) goto decNumber;

    /* LDA #$32; STA BonusPts_TimeCounter — 50 кадров показа очков */
    BonusPts_TimeCounter = 0x32u;
    /* LDA Bonus_Number; BMI @exit — если Bonus_Number ещё $FF, ничего не делаем */
    if ((int8_t)Bonus_Number < 0) goto exit;
    /* LDA Level_Mode; CMP #2; BEQ @bonus_Command — demo-режим: не начисляем очки */
    if (Level_Mode == 2u) goto bonus_Command;
    /* LDA #$50; JSR Num_To_NumString — 500 очков за бонус */
    num_to_num_string(0x50u);
    /* LDX Tank_Num; JSR Add_Score */
    add_score(Tank_Num);
    /* JSR Add_Life */
    add_life(Tank_Num);
    /* LDX Tank_Num; LDA #1; STA Snd_BonusTaken */
    Snd_BonusTaken = 1u;

bonus_Command:
    /* LDA Bonus_Number; ASL; TAY; LDA Bonus_JumpTable,Y / +1,Y; PLA;PLA; JMP (LowPtr) */
    if (Bonus_Number < (uint8_t)(sizeof(Bonus_JumpTable) / sizeof(Bonus_JumpTable[0]))) {
        Bonus_JumpTable[Bonus_Number]();
    }
    goto exit;

decNumber:
    /* DEC Tank_Num; BPL @loop */
    Tank_Num--;
    if ((int8_t)Tank_Num >= 0) goto loop;

exit:
    return;
}
/* ASM: Bonus_Helmet (7220) */
void bonus_helmet(void) {
    /* LDA #10; STA Invisible_Timer,X — X = Tank_Num (игрок, поднявший бонус) */
    Invisible_Timer[Tank_Num] = 10u;
}

/* ASM: Bonus_Watch (7229) */
void bonus_watch(void) {
    /* LDA #10; STA EnemyFreeze_Timer */
    EnemyFreeze_Timer = 10u;
}

/* ASM: Bonus_Shovel (7238) */
void bonus_shovel(void) {
    /* LDA HQ_Status; BPL End_Bonus_Shovel */
    if ((int8_t)HQ_Status >= 0) goto End_Bonus_Shovel;
    /* JSR Draw_ArmourHQ */
    draw_armour_hq();
    /* LDA #20; STA HQArmour_Timer */
    HQArmour_Timer = 20u;

End_Bonus_Shovel:
    return;
}
/* ASM: Bonus_Star (7252) */
void bonus_star(void) {
    uint8_t type;
    /* LDA Player_Type,X — X = Tank_Num */
    type = Player_Type[Tank_Num];
    /* CMP #$60; BEQ End_Bonus_Star — максимальный тип, не апгрейдим */
    if (type == 0x60u) goto End_Bonus_Star;
    /* CLC; ADC #$20 — следующий тип */
    type = (uint8_t)(type + 0x20u);
    /* STA Player_Type,X */
    Player_Type[Tank_Num] = type;
    /* STA Tank_Type,X */
    Tank_Type[Tank_Num] = type;

End_Bonus_Star:
    return;
}

/* ASM: Bonus_Grenade (7268) */
void bonus_grenade(void) {
    uint8_t status;

    /* LDA #7; STA Counter — стартуем с последнего врага */
    Counter = 7u;
    /* LDA #1; STA Snd_EnemyExplode */
    Snd_EnemyExplode = 1u;

Bonus_Grenade_Loop:
    /* LDY Counter; LDA Tank_Status,Y; BPL Explode_Next */
    status = Tank_Status[Counter];
    if ((int8_t)status >= 0) goto Explode_Next;
    /* CMP #$E0; BCS Explode_Next */
    if (status >= 0xE0u) goto Explode_Next;
    /* LDA #$73; STA Tank_Status,Y */
    Tank_Status[Counter] = 0x73u;
    /* LDA #0; STA Tank_Type,Y */
    Tank_Type[Counter] = 0u;

Explode_Next:
    /* DEC Counter; LDA Counter; CMP #1; BNE Bonus_Grenade_Loop — игроков (0,1) не взрываем */
    Counter--;
    if (Counter != 1u) goto Bonus_Grenade_Loop;
}

/* ASM: Bonus_Life (7296) — fallthrough в Bonus_Pistol (RTS) */
void bonus_life(void) {
    /* INC Player1_Lives,X — Player1_Lives и Player2_Lives лежат в zp подряд,
     * поэтому индекс X=Tank_Num выбирает соответствующего игрока */
    if (Tank_Num == 0u) {
        Player1_Lives++;
    } else {
        Player2_Lives++;
    }
    /* LDA #1; STA Snd_Ancillary_Life1; STA Snd_Ancillary_Life2 */
    Snd_Ancillary_Life1 = 1u;
    Snd_Ancillary_Life2 = 1u;
    /* fallthrough в Bonus_Pistol (только RTS) */
}

/* ASM: Bonus_Pistol (7302) — Not used and does nothing */
void bonus_pistol(void) {
}
