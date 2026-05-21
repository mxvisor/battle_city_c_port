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

/* ASM: Draw_RespawnPic (1112). Tile = $A1 + (3 - |3 - Counter|) * 4. */
void draw_respawn_pic(void) {
    nmi_wait();
    TSA_Pal = 3u;
    /* LDA #3; SEC; SBC Counter; BPL @_; EOR #$FF; CLC; ADC #1 */
    uint8_t a = (uint8_t)(3u - Counter);
    if ((int8_t)a >= 0) goto at_;
    a = (uint8_t)(~a + 1u);
at_:
    Temp = a;
    /* LDA #3; SEC; SBC Temp; ASL; ASL; CLC; ADC #$A1 */
    a = (uint8_t)((uint8_t)(3u - Temp) << 2u);
    Spr_TileIndex = (uint8_t)(a + 0xA1u);
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

/* ASM: Draw_Drop (1062). Сначала 8 кадров respawn-анимации (по 4 фрейма каждый),
 * затем капля падает по 1 пикселю до Block_Y = $F8. */
void draw_drop(void) {
    Block_X = 0x78u;
    Block_Y = 0x1Eu;
    Counter = 0u;

at_:
    draw_respawn_pic();
    draw_respawn_pic();
    draw_respawn_pic();
    draw_respawn_pic();
    Counter++;
    /* CMP #7; BNE @_ — ASM CMP сравнивает после INC, т.е. цикл идёт пока Counter != 7,
     * но это даёт Counter = 1..7 (7 итераций). C-версия (Counter < 8 = 8 итераций) выглядит как баг C-порта; следуем ASM. */
    if (Counter != 7u) goto at_;

at__:
    nmi_wait();
    Block_Y = (uint8_t)(Block_Y + 1u);
    Spr_TileIndex = 0x9Du;
    TSA_Pal = 1u;
    Temp_X = Block_X;
    Temp_Y = Block_Y;
    draw_whole_spr();
    if (Block_Y != 0xF8u) goto at__;
}

/* ASM: Wait_1Second (1046). Ждёт пока Frame_Counter & $3F не обнулится (60 кадров ≈ 1с). */
void wait_1second(void) {
    Frame_Counter = 0u;
at_:
    nmi_wait();
    if ((Frame_Counter & 0x3Fu) != 0u) goto at_;
}
