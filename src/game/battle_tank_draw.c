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



void tanks_status_handle(void) {
    Spr_Attrib = 0x20u;
    for (Counter = 0; Counter < 8; Counter++) {
        single_tank_status_handle(Counter);
    }
}

void single_tank_status_handle(uint8_t slot) {
    uint8_t status = Tank_Status[slot];
    uint8_t index = (status >> 3) & 0xFE;
    TankDraw_JumpTable[index >> 1](slot);
}

/* ASM: TankType_Pal — immediately after OperatingTank */
static const uint8_t TankType_Pal[8] = {2, 0, 0, 1, 2, 1, 2, 2};

/* ASM: Draw_Ricochet (5283) — draws 16x16 effect tile by direct offset. */
static void draw_ricochet(uint8_t tile_offset) {
    Spr_TileIndex = (uint8_t)(tile_offset + 0xF1u);
    TSA_Pal = 3u;
    draw_whole_spr();
}

void draw_kill_points(uint8_t enemy_slot) {
    Spr_Attrib = 0u;
    if (Tank_Type[enemy_slot] == 0u) {
        Temp_Y = Tank_Y[enemy_slot];
        Temp_X = Tank_X[enemy_slot];
        draw_ricochet(0u); /* Draw_PlayerKill path in ASM */
    } else {
        uint8_t tile = Tank_Type[enemy_slot];
        tile >>= 3u;
        tile &= 0xFCu;
        tile = (uint8_t)(tile - 0x10u + 0xB9u);
        Spr_TileIndex = tile;
        TSA_Pal = 3u;
        Temp_Y = Tank_Y[enemy_slot];
        Temp_X = Tank_X[enemy_slot];
        draw_whole_spr();
    }
    Spr_Attrib = 0x20u;
}

void draw_small_explode1(uint8_t slot) {
    Spr_Attrib = 0u;
    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    draw_ricochet(8u);
    Spr_Attrib = 0x20u;
}

void draw_small_explode2(uint8_t slot) {
    Spr_Attrib = 0u;
    Temp_Y = Tank_Y[slot];
    Temp_X = Tank_X[slot];
    draw_bullet_ricochet(Tank_Status[slot]);
    Spr_Attrib = 0x20u;
}

void draw_big_explode(uint8_t slot) {
    Counter = slot; /* ASM Set_SprIndex reads tank index from Counter. */
    TSA_Pal = 3u;
    Spr_Attrib = 0u;

    set_spr_index(0u);
    Temp_X = (uint8_t)(Temp_X - 8u);
    Temp_Y = (uint8_t)(Temp_Y - 8u);
    draw_whole_spr();

    set_spr_index(1u);
    Temp_X = (uint8_t)(Temp_X + 8u);
    Temp_Y = (uint8_t)(Temp_Y - 8u);
    draw_whole_spr();

    set_spr_index(2u);
    Temp_X = (uint8_t)(Temp_X - 8u);
    Temp_Y = (uint8_t)(Temp_Y + 8u);
    draw_whole_spr();

    set_spr_index(3u);
    Temp_X = (uint8_t)(Temp_X + 8u);
    Temp_Y = (uint8_t)(Temp_Y + 8u);
    draw_whole_spr();

    Spr_Attrib = 0x20u;
}

void set_spr_index(uint8_t value) {
    uint8_t tile_offset = (uint8_t)((value << 2u) + 0xD1u);
    Temp = tile_offset;
    uint8_t status_bits = Tank_Status[Counter] & 0xF0u;
    status_bits = (uint8_t)(status_bits - 0x30u);
    status_bits ^= 0x10u;
    Spr_TileIndex = (uint8_t)(status_bits + Temp);
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
