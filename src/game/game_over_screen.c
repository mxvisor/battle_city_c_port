#include "game_over_screen.h"
#include "draw.h"
#include "nmi.h"
#include "sound_engine.h"
#include "strings.h"
#include "zeropage.h"
#include "bss.h"

static const uint8_t aGame[] = { 'G', 'A', 'M', 'E', 0xFF };
static const uint8_t aOver[] = { 'O', 'V', 'E', 'R', 0xFF };

void draw_brick_game_over(void) {
    screen_off();
    PPU_Addr_Ptr = 0x1C;
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    null_nt_buffer();

    Block_X = 0x3C;
    Block_Y = 0x46;
    draw_brick_str(aGame);

    Block_X = 0x3C;
    Block_Y = 0x78;
    draw_brick_str(aOver);

    store_nt_buffer_in_vram();
    set_ppu();

    Seconds_Counter = 0;
    Snd_GameOver1 = 1;
    Snd_GameOver2 = 1;
    Snd_GameOver3 = 1;

Next_Frame:
    nmi_wait();
    if ((Joypad1_Differ & 0x0C) != 0) {
        goto End_Draw_Brick_GameOver;
    }
    if (Snd_GameOver1 == 0) {
        goto End_Draw_Brick_GameOver;
    }
    goto Next_Frame;

End_Draw_Brick_GameOver:
    screen_off();
    null_nt_buffer();
    store_nt_buffer_in_vram();
    set_ppu();
    sound_stop();
}
