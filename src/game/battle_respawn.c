#include "battle_respawn.h"
#include "zeropage.h"
#include "bss.h"
#include "draw.h"
#include "battle_hud.h"

static const uint8_t Enemy_Amount_ROMArray[35 * 4] = {
    0x12, 0x02, 0x00, 0x00,
    0x02, 0x04, 0x00, 0x0E,
    0x0E, 0x04, 0x00, 0x02,
    0x0A, 0x05, 0x02, 0x03,
    0x05, 0x02, 0x08, 0x05,
    0x07, 0x02, 0x09, 0x02,
    0x03, 0x04, 0x06, 0x07,
    0x07, 0x02, 0x04, 0x07,
    0x06, 0x04, 0x07, 0x03,
    0x0C, 0x02, 0x04, 0x02,
    0x05, 0x06, 0x04, 0x05,
    0x08, 0x06, 0x00, 0x06,
    0x08, 0x08, 0x00, 0x04,
    0x0A, 0x04, 0x00, 0x06,
    0x02, 0x00, 0x0A, 0x08,
    0x10, 0x00, 0x02, 0x02,
    0x02, 0x02, 0x08, 0x08,
    0x04, 0x02, 0x06, 0x08,
    0x04, 0x08, 0x04, 0x04,
    0x08, 0x02, 0x02, 0x08,
    0x08, 0x02, 0x06, 0x04,
    0x08, 0x06, 0x02, 0x04,
    0x06, 0x00, 0x04, 0x0A,
    0x04, 0x02, 0x04, 0x0A,
    0x02, 0x08, 0x00, 0x0A,
    0x06, 0x06, 0x04, 0x04,
    0x02, 0x08, 0x08, 0x02,
    0x02, 0x01, 0x0F, 0x02,
    0x0A, 0x04, 0x00, 0x06,
    0x04, 0x08, 0x04, 0x04,
    0x03, 0x08, 0x06, 0x03,
    0x08, 0x06, 0x02, 0x04,
    0x04, 0x08, 0x04, 0x04,
    0x04, 0x0A, 0x00, 0x06,
    0x04, 0x06, 0x00, 0x0A
};

static const uint8_t X_Player_Respawn[2] = { 0x58, 0x98 };
static const uint8_t Y_Player_Respawn[2] = { 0xD8, 0xD8 };
static const uint8_t X_Enemy_Respawn[3] = { 0x18, 0x78, 0xD8 };
static const uint8_t Y_Enemy_Respawn[3] = { 0x18, 0x18, 0x18 };

void respawn_handle(uint8_t slot) {
    if (Respawn_Timer != 0) {
        Respawn_Timer -= 1;
        return;
    }

    if (Enemy_Reinforce_Count == 0) {
        goto End_Respawn_Handle;
    }

    Counter = TanksOnScreen;

nextPlayer:
    {
        uint8_t idx = Counter;
        if (Tank_Status[idx] == 0) {
            Respawn_Timer = Respawn_Delay;
            make_respawn(idx);
            Enemy_Reinforce_Count -= 1;
            draw_empty_tile();
            return;
        }
    }

    Counter -= 1;
    if (Counter != 1) {
        goto nextPlayer;
    }

End_Respawn_Handle:
    return;
}

/* ASM: Make_Respawn (6185) */
void make_respawn(uint8_t slot) {
    Tank_Type[slot] = 0;
    if (slot >= 2) {
        goto Enemy_Operations;
    }

    Tank_X[slot] = X_Player_Respawn[slot];
    Tank_Y[slot] = Y_Player_Respawn[slot];
    Player_Blink_Timer[slot] = 0;
    goto exit_make_respawn;

/* ASM: Enemy_Operations (6199) */    
Enemy_Operations:
    EnemyRespawn_PlaceIndex++;
    uint8_t place_index = EnemyRespawn_PlaceIndex;
    if (place_index == 3) {
        EnemyRespawn_PlaceIndex = 0;
        place_index = 0;
    }

    Tank_X[slot] = X_Enemy_Respawn[place_index];
    Tank_Y[slot] = Y_Enemy_Respawn[place_index];

    if (Enemy_Reinforce_Count == 3 || Enemy_Reinforce_Count == 10 || Enemy_Reinforce_Count == 17) {
        goto Make_BonusEnemy;
    }
    goto exit_make_respawn;

/* ASM: Make_BonusEnemy (6221) */    
Make_BonusEnemy:
    Tank_Type[slot] = 4;
    Bonus_X = 0;

exit_make_respawn:
    Tank_Status[slot] = 0xF0;
    Block_Y = Tank_Y[slot];
    Block_X = Tank_X[slot];
    draw_tsa_block(0x0F);
}

const uint8_t Respawn_Status[8] = { 0xA0, 0xA0, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2 };

const uint8_t EnemyType_ROMArray[35 * 4] = {
    0x80, 0xA0, 0xC0, 0xE0,
    0xE0, 0xA0, 0xC0, 0x80,
    0x80, 0xA0, 0xC0, 0xE0,
    0xC0, 0xA0, 0x80, 0xE0,
    0xC0, 0xE0, 0x80, 0xA0,
    0xC0, 0xA0, 0x80, 0xE0,
    0x80, 0xA0, 0xC0, 0x80,
    0xC0, 0xE0, 0xA0, 0x80,
    0x80, 0xA0, 0xC0, 0xE0,
    0x80, 0xA0, 0xC0, 0xE0,
    0xA0, 0xE0, 0xC0, 0xA0,
    0xC0, 0xA0, 0x80, 0xE0,
    0xC0, 0xA0, 0x80, 0xE0,
    0xC0, 0xA0, 0x80, 0xE0,
    0x80, 0xC0, 0xA0, 0xE0,
    0xC0, 0xA0, 0xE0, 0xC0,
    0xE0, 0x80, 0xC0, 0xA0,
    0xA0, 0xE0, 0xC0, 0xA0,
    0xC0, 0xA0, 0x80, 0xE0,
    0xC0, 0xA0, 0x80, 0xE0,
};

/* ASM: Load_New_Tank (???) */
void load_new_tank(uint8_t slot) {
    Tank_Status[slot] = Respawn_Status[slot];
    if (slot >= 2) {
        goto load_NewEnemy;
    }

    Invisible_Timer[slot] = 3;
    uint8_t a = Player_Type[slot];
    goto checkTankType;

load_NewEnemy: /* ASM: @load_NewEnemy */
    {
        uint8_t y = Enemy_TypeNumber;
        if (Enemy_Count[y] == 0) {
            Enemy_TypeNumber++;
            goto load_NewEnemy;
        }

        Enemy_Count[y] -= 1;
        uint8_t level_idx;
        if (Level_Mode == 0) {
            level_idx = Level_Number;
            goto continueProcess;
        }

        level_idx = 35;
continueProcess: /* ASM: @continueProcess */
        level_idx -= 1;
        level_idx <<= 1;
        level_idx <<= 1;
        level_idx += Enemy_TypeNumber;
        a = EnemyType_ROMArray[level_idx];
        /* ASM: CMP #$E0; BNE @checkTankType; ORA #3
           Only the heavily-armored ($E0) enemy gets its armor-level bits set;
           inverting this condition makes EVERY enemy's Tank_Type cycle through
           palettes in operating_tank — visible as constant tank flicker. */
        if (a == 0xE0) {
            a |= 0x03;
        }
    }

checkTankType: /* ASM: @checkTankType */
    a |= Tank_Type[slot];
    if (a == 0xE7) {
        a = 0xE4;
    }

exit: /* ASM: @exit */
    Tank_Type[slot] = a;
    Track_Pos[slot] = 0;
}

void load_enemy_count(void) {
    uint8_t index_value;

    if (Level_Mode == 0) {
        goto LevelModeZero;
    }

    index_value = 35;
    goto SaveEnemyCount;

/* ASM: LevelModeZero (6349) */
LevelModeZero:
    index_value = Level_Number;

/* ASM: Save_Enemy_Count (6352) */    
SaveEnemyCount:
    index_value -= 1;
    index_value = index_value << 2;

    Enemy_Count[0] = Enemy_Amount_ROMArray[index_value + 0];
    Enemy_Count[1] = Enemy_Amount_ROMArray[index_value + 1];
    Enemy_Count[2] = Enemy_Amount_ROMArray[index_value + 2];
    Enemy_Count[3] = Enemy_Amount_ROMArray[index_value + 3];
}


