#include "secret_msg_screen.h"
#include "zeropage.h"
#include "nmi.h"
#include "draw.h"

static const uint8_t aThisProgramWas[] = {
    'T','H','I','S',' ',
    'P','R','O','G','R','A','M',' ',
    'W','A','S',0xFF
};
static const uint8_t aWrittenBy[] = {
    'W','R','I','T','T','E','N',' ',
    'B','Y',0xFF
};
static const uint8_t aOpenkreach[] = {
    'O','P','E','N','-','R','E','A','C','H',0xFF
};
static const uint8_t aWhoLovesNoriko[] = {
    'W','H','O',' ',
    'L','O','V','E','S',' ',
    'N','O','R','I','K','O',0xFF
};
static const uint8_t aDot[] = { '.', 0xFF };

static void draw_respawn_pic(void) {
    nmi_wait();
    TSA_Pal = 3;

    int diff = 3 - (int)Counter;
    if (diff < 0) {
        diff = -diff;
    }

    int tile_index = (3 - diff) << 2;
    Spr_TileIndex = (uint8_t)(0xA1 + tile_index);
    Temp_X = Block_X;
    Temp_Y = Block_Y;
    draw_whole_spr();
}

void show_secret_msg(void) {
    screen_off();
    PPU_Addr_Ptr = 0x1C;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    null_nt_buffer();
    store_nt_buffer_in_vram();
    set_ppu();

    wait_1second();
    wait_1second();

    string_to_screen_buffer(8, 8, aThisProgramWas);
    wait_1second();

    string_to_screen_buffer(8, 0x0A, aWrittenBy);
    wait_1second();

    string_to_screen_buffer(8, 0x0C, aOpenkreach);
    wait_1second();

    string_to_screen_buffer(8, 0x0E, aWhoLovesNoriko);
    wait_1second();

    string_to_screen_buffer(8, 0x10, aDot);
    wait_1second();
    string_to_screen_buffer(9, 0x10, aDot);
    wait_1second();
    string_to_screen_buffer(0x0A, 0x10, aDot);
    wait_1second();
    string_to_screen_buffer(0x0B, 0x10, aDot);
    wait_1second();
    string_to_screen_buffer(0x0C, 0x10, aDot);
    wait_1second();

    draw_drop();

    screen_off();
    make_gray_frame();
    store_nt_buffer_in_vram();
    set_ppu();
}

void draw_drop(void) {
    Block_X = 0x78;
    Block_Y = 0x1E;
    Counter = 0;

    while (Counter < 8) {
        draw_respawn_pic();
        draw_respawn_pic();
        draw_respawn_pic();
        draw_respawn_pic();
        Counter++;
    }

    while (Block_Y != 0xF8) {
        nmi_wait();
        Block_Y += 1;
        Spr_TileIndex = 0x9D;
        TSA_Pal = 1;
        Temp_X = Block_X;
        Temp_Y = Block_Y;
        draw_whole_spr();
    }
}

void wait_1second(void) {
    Frame_Counter = 0;
    do {
        nmi_wait();
    } while ((Frame_Counter & 0x3F) != 0);
}
