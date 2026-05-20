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

/* ASM: Draw_HQBigExplode */
void draw_hq_big_explode(uint8_t base) {
    HQExplode_SprBase = base;

    Temp_X = 0x70;
    Temp_Y = 0xD0;
    add_explode_spr_base(0xD1);

    Temp_X = 0x80;
    Temp_Y = 0xD0;
    add_explode_spr_base(0xD5);

    Temp_X = 0x70;
    Temp_Y = 0xE0;
    add_explode_spr_base(0xD9);

    Temp_X = 0x80;
    Temp_Y = 0xE0;
    add_explode_spr_base(0xDD);
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

/* ASM: FourthExplode_Pic */
void fourth_explode_pic(void) {
    draw_hq_big_explode(0x00);
}

/* ASM: FifthExplode_Pic */
void fifth_explode_pic(void) {
    draw_hq_big_explode(0x10);
}

static void (*const HQExplode_JumpTable[])(void) = {
    end_ice_move,
    first_explode_pic,
    second_explode_pic,
    third_explode_pic,
    fourth_explode_pic,
    fifth_explode_pic,
};

void hq_handle(void) {
    /* ASM: LDA HQArmour_Timer / BEQ HQ_Explode_Handle */
    if (HQArmour_Timer == 0) {
        goto HQ_Explode_Handle;
    }

    /* ASM: LDA Frame_Counter / AND #$F / BNE HQ_Explode_Handle */
    if ((Frame_Counter & 0x0F) != 0) {
        goto HQ_Explode_Handle;
    }

    /* ASM: LDA Frame_Counter / AND #63 / BNE Skip_DecHQTimer */
    if ((Frame_Counter & 0x3F) != 0) {
        goto Skip_DecHQTimer;
    }

    /* ASM: DEC HQArmour_Timer / BEQ Normal_HQ_Handle */
    HQArmour_Timer--;
    if (HQArmour_Timer == 0) {
        goto Normal_HQ_Handle;
    }

Skip_DecHQTimer:
    /* ASM: LDA HQArmour_Timer / CMP #4 / BCS HQ_Explode_Handle */
    if (HQArmour_Timer >= 4) {
        goto HQ_Explode_Handle;
    }

    /* ASM: LDA Frame_Counter / AND #$10 / BEQ Normal_HQ_Handle */
    if ((Frame_Counter & 0x10) == 0) {
        goto Normal_HQ_Handle;
    }

    /* ASM: JSR Draw_ArmourHQ / JMP HQ_Explode_Handle */
    draw_armour_hq();
    goto HQ_Explode_Handle;

Normal_HQ_Handle:
    /* ASM: JSR DraW_Normal_HQ */
    draw_normal_hq();

HQ_Explode_Handle:
    /* ASM: LDA HQ_Status / BEQ End_HQ_Handle */
    if (HQ_Status == 0) {
        goto End_HQ_Handle;
    }

    /* ASM: BMI End_HQ_Handle */
    if ((int8_t)HQ_Status < 0) {
        goto End_HQ_Handle;
    }

    /* ASM: LDA #3 / STA TSA_Pal */
    TSA_Pal = 3;

    /* ASM: DEC HQ_Status */
    HQ_Status--;

    /* ASM: LDA HQ_Status / LSR A / LSR A */
    uint8_t temp = (uint8_t)(HQ_Status >> 2u);

    /* ASM: SEC / SBC #5 / BPL @_ */
    int value = (int)temp - 5;
    if (value < 0) {
        /* ASM: EOR #$FF / CLC / ADC #1 => value = -value */
        value = (value ^ 0xFF) + 1;
    }

at_: /* ASM: @_ */
    /* ASM: SEC / SBC #5 / BPL @__ */
    value -= 5;
    if (value < 0) {
        /* ASM: EOR #$FF / CLC / ADC #1 => value = -value */
        value = (value ^ 0xFF) + 1;
    }

at__: /* ASM: @__ */
    /* ASM: ASL A / TAY */
    uint8_t index = (uint8_t)(value << 1);

    /* ASM: LDA HQExplode_JumpTable,Y / STA LowPtr_Byte */
    /* ASM: LDA HQExplode_JumpTable+1,Y / STA HighPtr_Byte */
    /* ASM: JMP (LowPtr_Byte) */
    if (index < (sizeof(HQExplode_JumpTable) / sizeof(HQExplode_JumpTable[0]) * 2)) {
        HQExplode_JumpTable[index / 2]();
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
