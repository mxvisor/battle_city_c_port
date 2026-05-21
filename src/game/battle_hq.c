#include "battle_hq.h"
#include "zeropage.h"
#include "draw.h"
#include "bss.h"

static const uint8_t Normal_HQ_TSA[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF };
static const uint8_t NormalLine2[] = { 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF };
static const uint8_t NormalLine3[] = { 0x00, 0x0F, 0xC8, 0xCA, 0x0F, 0x00, 0xFF };
static const uint8_t Normalline4[] = { 0x00, 0x0F, 0xC9, 0xCB, 0x0F, 0x00, 0xFF };

static const uint8_t Armour_HQ_TSA_Line1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF };
static const uint8_t Armour_HQ_TSA_Line2[] = { 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0xFF };
static const uint8_t Armour_HQ_TSA_Line3[] = { 0x00, 0x10, 0xC8, 0xCA, 0x10, 0x00, 0xFF };
static const uint8_t Armour_HQ_TSA_Line4[] = { 0x00, 0x10, 0xC9, 0xCB, 0x10, 0x00, 0xFF };

static const uint8_t Naked_HQ_TSA_FirstLine[] = { 0xC8, 0xCA, 0xFF };
static const uint8_t Naked_HQ_TSA_SecndLine[] = { 0xC9, 0xCB, 0xFF };

static const uint8_t DestroyedHQ_TSA_Line1[] = { 0xCC, 0xCE, 0xFF };
static const uint8_t DestroyedHQ_TSA_Line2[] = { 0xCD, 0xCF, 0xFF };

void draw_normal_hq(void) {
    string_to_screen_buffer(0x0C, 0x18, Normal_HQ_TSA);
    string_to_screen_buffer(0x0C, 0x19, NormalLine2);
    string_to_screen_buffer(0x0C, 0x1A, NormalLine3);
    string_to_screen_buffer(0x0C, 0x1B, Normalline4);

    uint8_t x = ScrBuffer_Pos;
    Screen_Buffer[x++] = 0x23;
    Screen_Buffer[x++] = 0xF3;
    NT_Buffer[0x3F3] = 0x00;
    Screen_Buffer[x++] = NT_Buffer[0x3F3];
    uint8_t attr = NT_Buffer[0x3F4] & 0xCC;
    NT_Buffer[0x3F4] = attr;
    Screen_Buffer[x++] = attr;
    Screen_Buffer[x++] = 0xFF;
    ScrBuffer_Pos = x;
}

void draw_naked_hq(void) {
    string_to_screen_buffer(0x0E, 0x1A, Naked_HQ_TSA_FirstLine);
    string_to_screen_buffer(0x0E, 0x1B, Naked_HQ_TSA_SecndLine);

    uint8_t x = ScrBuffer_Pos;
    Screen_Buffer[x++] = 0x23;
    Screen_Buffer[x++] = 0xF3;
    uint8_t attr = NT_Buffer[0x3F3] & 0x3F;
    NT_Buffer[0x3F3] = attr;
    Screen_Buffer[x++] = attr;
    Screen_Buffer[x++] = 0xFF;
    ScrBuffer_Pos = x;
}

void draw_armour_hq(void) {
    string_to_screen_buffer(0x0C, 0x18, Armour_HQ_TSA_Line1);
    string_to_screen_buffer(0x0C, 0x19, Armour_HQ_TSA_Line2);
    string_to_screen_buffer(0x0C, 0x1A, Armour_HQ_TSA_Line3);
    string_to_screen_buffer(0x0C, 0x1B, Armour_HQ_TSA_Line4);

    uint8_t x = ScrBuffer_Pos;
    Screen_Buffer[x++] = 0x23;
    Screen_Buffer[x++] = 0xF3;
    NT_Buffer[0x3F3] = 0x3F;
    Screen_Buffer[x++] = 0x3F;
    uint8_t attr = (NT_Buffer[0x3F4] & 0xCC) | 0x33;
    NT_Buffer[0x3F4] = attr;
    Screen_Buffer[x++] = attr;
    Screen_Buffer[x++] = 0xFF;
    ScrBuffer_Pos = x;
}

void draw_destroyed_hq(void) {
    string_to_screen_buffer(0x0E, 0x1A, DestroyedHQ_TSA_Line1);
    string_to_screen_buffer(0x0E, 0x1B, DestroyedHQ_TSA_Line2);
}

void draw_small_explode(uint8_t tile) {
    Spr_TileIndex = tile;
    draw_whole_spr();
}

void add_explode_spr_base(uint8_t delta) {
    uint8_t tile = delta + HQExplode_SprBase;
    draw_small_explode(tile);
}

/* ASM: Draw_BigExplode (6162). HQExplode_SprBase устанавливается caller'ом
 * (FourthExplode_Pic ставит 0, FifthExplode_Pic ставит $10) до вызова. */
void draw_hq_big_explode(void) {
    /* LDX #$70; LDY #$D0; LDA #$D1; JSR Add_ExplodeSprBase */
    Temp_X = 0x70u;
    Temp_Y = 0xD0u;
    add_explode_spr_base(0xD1u);
    /* LDX #$80; LDY #$D0; LDA #$D5; JSR Add_ExplodeSprBase */
    Temp_X = 0x80u;
    Temp_Y = 0xD0u;
    add_explode_spr_base(0xD5u);
    /* LDX #$70; LDY #$E0; LDA #$D9; JSR Add_ExplodeSprBase */
    Temp_X = 0x70u;
    Temp_Y = 0xE0u;
    add_explode_spr_base(0xD9u);
    /* LDX #$80; LDY #$E0; LDA #$DD; JSR Add_ExplodeSprBase */
    Temp_X = 0x80u;
    Temp_Y = 0xE0u;
    add_explode_spr_base(0xDDu);
}

/* ASM: End_Ice_Move (HQExplode_JumpTable entry) */
void end_ice_move(void) {
    /* Intentional no-op frame in HQ explosion timeline. */
}

/* ASM: FirstExplode_Pic */
void first_explode_pic(void) {
    draw_hq_small_explode(0xF1);
}

/* ASM: SecondExplode_Pic */
void second_explode_pic(void) {
    draw_hq_small_explode(0xF5);
}

/* ASM: ThirdExplode_Pic */
void third_explode_pic(void) {
    draw_hq_small_explode(0xF9);
}

/* ASM: FourthExplode_Pic (6140) — 32x32 smaller explosion */
void fourth_explode_pic(void) {
    /* LDA #0; STA HQExplode_SprBase */
    HQExplode_SprBase = 0u;
    /* JSR Draw_BigExplode */
    draw_hq_big_explode();
}

/* ASM: FifthExplode_Pic (6151) — biggest 32x32 explosion */
void fifth_explode_pic(void) {
    /* LDA #$10; STA HQExplode_SprBase */
    HQExplode_SprBase = 0x10u;
    /* JSR Draw_BigExplode */
    draw_hq_big_explode();
}

static void (*const HQExplode_JumpTable[])(void) = {
    end_ice_move,
    first_explode_pic,
    second_explode_pic,
    third_explode_pic,
    fourth_explode_pic,
    fifth_explode_pic,
};

/* ASM: HQ_Handle (6032). Все 6 внутренних ASM-меток сохранены как goto-цели. */
void hq_handle(void) {
    uint8_t a;
    uint8_t index;

    /* LDA HQArmour_Timer; BEQ HQ_Explode_Handle */
    if (HQArmour_Timer == 0u) goto HQ_Explode_Handle;
    /* LDA Frame_Counter; AND #$F; BNE HQ_Explode_Handle — 4 раза/сек */
    if ((Frame_Counter & 0x0Fu) != 0u) goto HQ_Explode_Handle;
    /* LDA Frame_Counter; AND #63; BNE Skip_DecHQTimer — каждую секунду */
    if ((Frame_Counter & 0x3Fu) != 0u) goto Skip_DecHQTimer;
    /* DEC HQArmour_Timer; BEQ Normal_HQ_Handle */
    HQArmour_Timer--;
    if (HQArmour_Timer == 0u) goto Normal_HQ_Handle;

Skip_DecHQTimer:
    /* LDA HQArmour_Timer; CMP #4; BCS HQ_Explode_Handle */
    if (HQArmour_Timer >= 4u) goto HQ_Explode_Handle;
    /* LDA Frame_Counter; AND #$10; BEQ Normal_HQ_Handle — мигание раз в 16 кадров */
    if ((Frame_Counter & 0x10u) == 0u) goto Normal_HQ_Handle;
    /* JSR Draw_ArmourHQ; JMP HQ_Explode_Handle */
    draw_armour_hq();
    goto HQ_Explode_Handle;

Normal_HQ_Handle:
    /* JSR DraW_Normal_HQ; fallthrough → HQ_Explode_Handle */
    draw_normal_hq();

HQ_Explode_Handle:
    /* LDA HQ_Status; BEQ End_HQ_Handle */
    if (HQ_Status == 0u) goto End_HQ_Handle;
    /* BMI End_HQ_Handle */
    if ((int8_t)HQ_Status < 0) goto End_HQ_Handle;
    /* LDA #3; STA TSA_Pal */
    TSA_Pal = 3u;
    /* DEC HQ_Status */
    HQ_Status--;
    /* LDA HQ_Status; LSR A; LSR A — квантизация по 4 кадра */
    a = (uint8_t)(HQ_Status >> 2u);
    /* SEC; SBC #5; BPL @_ */
    a = (uint8_t)(a - 5u);
    if ((int8_t)a >= 0) goto at_;
    /* EOR #$FF; CLC; ADC #1 — два-комплемент инверсия */
    a = (uint8_t)((uint8_t)(~a) + 1u);

at_:
    /* SEC; SBC #5; BPL @__ */
    a = (uint8_t)(a - 5u);
    if ((int8_t)a >= 0) goto at__;
    /* EOR #$FF; CLC; ADC #1 */
    a = (uint8_t)((uint8_t)(~a) + 1u);

at__:
    /* ASL A; TAY */
    index = (uint8_t)(a << 1);
    /* LDA HQExplode_JumpTable,Y / +1,Y; JMP (LowPtr_Byte) */
    if (index < (uint8_t)(sizeof(HQExplode_JumpTable) / sizeof(HQExplode_JumpTable[0]) * 2u)) {
        HQExplode_JumpTable[index / 2u]();
    }

End_HQ_Handle:
    return;
}

/* ASM: Draw_HQSmallExplode (6115) */
void draw_hq_small_explode(uint8_t tile) {
    Temp_X = 0x78;
    Temp_Y = 0xD8;
    draw_small_explode(tile);
}
