#include "debug.h"
#include <stdint.h>

extern uint8_t  Joypad1_Differ;
extern uint8_t  Joy_Counter;
extern uint8_t  Construction_Flag;
extern uint8_t  Player1_Lives;
extern uint8_t  Player2_Lives;
extern uint8_t  HQ_Status;
extern uint8_t  CursorPos;
extern uint8_t  HiScore_1P_String[8];

/* ===== Edit ONE of these to 1 to pick a target screen. =====
 * DBG_HISCORE also walks through the Game Over screen on the way. */
#define DBG_SECRET   0
#define DBG_HISCORE  1
/* ============================================================ */

#define KICK_DELAY_FRAMES (60 * 2)

void debug(void) {
#if DBG_SECRET + DBG_HISCORE == 0
    return;
#else
    static int kick = KICK_DELAY_FRAMES;
    static int started = 0;

#if DBG_SECRET
    /* One-shot: arm secret-msg condition on title, then press START. */
    if (started) return;
    if (kick-- > 0) return;
    Construction_Flag = 7;
    Joy_Counter       = 0x74;
    CursorPos         = 2;
    Joypad1_Differ    = 8;
    started = 1;
    return;
#else
    /* HISCORE path (Game Over shows automatically along the way):
     *   1) wait so title is fully drawn,
     *   2) press START to abort scroll_title_scrn() animation,
     *   3) wait, press START to pick 2P → start_stage_sel_scrn(),
     *   4) wait, press START to launch the level,
     *   5) every frame after that, pin lives=0 and HQ destroyed so
     *      level_end_check() fires Game Over on the first battle tick,
     *      and pin 1P score > HiScore_String so hi-score entry triggers. */
    if (started < 3) {
        if (kick-- > 0) return;
        CursorPos      = 1;
        Joypad1_Differ = 8;   /* 1: abort scroll, 2: pick 2P, 3: start level */
        kick = (started == 1) ? 60 : 10;
        started++;
        return;
    }

    Player1_Lives = 0;
    Player2_Lives = 0;
    HQ_Status     = 0;

    /* Score encoding (see draw.c:563 null_8bytes_string):
     *   bytes [0..6] = digits MSB-first, byte [7] = 0xFF terminator.
     *   Position [6]=ones, [5]=tens, [4]=hundreds, [3]=thousands,
     *   [2]=ten-thousands.  Display skips leading zeros.
     * Default HiScore_String has [2]=2 (=20000) — leave it.
     * null_both_hi_score() (called inside selected_1player) zeros the
     * 1P string, so we pin 1P every frame to beat HI.                  */
    HiScore_1P_String[0] = 0;
    HiScore_1P_String[1] = 0;
    HiScore_1P_String[2] = 3;    /* 1P = 30000 → beats HI 20000 */
    HiScore_1P_String[3] = 0;
    HiScore_1P_String[4] = 0;
    HiScore_1P_String[5] = 0;
    HiScore_1P_String[6] = 0;
    HiScore_1P_String[7] = 0xFF; /* terminator — must not be 0 */
#endif
#endif
}
