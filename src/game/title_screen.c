#include "title_screen.h"
#include "zeropage.h"
#include "nmi.h"
#include "secret_msg_screen.h"
#include "stage_select_screen.h"
#include "construction_screen.h"
#include "coords.h"
#include "battle_screen.h"
#include "battle_tank_draw.h"
#include "battle_tank_status.h"
#include "battle_tank.h"
#include "draw.h"
#include "strings.h"
#include "hiscore_screen.h"

static const uint8_t aNAMCOT[] = {
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0xFF
};

static const uint8_t Copyrights[] = {
    0x40, 0x20,
    '1', '9', '8', '1', 0x20,
    '1', '9', '8', '5', 0x20,
    'N', 'A', 'M', 'C', 'O', 0x20,
    'L', 'T', 'D', 0x69, 0xFF
};

static const uint8_t aAllRightsReserved[] = {
    'A','L','L',' ', 'R','I','G','H','T','S',' ', 'R','E','S','E','R','V','E','D', 0xFF
};

static const uint8_t a1Player[] = { '1', ' ', 'P', 'L', 'A', 'Y', 'E', 'R', 0xFF };
static const uint8_t a2Players[] = { '2', ' ', 'P', 'L', 'A', 'Y', 'E', 'R', 'S', 0xFF };
static const uint8_t aConstruction[] = { 'C', 'O', 'N', 'S', 'T', 'R', 'U', 'C', 'T', 'I', 'O', 'N', 0xFF };
static const uint8_t aK[] = { 0x5E, 0x6B, 0xFF };
static const uint8_t a_k[] = { 0x5F, 0x6B, 0xFF };
static const uint8_t aHik[] = { 'H', 'I', 0x6B, 0xFF };

typedef void (*TitleFunc)(void);

void selected_1player(void) {
    TanksOnScreen = 5;
    null_both_hi_score();
    start_stage_sel_scrn();
}

void selected_2players(void) {
    TanksOnScreen = 7;
    null_both_hi_score();
    start_stage_sel_scrn();
}

void selected_construction(void) {
    TanksOnScreen = 7;
    construction();
}

static const TitleFunc Title_JumpTable[] = {
    selected_1player,
    selected_2players,
    selected_construction
};

void draw_title_cursor(void) {
    tanks_status_handle();
}

/* ASM: Draw_TitleScreen (2948).
 * Внутренняя метка @_ (line 3014) сохранена как `at_`. */
static const uint8_t aBattle[] = { 'B','A','T','T','L','E', 0xFFu };
static const uint8_t aCity[]   = { 'C','I','T','Y', 0xFFu };

void draw_title_screen(void) {
    /* JSR Screen_Off */
    screen_off();
    /* LDA #$24; STA PPU_Addr_Ptr */
    PPU_Addr_Ptr = 0x24u;
    /* JSR Null_NT_Buffer */
    null_nt_buffer();

    /* LDX #$1A; STX Block_X; LDY #$2E; STY Block_Y; JSR Draw_BrickStr("BATTLE") */
    Block_X = 0x1Au;
    Block_Y = 0x2Eu;
    draw_brick_str(aBattle);

    /* LDX #$3C; STX Block_X; LDY #$56; STY Block_Y; JSR Draw_BrickStr("CITY") */
    Block_X = 0x3Cu;
    Block_Y = 0x56u;
    draw_brick_str(aCity);

    /* JSR Store_NT_Buffer_InVRAM */
    store_nt_buffer_in_vram();
    /* JSR Set_PPU */
    set_ppu();

    /* LDA #$30; STA Char_Index_Base — цифры начинаются с тайла $30 */
    Char_Index_Base = 0x30u;

    /* Draw "I-" at (2,3), score at (4,3) */
    string_to_screen_buffer(2u, 3u, aK);
    /* LDY #$16; LDX #4; JSR PtrToNonzeroStrElem; LDY #3; JSR Save_Str_To_ScrBuffer */
    /* $16 = HiScore_1P_String+1 (zp offset $15 + 1) */
    save_aligned_str_to_scr_buffer(4u, 3u, &HiScore_1P_String[1]);

    /* Draw "HI-" at ($B,3), score at ($E,3) */
    string_to_screen_buffer(0x0Bu, 3u, aHik);
    /* LDY #$3E; LDX #$E — $3E = HiScore_String+1 (zp offset $3D + 1) */
    save_aligned_str_to_scr_buffer(0x0Eu, 3u, &HiScore_String[1]);

    /* LDA CursorPos; BEQ @_ */
    if (CursorPos == 0u) goto at_;

    /* Draw "II-" at ($15,3), score at ($17,3) */
    string_to_screen_buffer(0x15u, 3u, a_k);
    /* LDY #$1E; LDX #$17 — $1E = HiScore_2P_String+1 (zp offset $1D + 1) */
    save_aligned_str_to_scr_buffer(0x17u, 3u, &HiScore_2P_String[1]);

at_:
    /* LDA #0; STA Char_Index_Base */
    Char_Index_Base = 0u;
    /* JSR NMI_Wait */
    nmi_wait();

    /* Draw menu items at ($B, $11/$13/$15) */
    string_to_screen_buffer(0x0Bu, 0x11u, a1Player);
    string_to_screen_buffer(0x0Bu, 0x13u, a2Players);
    string_to_screen_buffer(0x0Bu, 0x15u, aConstruction);

    /* JSR NMI_Wait */
    nmi_wait();
    /* aNAMCOT at ($B, $17); Copyrights at (4, $19) */
    string_to_screen_buffer(0x0Bu, 0x17u, aNAMCOT);
    string_to_screen_buffer(4u, 0x19u, Copyrights);

    /* JSR NMI_Wait */
    nmi_wait();
    /* aAllRightsReserved at (6, $1B) */
    string_to_screen_buffer(6u, 0x1Bu, aAllRightsReserved);
}

/* ASM: Scroll_TitleScrn (1446).
 * Возвращает 0 при штатном завершении скролла (CMP #$F0 совпало) — caller
 * (begin) делает обычный fallthrough в Title_Loaded.
 * Возвращает 1 при `BNE @__` (нажат START/SELECT) — это аналог `PLA PLA;
 * JMP Title_Loaded` в ASM: tear down stack и прыжок в Title_Loaded. */
int scroll_title_scrn(void) {
    /* LDA #0; STA Scroll_Byte; STA PPU_REG1_Stts */
    Scroll_Byte = 0u;
    PPU_REG1_Stts = 0u;

at_:
    /* JSR NMI_Wait */
    nmi_wait();
    /* INC Scroll_Byte */
    Scroll_Byte++;
    /* LDA Joypad1_Differ; AND #%1100; BNE @__ */
    if ((Joypad1_Differ & 0x0Cu) != 0u) goto at__;
    /* LDA Scroll_Byte; CMP #$F0; BNE @_ */
    if (Scroll_Byte != 0xF0u) goto at_;
    /* RTS */
    return 0;

at__:
    /* PLA; PLA; JMP Title_Loaded — моделируется кодом возврата */
    return 1;
}

/* ASM: Title_Screen_Loop (1827).
 * Возвращаемые значения моделируют ASM `PLA PLA; JMP (LowPtr_Byte)`:
 *   1 — `RTS` из @plus (Seconds_Counter == 10, Construction_Flag == 0): caller
 *       загружает demo-roll.
 *   2 — START + CursorPos == 2 (Selected_Construction): caller делает
 *       `goto Title_Loaded`.
 *   3 — START + CursorPos == 0/1 (Selected_1player/2players): caller
 *       перезапускает с BEGIN после возврата из Start_StageSelScrn. */
int title_screen_loop(void) {
    /* LDA #3; STA BkgPal_Number */
    BkgPal_Number = 3u;
    /* JSR Null_Status */
    null_status();
    /* LDA #$48; STA Tank_X */
    Tank_X[0] = 0x48u;
    /* JSR CurPos_To_PixelCoord */
    cur_pos_to_pixel_coord(CursorPos);
    /* LDA #$83; STA Tank_Status */
    Tank_Status[0] = 0x83u;
    /* LDA #0; STA Seconds_Counter / Tank_Type / Track_Pos / Player_Blink_Timer{0,1} / Scroll_Byte / Joy_Counter */
    Seconds_Counter = 0u;
    Tank_Type[0] = 0u;
    Track_Pos[0] = 0u;
    Player_Blink_Timer[0] = 0u;
    Player_Blink_Timer[1] = 0u;
    Scroll_Byte = 0u;
    Joy_Counter = 0u;
    /* LDA #2; STA PPU_REG1_Stts */
    PPU_REG1_Stts = 2u;

loop:
    /* JSR NMI_Wait */
    nmi_wait();
    /* LDA Frame_Counter; AND #3; BNE @__ */
    if ((Frame_Counter & 3u) != 0u) goto at__;
    /* LDA Track_Pos; EOR #4; STA Track_Pos */
    Track_Pos[0] = (uint8_t)(Track_Pos[0] ^ 4u);

at__:
    /* JSR TanksStatus_Handle */
    tanks_status_handle();
    /* LDA Joypad1_Differ; AND #4; BEQ @___ — SELECT */
    if ((Joypad1_Differ & 4u) == 0u) goto at___;
    /* INC CursorPos; LDA #0; STA Seconds_Counter */
    CursorPos++;
    Seconds_Counter = 0u;

at___:
    /* LDA Joypad1_Buttons; AND #$20; BEQ @____ — P1 DOWN */
    if ((Joypad1_Buttons & 0x20u) == 0u) goto at____;
    /* LDA Joypad2_Differ; AND #1; BEQ @____ — P2 A */
    if ((Joypad2_Differ & 1u) == 0u) goto at____;
    /* LDA #$10; CLC; ADC Joy_Counter; STA Joy_Counter */
    Joy_Counter = (uint8_t)(Joy_Counter + 0x10u);

at____:
    /* LDA Joypad1_Buttons; AND #$80; BEQ @check_Max_CurPos — P1 RIGHT */
    if ((Joypad1_Buttons & 0x80u) == 0u) goto check_Max_CurPos;
    /* LDA Joypad2_Differ; AND #2; BEQ @check_Max_CurPos — P2 B */
    if ((Joypad2_Differ & 2u) == 0u) goto check_Max_CurPos;
    /* DEC Joy_Counter */
    Joy_Counter--;

check_Max_CurPos:
    /* LDA CursorPos; CMP #3; BCC @plus */
    if (CursorPos < 3u) goto plus;
    /* LDA #0; STA CursorPos */
    CursorPos = 0u;

plus:
    /* JSR CurPos_To_PixelCoord */
    cur_pos_to_pixel_coord(CursorPos);
    /* LDA Seconds_Counter; CMP #10; BNE @start_Check */
    if (Seconds_Counter != 10u) goto start_Check;
    /* LDA Construction_Flag; BNE @start_Check */
    if (Construction_Flag != 0u) goto start_Check;
    /* RTS — caller грузит demo-roll */
    return 1;

start_Check:
    /* LDA Joypad1_Differ; AND #8; BEQ @loop */
    if ((Joypad1_Differ & 8u) == 0u) goto loop;
    /* LDA Construction_Flag; CMP #7; BNE @start_Pressed */
    if (Construction_Flag != 7u) goto start_Pressed;
    /* LDA Joy_Counter; CMP #$74; BNE @start_Pressed */
    if (Joy_Counter != 0x74u) goto start_Pressed;
    /* JSR Show_Secret_Msg */
    show_secret_msg();

start_Pressed:
    /* LDA #0; STA BkgPal_Number */
    BkgPal_Number = 0u;
    /* PLA; PLA — моделируется через возврат разных кодов в caller */
    /* LDA CursorPos; ASL; TAY; LDA Title_JumpTable,Y / +1,Y; JMP (LowPtr_Byte) */
    Title_JumpTable[CursorPos]();
    /* JMP (LowPtr_Byte) — поведение jump'а реализуется через return code:
     *   CursorPos == 2 → Selected_Construction → возврат в Title_Loaded (2),
     *   иначе → Selected_1/2player → Start_StageSelScrn → BEGIN после возврата (3). */
    if (CursorPos == 2u) return 2;
    return 3;
}
