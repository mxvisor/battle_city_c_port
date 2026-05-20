#include "hiscore_screen.h"
#include "zeropage.h"
#include "bss.h"
#include "battle_screen.h"
#include "draw.h"
#include "nmi.h"

static const uint8_t aHiscore[] = { 'H', 'I', 'S', 'C', 'O', 'R', 'E', 0xFF };

static void draw_record_digit(void) {
    const uint8_t *digit = ptr_to_nonzero_str_elem(HiScore_String);
    while (digit && *digit != 0xFF) {
        draw_char(*digit);
        Block_X += 0x20;
        digit++;
    }
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

    while (Snd_RecordPts1 != 0) {
        nmi_wait();
        BkgPal_Number = (Frame_Counter & 3) + 5;
    }

    BkgPal_Number = 0;
}

int update_hi_score(void) { return 0; }
