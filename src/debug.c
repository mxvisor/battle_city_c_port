#include "debug.h"
#include <stdint.h>

extern uint8_t  Joypad1_Buttons;
extern uint8_t  Joypad1_Differ;
extern uint8_t  Joypad2_Buttons;
extern uint8_t  Joypad2_Differ;
extern uint8_t  Joy_Counter;
extern uint8_t  Construction_Flag;
extern uint8_t  Player1_Lives;
extern uint8_t  Player2_Lives;
extern uint8_t  HQ_Status;
extern uint8_t  CursorPos;
extern uint8_t  HiScore_1P_String[8];

/* ===== Pick exactly one (state vars share names across the two paths). ===== */
#define DBG_SECRET   0
#define DBG_HISCORE  1
/* =========================================================================== */

/* NES $4016 bit layout */
#define BTN_A      0x01
#define BTN_B      0x02
#define BTN_START  0x08
#define BTN_DOWN   0x20
#define BTN_RIGHT  0x80

#define CURSOR_CONSTRUCTION 2
#define CURSOR_2P           1

#define KICK_DELAY_FRAMES (10)

void debug(void) {
#if DBG_SECRET + DBG_HISCORE == 0
    return;
#endif

#if DBG_SECRET
    /* Verified against title_screen.c::title_screen_loop:
     *
     *   trigger: (Joypad1_Differ & 8) && Construction_Flag == 7
     *                                 && Joy_Counter == 0x74
     *
     *   Joy_Counter accumulator (reset every title_screen_loop entry):
     *     (Joypad1_Buttons & DOWN)  & (Joypad2_Differ & A)  → += 0x10
     *     (Joypad1_Buttons & RIGHT) & (Joypad2_Differ & B)  → -= 1
     *     8 × 0x10 = 0x80,  0x80 - 12 = 0x74.
     *
     *   Construction_Flag is bumped per Construction-screen entry elsewhere;
     *   we just need 7 enter↔exit cycles via Start.
     *
     * Important contract with nmi.c::read_joypads (which runs every NMI right
     * before debug()): it overwrites Joypad{1,2}_Buttons / Joypad{1,2}_Differ
     * from real input. So our writes here are valid for ONE game frame; we
     * re-assert holds every frame.
     *
     * We never set Joypad1_Differ for DOWN/RIGHT — only Joypad1_Buttons —
     * because (Joypad1_Differ & 4) would falsely register SELECT and bump
     * CursorPos. */
    enum {
        S_WAIT_TITLE,
        S_BEFORE_CYCLE,
        S_ENTER_CONSTR,
        S_EXIT_CONSTR,
        S_TAP_A,
        S_TAP_B,
        S_FINAL_START,
        S_DONE
    };

    static int phase         = S_WAIT_TITLE;
    static int wait_frames   = KICK_DELAY_FRAMES;
    static int constr_cycles = 0;
    static int taps          = 0;

    /* Maintain Down/Right hold every frame (read_joypads cleared it). */
    if (phase == S_TAP_A)      Joypad1_Buttons = BTN_DOWN;
    else if (phase == S_TAP_B) Joypad1_Buttons = BTN_RIGHT;

    if (wait_frames > 0) {
        wait_frames--;
        return;
    }

    switch (phase) {

    case S_WAIT_TITLE:
        /* scroll_title_scrn() exits on (Joypad1_Differ & 0x0C) — Start kills it. */
        Joypad1_Differ = BTN_START;
        wait_frames    = 1;
        phase          = S_BEFORE_CYCLE;
        return;

    case S_BEFORE_CYCLE:
        /* By now title_screen_loop is the active state. */
        CursorPos = CURSOR_CONSTRUCTION;
        phase     = S_ENTER_CONSTR;
        return;

    case S_ENTER_CONSTR:
        CursorPos      = CURSOR_CONSTRUCTION;     /* reassert every cycle */
        Joypad1_Differ = BTN_START;               /* enter Construction   */
        wait_frames    = 1;
        phase          = S_EXIT_CONSTR;
        return;

    case S_EXIT_CONSTR:
        Joypad1_Differ = BTN_START;               /* back to title        */
        wait_frames    = 1;
        constr_cycles++;
        phase = (constr_cycles >= 7) ? S_TAP_A : S_ENTER_CONSTR;
        return;

    case S_TAP_A:
        Joypad1_Buttons = BTN_DOWN;
        Joypad2_Buttons = BTN_A;
        Joypad2_Differ  = BTN_A;                  /* Joy_Counter += 0x10  */
        taps++;
        wait_frames = 1;
        if (taps >= 8) {                          /* Joy_Counter == 0x80  */
            taps  = 0;
            phase = S_TAP_B;
        }
        return;

    case S_TAP_B:
        Joypad1_Buttons = BTN_RIGHT;
        Joypad2_Buttons = BTN_B;
        Joypad2_Differ  = BTN_B;                  /* Joy_Counter -= 1     */
        taps++;
        wait_frames = 1;
        if (taps >= 12) {                         /* Joy_Counter == 0x74  */
            phase       = S_FINAL_START;
            wait_frames = 0;
        }
        return;

    case S_FINAL_START:
        Joypad1_Buttons = 0;                      /* release Right        */
        Joypad1_Differ  = BTN_START;              /* fires show_secret_msg */
        CursorPos       = CURSOR_CONSTRUCTION;
        phase = S_DONE;
        return;

    case S_DONE:
    default:
        (void)Construction_Flag;
        (void)Joy_Counter;
        return;
    }
#endif /* DBG_SECRET */

#if DBG_HISCORE
    /* Walks the menus to launch a 2P battle, then pins state so the very
     * first battle tick triggers Game Over and the hi-score entry screen.
     *
     * Path:
     *   1) Initial wait so title is drawn.
     *   2) Press Start → aborts scroll_title_scrn() (it tests Differ & 0x0C).
     *   3) Wait, press Start with CursorPos = 2P → selected_2players() →
     *      null_both_hi_score() → start_stage_sel_scrn().
     *   4) Wait, press Start on stage select → level launches.
     *   5) Every frame after: pin Player1/2_Lives = 0 and HQ_Status = 0 so
     *      level_end_check() fires Game Over, and pin HiScore_1P_String to
     *      30000 (beats default HI = 20000) so the hi-score entry triggers.
     *      Re-pinning every frame is required because null_both_hi_score()
     *      (run on the 2P press) zeroes the 1P string.
     *
     * Score encoding (draw.c::null_8bytes_string):
     *   bytes [0..6] = digits MSB-first, byte [7] = 0xFF terminator.
     *   [2]=ten-thousands, [3]=thousands, ..., [6]=ones. Default HiScore
     *   has [2]=2 (=20000). Leading zeros are skipped on display. */
    enum {
        H_WAIT_TITLE,     /* press Start to skip title scroll      */
        H_PICK_2P,        /* press Start with cursor on 2P         */
        H_START_LEVEL,    /* press Start on stage select to launch */
        H_TRIGGER         /* pin variables every frame             */
    };

    static int phase       = H_WAIT_TITLE;
    static int wait_frames = KICK_DELAY_FRAMES;

    if (wait_frames > 0) { wait_frames--; return; }

    switch (phase) {

    case H_WAIT_TITLE:
        Joypad1_Differ = BTN_START;       /* aborts scroll_title_scrn */
        wait_frames    = 1;
        phase          = H_PICK_2P;
        return;

    case H_PICK_2P:
        CursorPos      = CURSOR_2P;       /* selected_2players path   */
        Joypad1_Differ = BTN_START;
        wait_frames    = 30;              /* let stage select load    */
        phase          = H_START_LEVEL;
        return;

    case H_START_LEVEL:
        Joypad1_Differ = BTN_START;       /* launch the level         */
        phase          = H_TRIGGER;
        /* wait_frames stays at 0 — H_TRIGGER fires from the next frame */
        return;

    case H_TRIGGER:
    default:
        /* Force Game Over on the first battle tick. */
        Player1_Lives = 0;
        Player2_Lives = 0;
        HQ_Status     = 0;

        /* Pin 1P score = 30000 so hi-score entry triggers. */
        HiScore_1P_String[0] = 0;
        HiScore_1P_String[1] = 0;
        HiScore_1P_String[2] = 3;
        HiScore_1P_String[3] = 0;
        HiScore_1P_String[4] = 0;
        HiScore_1P_String[5] = 0;
        HiScore_1P_String[6] = 0;
        HiScore_1P_String[7] = 0xFF;
        return;
    }
#endif /* DBG_HISCORE */
}