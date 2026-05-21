#include "hiscore_screen.h"
#include "zeropage.h"
#include "bss.h"
#include "battle_screen.h"
#include "draw.h"
#include "nmi.h"

static const uint8_t aHiscore[] = { 'H', 'I', 'S', 'C', 'O', 'R', 'E', 0xFF };

/* ASM: Draw_RecordDigit (4165) — пропускает ведущие нули в HiScore_String,
 * прибавляя $20 к Block_X за каждый, затем вызывает Draw_BrickStr с остатком. */
void draw_record_digit(void) {
    Block_X = 0x10u;
    Block_Y = 0x64u;
    Char_Index_Base = 0x30u;
    uint8_t y = 0u;

at_:
    if (HiScore_String[y] != 0u) goto at__;
    y++;
    Block_X = (uint8_t)(Block_X + 0x20u);
    goto at_;

at__:
    /* LDA #0; STA HighStrPtr_Byte; STY LowStrPtr_Byte; JSR Draw_BrickStr */
    draw_brick_str(&HiScore_String[y]);
    Char_Index_Base = 0u;
}

void null_both_hi_score(void) {
    null_8bytes_string(HiScore_1P_String);
    null_8bytes_string(HiScore_2P_String);
    init_level_vars();
}

void draw_record_hi_score(void) {
    screen_off();
    PPU_Addr_Ptr = 0x1C;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    null_nt_buffer();

    Block_X = 0x10;
    Block_Y = 0x32;
    draw_brick_str(aHiscore);
    draw_record_digit();

    store_nt_buffer_in_vram();
    set_ppu();

    Seconds_Counter = 0;
    Snd_RecordPts1 = 1;
    Snd_RecordPts2 = 1;
    Snd_RecordPts3 = 1;

at_:
    /* JSR NMI_Wait; LDA Frame_Counter; AND #3; CLC ADC #5; STA BkgPal_Number;
     * LDA Snd_RecordPts1; BNE @_ */
    nmi_wait();
    BkgPal_Number = (uint8_t)((Frame_Counter & 3u) + 5u);
    if (Snd_RecordPts1 != 0u) goto at_;

    BkgPal_Number = 0u;
}

/* ASM: Update_HiScore (4198). Если 1P-score > HiScore_String → копируем 1P→HiScore, Y=1.
 * Затем то же для 2P, Y=$FF.  Возвращает Y (1=1P record, $FF=2P record, 0=none). */
uint8_t update_hi_score(void) {
    uint8_t x = 0u;
    uint8_t y = 0u;

at_:
    if (HiScore_1P_String[x] != HiScore_String[x]) goto hiscoreFinished;
    x++;
    if (x == 7u) goto continueProcess;
    goto at_;

hiscoreFinished:
    /* BMI @continueProcess — если 1P[X] - HiScore[X] < 0 (signed) */
    if ((int8_t)(uint8_t)(HiScore_1P_String[x] - HiScore_String[x]) < 0) goto continueProcess;
    x = 0u;
fillLoop:
    HiScore_String[x] = HiScore_1P_String[x];
    x++;
    if (x != 7u) goto fillLoop;
    y = 1u;

continueProcess:
    x = 0u;
loop_2:
    if (HiScore_2P_String[x] != HiScore_String[x]) goto continueProcess_2;
    x++;
    if (x == 7u) goto exit_;
    goto loop_2;

continueProcess_2:
    if ((int8_t)(uint8_t)(HiScore_2P_String[x] - HiScore_String[x]) < 0) goto exit_;
    x = 0u;
fillLoop_2:
    HiScore_String[x] = HiScore_2P_String[x];
    x++;
    if (x != 7u) goto fillLoop_2;
    y = 0xFFu;

exit_:
    return y;
}
