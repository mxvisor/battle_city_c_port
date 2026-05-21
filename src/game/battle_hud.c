#include "battle_hud.h"
#include "strings.h"
#include "zeropage.h"
#include "nmi.h"
#include "draw.h"


static const uint8_t I_p[] = { 0x58, 0x13, 0xFF };
static const uint8_t II_p[] = { 0x5A, 0x13, 0xFF };

static const uint8_t LevelFlag_Upper_Icons[] = { 0x6C, 0xFC, 0xFF };
static const uint8_t LevelFlag_Lower_Icons[] = { 0x6D, 0xFD, 0xFF };
static const uint8_t PlayerLives_Icon[] = { 0x14, 0xFF };
static const uint8_t Reinforcement_Icons[] = { 0x6A, 0x6A, 0xFF };
static const uint8_t Empty_Tile[] = { 0x11, 0xFF };

/* ASM: PointAt_RightScrnColumn (1599) */
void point_at_right_scrn_column(uint8_t icon_index, uint8_t *x, uint8_t *y) {
    /* icon_index is pushed, low bit selects left/right of info column */
    /* 29 tiles from line start to right info column start */
    *x = 29 + (icon_index & 1);

    /* divide icon index by 2 because info column is two tiles wide */
    /* info column is 3 tiles from top screen border */
    *y = (icon_index >> 1) + 3;
}

/* ASM: ReinforceToRAM (1618) */
void reinforce_to_ram(uint8_t icon_index) {
    uint8_t x, y;
    point_at_right_scrn_column(icon_index, &x, &y);
    string_to_screen_buffer(x, y, Reinforcement_Icons); //Compose	list of	remaining enemies
}

/* ASM: Draw_Reinforcemets (1645). Counter идёт 18,16,..0 (10 итераций),
 * каждая итерация рисует одну строку (2 иконки) в стек резерва. */
void draw_reinforcements(void) {
    Counter = 18u;
at_:
    reinforce_to_ram(Counter);
    /* DEC Counter; DEC Counter — два декремента (две иконки в линии) */
    Counter = (uint8_t)(Counter - 2u);
    if ((int8_t)Counter >= 0) goto at_;
}

void draw_level_flag(void) {
    /* ASM: Draw_LevelFlag (1566). */
    nmi_wait();
    string_to_screen_buffer(0x1D, 0x17, LevelFlag_Upper_Icons);
    string_to_screen_buffer(0x1D, 0x18, LevelFlag_Lower_Icons);

    Char_Index_Base = 0x6E;
    byte_to_num_string(Level_Number);
    uint8_t *level_number_str = ptr_to_nonzero_str_elem(Num_String + 1);
    uint8_t level_x = 0x19 + (uint8_t)(level_number_str - (Num_String + 1));
    save_str_to_scr_buffer(level_x, 0x19, level_number_str);
    Char_Index_Base = 0;
}

void draw_ip(void) {
    string_to_screen_buffer(0x1D, 0x11, I_p);

    if (Level_Mode == 2) {
        goto Draw_IIP;
    }

    if (CursorPos == 0) {
        goto locret_C858;
    }

Draw_IIP:
    string_to_screen_buffer(0x1D, 0x14, II_p);

locret_C858:
    return;
}

void draw_player_lives(void) {
    Counter = 1;
    Tmp_CharIndexBase = Counter;
    Char_Index_Base = 0x6E;

    string_to_screen_buffer(0x1D, 0x12, PlayerLives_Icon);

    if (Level_Mode == 2) {
        goto Draw_2P_Lives;
    }

    if (CursorPos != 0) {
        goto Draw_2P_Lives;
    }

    Counter = 0;
    goto Draw_1P_Lives;

Draw_2P_Lives:
    string_to_screen_buffer(0x1D, 0x15, PlayerLives_Icon);

Draw_1P_Lives:
    {
        uint8_t lives = Counter ? Player2_Lives : Player1_Lives;
        if (lives > 0) {
            lives--;
        }

        byte_to_num_string(lives);
        uint8_t *lives_str = ptr_to_nonzero_str_elem(Num_String + 1);
        uint8_t lives_x = 0x19 + (uint8_t)(lives_str - (Num_String + 1));
        uint8_t y_coord = Counter ? 0x15 : 0x12;
        save_str_to_scr_buffer(lives_x, y_coord, lives_str);
    }

    if (Counter != 0) {
        Counter--;
        goto Draw_1P_Lives;
    }

    Char_Index_Base = 0;
    Tmp_CharIndexBase = 0;
}

/* Draws an empty tile in the enemy stock column when they exit */
void draw_empty_tile(void) {
    uint8_t x, y;
    point_at_right_scrn_column(Enemy_Reinforce_Count, &x, &y); //Replaces enemy icon when it spawns
    string_to_screen_buffer(x, y, Empty_Tile);
}
