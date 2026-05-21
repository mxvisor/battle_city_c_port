#include "battle_tank_draw.h"
#include "battle_bullet.h"
#include "zeropage.h"
#include "draw.h"

typedef void (*TankFunc)(uint8_t);

/* placeholder */
static void status_noop(uint8_t slot) {
    (void)slot;
}

static const TankFunc TankDraw_JumpTable[] = {
    status_noop,         // 0
    draw_kill_points,    // 2
    draw_small_explode1, // 4
    draw_big_explode,    // 6
    draw_big_explode,    // 8
    draw_small_explode2, // 10
    draw_small_explode2, // 12
    draw_small_explode2, // 14
    operating_tank,      // 16
    operating_tank,      // 18
    operating_tank,      // 20
    operating_tank,      // 22
    operating_tank,      // 24
    operating_tank,      // 26
    respawn,             // 28
    respawn              // 30
};



/* ASM: TanksStatus_Handle (5215) — обрабатывает 8 танков */
void tanks_status_handle(void) {
    /* LDA #0; STA Counter; @_: LDX Counter; JSR SingleTankStatus_Handle; INC Counter; CMP #8; BNE @_ */
    for (Counter = 0u; Counter < 8u; Counter++) {
        single_tank_status_handle(Counter);
    }
}

/* ASM: SingleTankStatus_Handle (5233) — диспетч по высоким битам Tank_Status */
void single_tank_status_handle(uint8_t slot) {
    /* LDA Tank_Status,X; LSR;LSR;LSR; AND #$FE; TAY — Y = байтовый offset (2-byte ASM-pointers) */
    uint8_t y = (uint8_t)((Tank_Status[slot] >> 3u) & 0xFEu);
    /* LDA TankDraw_JumpTable,Y / +1,Y; JMP (LowPtr) — в C индекс = Y/2 */
    uint8_t idx = (uint8_t)(y >> 1u);
    if (idx < (uint8_t)(sizeof(TankDraw_JumpTable) / sizeof(TankDraw_JumpTable[0]))) {
        TankDraw_JumpTable[idx](slot);
    }
}

/* ASM: TankType_Pal — immediately after OperatingTank */
static const uint8_t TankType_Pal[8] = {2, 0, 0, 1, 2, 1, 2, 2};

/* ASM: Draw_Ricochet (5283) — fallthrough-точка из Draw_Bullet_Ricochet.
 * CLC; ADC #$F1; STA Spr_TileIndex; LDA #3; STA TSA_Pal; JSR Draw_WholeSpr */
static void draw_ricochet(uint8_t tile_offset) {
    Spr_TileIndex = (uint8_t)(tile_offset + 0xF1u);
    TSA_Pal = 3u;
    draw_whole_spr();
}

/* ASM: Draw_Small_Explode2 (5250) — explode для bullets-stage-2 */
void draw_small_explode2(uint8_t slot) {
    /* LDA #0; STA Spr_Attrib */
    Spr_Attrib = 0u;
    /* LDA Tank_Status,X; PHA — saved для Draw_Bullet_Ricochet */
    /* LDY Tank_Y,X; LDA Tank_X,X; TAX — X=Tank_X, Y=Tank_Y (через Temp_X/Y в C) */
    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    /* PLA; JSR Draw_Bullet_Ricochet — A = saved Tank_Status */
    draw_bullet_ricochet(Tank_Status[slot]);
    /* LDA #$20; STA Spr_Attrib */
    Spr_Attrib = 0x20u;
}

/* ASM: Draw_Bullet_Ricochet (5269). Fallthrough в Draw_Ricochet. */
void draw_bullet_ricochet(uint8_t a_val) {
    /* LSR A ×4 — A = a_val >> 4 */
    uint8_t a = (uint8_t)(a_val >> 4u);
    /* SEC; SBC #7 */
    a = (uint8_t)(a - 7u);
    /* EOR #$FF; CLC; ADC #1 — negate в 8-bit (~a + 1) */
    a = (uint8_t)((uint8_t)(~a) + 1u);
    /* ASL A; ASL A — *4 (tile offset) */
    a = (uint8_t)(a << 2u);
    /* fallthrough → Draw_Ricochet */
    draw_ricochet(a);
}

/* ASM: Draw_Kill_Points (5296). Все ASM-метки сохранены как goto-цели. */
void draw_kill_points(uint8_t enemy_slot) {
    uint8_t tile;
    /* LDA #0; STA Spr_Attrib */
    Spr_Attrib = 0u;
    /* LDA Tank_Type,X; BEQ Draw_PlayerKill */
    if (Tank_Type[enemy_slot] == 0u) goto Draw_PlayerKill;
    /* LDA Tank_Type,X; LSR;LSR;LSR; AND #$FC */
    tile = (uint8_t)((Tank_Type[enemy_slot] >> 3u) & 0xFCu);
    /* SEC; SBC #$10; CLC; ADC #$B9 → tile + ($B9 - $10) = tile + $A9 */
    tile = (uint8_t)(tile - 0x10u + 0xB9u);
    /* STA Spr_TileIndex; LDA #3; STA TSA_Pal */
    Spr_TileIndex = tile;
    TSA_Pal = 3u;
    /* LDY Tank_Y,X; LDA Tank_X,X; TAX */
    Temp_Y = Tank_Y[enemy_slot];
    Temp_X = Tank_X[enemy_slot];
    /* JSR Draw_WholeSpr; JMP Draw_Kill_Points_Skip */
    draw_whole_spr();
    goto Draw_Kill_Points_Skip;

Draw_PlayerKill:
    /* LDA Tank_Y,X; TAY; LDA Tank_X,X; TAX; LDA #0; JSR Draw_Ricochet */
    Temp_Y = Tank_Y[enemy_slot];
    Temp_X = Tank_X[enemy_slot];
    draw_ricochet(0u);
    /* fallthrough → Draw_Kill_Points_Skip */

Draw_Kill_Points_Skip:
    /* LDA #$20; STA Spr_Attrib */
    Spr_Attrib = 0x20u;
}

/* ASM: Draw_Small_Explode1 (5336) */
void draw_small_explode1(uint8_t slot) {
    /* LDA #0; STA Spr_Attrib */
    Spr_Attrib = 0u;
    /* LDY Tank_Y,X; LDA Tank_X,X; TAX */
    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    /* LDA #8; JSR Draw_Ricochet */
    draw_ricochet(8u);
    /* LDA #$20; STA Spr_Attrib */
    Spr_Attrib = 0x20u;
}

/* ASM: Draw_Big_Explode (5352) — рисует 32x32 взрыв как 4 квадранта.
 * Set_SprIndex использует Counter как slot-индекс и устанавливает Spr_TileIndex
 * + Temp_X/Y из Tank_X/Y[Counter] (вместо ASM CPU X/Y регистров). */
void draw_big_explode(uint8_t slot) {
    /* В C нет аналога CPU X/Y регистров; Set_SprIndex читает slot из Counter */
    Counter = slot;
    /* LDA #3; STA TSA_Pal */
    TSA_Pal = 3u;
    /* LDA #0; STA Spr_Attrib */
    Spr_Attrib = 0u;

    /* JSR Set_SprIndex (A=0); TXA;SEC;SBC #8;TAX; TYA;SEC;SBC #8;TAY; JSR Draw_WholeSpr */
    set_spr_index(0u);
    Temp_X = (uint8_t)(Temp_X - 8u);
    Temp_Y = (uint8_t)(Temp_Y - 8u);
    draw_whole_spr();

    /* LDA #1; JSR Set_SprIndex; X+=8, Y-=8 */
    set_spr_index(1u);
    Temp_X = (uint8_t)(Temp_X + 8u);
    Temp_Y = (uint8_t)(Temp_Y - 8u);
    draw_whole_spr();

    /* LDA #2; JSR Set_SprIndex; X-=8, Y+=8 */
    set_spr_index(2u);
    Temp_X = (uint8_t)(Temp_X - 8u);
    Temp_Y = (uint8_t)(Temp_Y + 8u);
    draw_whole_spr();

    /* LDA #3; JSR Set_SprIndex; X+=8, Y+=8 */
    set_spr_index(3u);
    Temp_X = (uint8_t)(Temp_X + 8u);
    Temp_Y = (uint8_t)(Temp_Y + 8u);
    draw_whole_spr();

    /* LDA #$20; STA Spr_Attrib */
    Spr_Attrib = 0x20u;
}

/* ASM: Set_SprIndex (5408). Принимает value=0..3 (квадрант), читает slot из Counter. */
void set_spr_index(uint8_t value) {
    /* LDX Counter — slot для Tank_Status/Tank_X/Tank_Y */
    /* ASL; ASL; CLC; ADC #$D1; STA Temp — Temp = value*4 + $D1 */
    Temp = (uint8_t)((value << 2u) + 0xD1u);
    /* LDA Tank_Status,X; AND #$F0; SEC; SBC #$30; EOR #$10 — извлекает direction-биты */
    uint8_t bits = (uint8_t)(Tank_Status[Counter] & 0xF0u);
    bits = (uint8_t)(bits - 0x30u);
    bits ^= 0x10u;
    /* CLC; ADC Temp; STA Spr_TileIndex */
    Spr_TileIndex = (uint8_t)(bits + Temp);
    /* LDY Tank_Y,X; LDA Tank_X,X; TAX — выход через Temp_X/Y */
    Temp_Y = Tank_Y[Counter];
    Temp_X = Tank_X[Counter];
}

void operating_tank(uint8_t slot) {
    if (slot < 2u) {
        uint8_t blink = Player_Blink_Timer[slot];
        if (blink != 0u) {
            if ((Frame_Counter & 8u) != 0u) {
                return;
            }
        }
        TSA_Pal = slot;
        goto OperTank_Draw;
    }

    {
        uint8_t type = Tank_Type[slot] & 4u;
        if (type != 0u) {
            uint8_t pal = (uint8_t)(((Frame_Counter >> 3u) & 1u) + 2u);
            TSA_Pal = pal;
            goto OperTank_Draw;
        }
    }

    {
        uint8_t pal_index = (uint8_t)(((Frame_Counter << 2u) + Tank_Type[slot]) & 7u);
        TSA_Pal = TankType_Pal[pal_index];
    }

OperTank_Draw:
    Spr_TileIndex = (uint8_t)((Tank_Type[slot] & 0xF0u) + Track_Pos[slot]);
    uint8_t direction = Tank_Status[slot] & 3u;
    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    spr_tile_index_add(direction);
    draw_whole_spr();
}

/* ASM: Respawn (5491) */
void respawn(uint8_t slot) {
    uint8_t a = (uint8_t)((Tank_Status[slot] & 0x0Fu) - 7u);
    if ((int8_t)a >= 0) {
        goto skip;
    }
    a = (uint8_t)(~a + 1u); /* negate: EOR #$FF; ADC #1 */

skip: /* ASM: @skip */
    Spr_TileIndex = (uint8_t)(((a << 1u) & 0xFCu) + 0xA1u);
    TSA_Pal = 3;

    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    draw_whole_spr();
}
