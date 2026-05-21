#include "pts_screen.h"
#include "draw.h"
#include <string.h>
#include "nmi.h"
#include "zeropage.h"
#include "score.h"
#include "bss.h"

static const uint8_t aHikscore[] = { 'H', 'I', 'k', 'S', 'C', 'O', 'R', 'E', 0xFF };
static const uint8_t aStage[] = { 'S', 'T', 'A', 'G', 'E', 0xFF };
static const uint8_t aKplayer[] = { '^', 'k', 'P', 'L', 'A', 'Y', 'E', 'R', 0xFF };
static const uint8_t a_kplayer[] = { '_', 'k', 'P', 'L', 'A', 'Y', 'E', 'R', 0xFF };
static const uint8_t aPts[] = { 'P', 'T', 'S', 0xFF };
static const uint8_t Arrow_Left[] = { 0x5B, 0xFF };
static const uint8_t Arrow_Right[] = { 0x5D, 0xFF };
static const uint8_t aLine[] = { 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0xFF };
static const uint8_t aTotal[] = { 'T', 'O', 'T', 'A', 'L', 0xFF };
static const uint8_t aBonus[] = { 'B', 'O', 'N', 'U', 'S', 0x15, 0xFF };
static const uint8_t TankKill_Pts[4] = { 0x10, 0x20, 0x30, 0x40 };

void fill_attrib_table(void) {
    memset(&NT_Buffer[0x3C0], 0x50, 4);
    NT_Buffer[0x3C8] = 0x50;
    NT_Buffer[0x3C9] = 0x50;
    NT_Buffer[0x3CA] = 0x50;
    NT_Buffer[0x3CD] = 0x50;
    NT_Buffer[0x3CE] = 0x50;
    NT_Buffer[0x3CF] = 0x50;

    memset(&NT_Buffer[0x3C4], 0xA0, 4);
    NT_Buffer[0x3D0] = 0x0A;
    NT_Buffer[0x3D1] = 0x0A;
    NT_Buffer[0x3D2] = 0x0A;
    NT_Buffer[0x3D5] = 0x0A;
    NT_Buffer[0x3D6] = 0x0A;
    NT_Buffer[0x3D7] = 0x0A;

    NT_Buffer[0x3F0] = 5;
    NT_Buffer[0x3F1] = 5;
    NT_Buffer[0x3F2] = 5;
    NT_Buffer[0x3F5] = 5;
    NT_Buffer[0x3F6] = 5;
    NT_Buffer[0x3F7] = 5;
}

/* ASM: Draw_Pts_Screen_Template (2611) */
void draw_pts_screen_template(void) {
    nmi_wait();
    Tmp_CharIndexBase = 1;
    PPU_Addr_Ptr = 0x24;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0x02;
    Char_Index_Base = 0x30;
    BkgPal_Number = 3;

    screen_off();
    null_nt_buffer();
    fill_attrib_table();
    store_nt_buffer_in_vram();
    set_ppu();

    string_to_screen_buffer(8, 3, aHikscore);

    save_aligned_str_to_scr_buffer(0x12, 3, &HiScore_String[1]);

    string_to_screen_buffer(0xC, 5, aStage);
    byte_to_num_string(Level_Number);
    save_aligned_str_to_scr_buffer(0xE, 5, &Num_String[1]);

    nmi_wait();
    string_to_screen_buffer(3, 7, aKplayer);
    save_aligned_str_to_scr_buffer(5, 9, &HiScore_1P_String[1]);

    string_to_screen_buffer(0xE, 0xC, Arrow_Left);
    string_to_screen_buffer(0xE, 0xF, Arrow_Left);
    string_to_screen_buffer(0xE, 0x12, Arrow_Left);
    string_to_screen_buffer(0xE, 0x15, Arrow_Left);

    /* ASM: BEQ Skip_ScndPlayerDraw */
    if (CursorPos == 0) goto Skip_ScndPlayerDraw;

    nmi_wait();
    string_to_screen_buffer(0x15, 7, a_kplayer);
    save_aligned_str_to_scr_buffer(0x17, 9, &HiScore_2P_String[1]);

    string_to_screen_buffer(0x11, 0xC, Arrow_Right);
    string_to_screen_buffer(0x11, 0xF, Arrow_Right);
    string_to_screen_buffer(0x11, 0x12, Arrow_Right);
    string_to_screen_buffer(0x11, 0x15, Arrow_Right);

Skip_ScndPlayerDraw:
    nmi_wait();
    string_to_screen_buffer(8, 0xC, aPts);
    string_to_screen_buffer(8, 0xF, aPts);
    string_to_screen_buffer(8, 0x12, aPts);
    string_to_screen_buffer(8, 0x15, aPts);

    /* ASM: BEQ Skip_ScndPlayerPtsDraw */
    if (CursorPos == 0) goto Skip_ScndPlayerPtsDraw;

    nmi_wait();
    string_to_screen_buffer(0x1A, 0xC, aPts);
    string_to_screen_buffer(0x1A, 0xF, aPts);
    string_to_screen_buffer(0x1A, 0x12, aPts);
    string_to_screen_buffer(0x1A, 0x15, aPts);

Skip_ScndPlayerPtsDraw:
    nmi_wait();
    string_to_screen_buffer(0xC, 0x16, aLine);
    string_to_screen_buffer(6, 0x17, aTotal);
}

/* ASM: Draw_Pts_Screen (2331) */
void draw_pts_screen(void) {
    uint8_t row;

    draw_pts_screen_template();

    draw_tank_column_x_times(0x1E);

    /* Calculate totals */
    TotalEnmy_KilledBy1P = (uint8_t)(Enmy_KlledBy1P_Count[0] + Enmy_KlledBy1P_Count[1]
                                   + Enmy_KlledBy1P_Count[2] + Enmy_KlledBy1P_Count[3]);
    TotalEnmy_KilledBy2P = (uint8_t)(Enmy_KlledBy2P_Count[0] + Enmy_KlledBy2P_Count[1]
                                   + Enmy_KlledBy2P_Count[2] + Enmy_KlledBy2P_Count[3]);
    Counter = 0;

DrawPtsScrn_NxtTank:
    nmi_wait();
    draw_tank_column();
    null_8bytes_string(Temp_1PPts_String);
    null_8bytes_string(Temp_2PPts_String);
    BrickChar_X = 0;
    BrickChar_Y = 0;

DrawPtsScrn_NxtCount:
    nmi_wait();
    draw_tank_column();
    EndCount_Flag = 0;
    num_to_num_string(TankKill_Pts[Counter]);

    /* ASM: LDA Enmy_KlledBy1P_Count,X; BEQ @_ */
    if (Enmy_KlledBy1P_Count[Counter] == 0) {
        goto at_;
    }
    Snd_PtsCount1 = 1;
    Snd_PtsCount2 = 1;
    Enmy_KlledBy1P_Count[Counter]--;
    BrickChar_X++;
    add_score(2); /* ASM: LDX #2 → Temp_1PPts_String */
    EndCount_Flag = 1;
    add_life(0);

at_: /* ASM: @_ */
    if (Enmy_KlledBy2P_Count[Counter] == 0) {
        goto at__;
    }
    Snd_PtsCount1 = 1;
    Snd_PtsCount2 = 1;
    Enmy_KlledBy2P_Count[Counter]--;
    BrickChar_Y++;
    add_score(3); /* ASM: LDX #3 → Temp_2PPts_String */
    EndCount_Flag = 1;
    add_life(0);

at__: /* ASM: @__ */
    save_aligned_str_to_scr_buffer(5, 9, &HiScore_1P_String[1]);

    row = (uint8_t)((uint8_t)(Counter * 3u) + 0x0Cu);
    save_aligned_str_to_scr_buffer(1, row, &Temp_1PPts_String[1]);

    byte_to_num_string(BrickChar_X);
    save_aligned_str_to_scr_buffer(8, row, &Num_String[1]);

    /* ASM: BEQ @___ */
    if (CursorPos == 0) {
        goto at___;
    }
    save_aligned_str_to_scr_buffer(0x17, 9, &HiScore_2P_String[1]);

    save_aligned_str_to_scr_buffer(0x13, row, &Temp_2PPts_String[1]);

    byte_to_num_string(BrickChar_Y);
    save_aligned_str_to_scr_buffer(0x0E, row, &Num_String[1]);

at___: /* ASM: @___ */
    draw_tank_column_x_times(8);

    /* ASM: BEQ @_____; JMP DrawPtsScrn_NxtCount */
    if (EndCount_Flag == 0) goto at_____;
    goto DrawPtsScrn_NxtCount;

at_____: /* ASM: @_____ */
    Counter++;
    if (Counter == 4) goto tanksProcessed;
    draw_tank_column_x_times(0x14);
    goto DrawPtsScrn_NxtTank;

tanksProcessed:
    draw_tank_column_x_times(0x1E);

    byte_to_num_string(TotalEnmy_KilledBy1P);
    save_aligned_str_to_scr_buffer(8, 0x17, &Num_String[1]);

    /* ASM: BEQ @______ */
    if (CursorPos == 0) goto at______;
    byte_to_num_string(TotalEnmy_KilledBy2P);
    save_aligned_str_to_scr_buffer(0x0E, 0x17, &Num_String[1]);

at______: /* ASM: @______ */
    draw_tank_column_x_times(0x0F);

    /* ASM: BNE DrawPtsScrn_CheckHQ; JMP End_Draw_Pts_Screen */
    if (CursorPos == 0) goto End_Draw_Pts_Screen;
    goto DrawPtsScrn_CheckHQ;

DrawPtsScrn_CheckHQ:
    /* ASM: BNE DrawPtsScrn_CheckNum; JMP End_Draw_Pts_Screen */
    if (HQ_Status == 0) goto End_Draw_Pts_Screen;
    goto DrawPtsScrn_CheckNum;

DrawPtsScrn_CheckNum:
    /* ASM: CMP TotalEnmy_KilledBy1P; BCS DrawPtsScrn_CheckLives */
    if (TotalEnmy_KilledBy2P >= TotalEnmy_KilledBy1P) goto DrawPtsScrn_CheckLives;
    /* ASM: BEQ DrawPtsScrn_CheckLives */
    if (Player1_Lives == 0) goto DrawPtsScrn_CheckLives;

    /* 1P wins BONUS */
    num_to_num_string(0);
    add_score(0);

    save_aligned_str_to_scr_buffer(5, 9, &HiScore_1P_String[1]);

    save_aligned_str_to_scr_buffer(1, 0x1A, &Num_String[1]);

    string_to_screen_buffer(3, 0x19, aBonus);
    string_to_screen_buffer(8, 0x1A, aPts);

    Snd_BonusPts = 1;
    Sound_DataBlocks[0] = 1;
    Sound_Tmp_Unused = 1;
    add_life(0);
    goto End_Draw_Pts_Screen;

DrawPtsScrn_CheckLives:
    /* ASM: CMP TotalEnmy_KilledBy2P; BCS End_Draw_Pts_Screen */
    if (TotalEnmy_KilledBy1P >= TotalEnmy_KilledBy2P) goto End_Draw_Pts_Screen;
    /* ASM: BEQ End_Draw_Pts_Screen */
    if (Player2_Lives == 0) goto End_Draw_Pts_Screen;

    /* 2P wins BONUS */
    num_to_num_string(0);
    add_score(1);

    save_aligned_str_to_scr_buffer(0x17, 9, &HiScore_2P_String[1]);

    save_aligned_str_to_scr_buffer(0x14, 0x1A, &Num_String[1]);

    string_to_screen_buffer(0x16, 0x19, aBonus);
    string_to_screen_buffer(0x1B, 0x1A, aPts);

    Snd_BonusPts = 1;
    Sound_DataBlocks[0] = 1;
    Sound_Tmp_Unused = 1;
    add_life(0);

End_Draw_Pts_Screen:
    /* ASM: LDX #Enmy_KlledBy2P_Count+1 → addr 0x77+1 = 0x78 = 120 frames */
    draw_tank_column_x_times(0x78);

    PPU_REG1_Stts = 0;
    Char_Index_Base = 0;
    Tmp_CharIndexBase = 0;
    BkgPal_Number = 0;
}

/* ASM: Draw_Spr_InColumn (2886). Принимает tile через параметр (был в A до JSR);
 * STA Spr_TileIndex; LDX #$81; JSR Draw_WholeSpr */
void draw_spr_in_column(uint8_t tile) {
    /* STA Spr_TileIndex */
    Spr_TileIndex = tile;
    /* LDX #$81 — X-координата (центр столбца enemy-икон) */
    Temp_X = 0x81u;
    /* JSR Draw_WholeSpr */
    draw_whole_spr();
}

/* ASM: Draw_Tank_Column (2826) — рисует 4 enemy-иконки в столбец */
void draw_tank_column(void) {
    /* LDA #2; STA TSA_Pal — спрайт-палитра 2 */
    TSA_Pal = 2u;
    /* LDY #$64; LDA #$80; JSR Draw_Spr_InColumn — 1-й тип */
    Temp_Y = 0x64u;
    draw_spr_in_column(0x80u);
    /* LDY #$7C; LDA #$A0 — 2-й тип */
    Temp_Y = 0x7Cu;
    draw_spr_in_column(0xA0u);
    /* LDY #$94; LDA #$C0 — 3-й тип */
    Temp_Y = 0x94u;
    draw_spr_in_column(0xC0u);
    /* LDY #$AC; LDA #$E0 — 4-й тип */
    Temp_Y = 0xACu;
    draw_spr_in_column(0xE0u);
}

/* ASM: DrawTankColumn_XTimes (3069) — count раз вызывает NMI_Wait + Draw_Tank_Column */
void draw_tank_column_x_times(uint8_t count) {
DrawTankColumn_XTimes:
    /* JSR NMI_Wait */
    nmi_wait();
    /* TXA; PHA; JSR Draw_Tank_Column; PLA; TAX — count сохраняется через локальную переменную */
    draw_tank_column();
    /* DEX; BNE DrawTankColumn_XTimes */
    count--;
    if (count != 0u) goto DrawTankColumn_XTimes;
}
