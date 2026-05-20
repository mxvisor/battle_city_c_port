#include "begin.h"
#include "draw.h"
#include "nmi.h"
#include "zeropage.h"
#include "title_screen.h"
#include "demo_level_screen.h"

void begin(void) {
    if (setjmp(game_exit_buf) != 0) {
        return;
    }

begin_label:
    draw_title_screen();
    Construction_Flag = 0;

new_scroll:
    null_upper_nt();
    scroll_title_scrn();

title_loaded:;
    int ret = title_screen_loop();
    if (ret == 2) { // Goto Title_Loaded
        goto title_loaded;
    } else if (ret == 3) { // Goto BEGIN
        goto begin_label;
    }
    
    // In assembly, it continues after JSR Title_Screen_Loop to Load_DemoLevel
    load_demo_level();
    /* ASM: BonusLevel_ButtonCheck возвращает 1 при `JMP Title_Loaded` (Button_Pressed). */
    if (bonus_level_button_check() == 1) {
        goto title_loaded;
    }
    goto new_scroll;
}
