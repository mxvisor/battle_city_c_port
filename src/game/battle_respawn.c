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

/* ASM: Respawn_Handle (6155). Запускает респаун следующего врага из резерва. */
void respawn_handle(uint8_t unused) {
    (void)unused;
    /* LDA Respawn_Timer; BEQ @_ */
    if (Respawn_Timer == 0u) goto at_;
    /* DEC Respawn_Timer; RTS */
    Respawn_Timer = (uint8_t)(Respawn_Timer - 1u);
    return;

at_: /* ASM: @_ */
    /* LDA Enemy_Reinforce_Count; BEQ End_Respawn_Handle */
    if (Enemy_Reinforce_Count == 0u) goto End_Respawn_Handle;
    Counter = TanksOnScreen;

nextPlayer:
    /* LDX Counter; LDA Tank_Status,X; BNE __ */
    if (Tank_Status[Counter] != 0u) goto at__;
    Respawn_Timer = Respawn_Delay;
    make_respawn(Counter);
    Enemy_Reinforce_Count = (uint8_t)(Enemy_Reinforce_Count - 1u);
    draw_empty_tile();
    return;

at__: /* ASM: __ */
    Counter = (uint8_t)(Counter - 1u);
    if (Counter != 1u) goto nextPlayer;

End_Respawn_Handle:
    return;
}

/* ASM: Make_Respawn (6221). Инициализирует Tank_X/Y/Status для нового танка
 * (игрока или врага), с возможностью бонусного танка при Reinforce_Count==3/10/17. */
void make_respawn(uint8_t x) {
    Tank_Type[x] = 0u;
    /* CPX #2; BCS Enemy_Operations */
    if (x >= 2u) goto Enemy_Operations;
    /* Игрок */
    Tank_X[x] = X_Player_Respawn[x];
    Tank_Y[x] = Y_Player_Respawn[x];
    Player_Blink_Timer[x] = 0u;
    goto exit_;

Enemy_Operations:
    /* INC EnemyRespawn_PlaceIndex; LDY EnemyRespawn_PlaceIndex; CPY #3; BNE @_ */
    EnemyRespawn_PlaceIndex = (uint8_t)(EnemyRespawn_PlaceIndex + 1u);
    if (EnemyRespawn_PlaceIndex != 3u) goto at_;
    EnemyRespawn_PlaceIndex = 0u;

at_: /* ASM: @_ */
    Tank_X[x] = X_Enemy_Respawn[EnemyRespawn_PlaceIndex];
    Tank_Y[x] = Y_Enemy_Respawn[EnemyRespawn_PlaceIndex];
    /* CMP #3 / #10 / #17 */
    if (Enemy_Reinforce_Count == 3u)  goto Make_BonusEnemy;
    if (Enemy_Reinforce_Count == 10u) goto Make_BonusEnemy;
    if (Enemy_Reinforce_Count != 17u) goto exit_;

Make_BonusEnemy:
    Tank_Type[x] = 4u;
    Bonus_X = 0u;

exit_:
    Tank_Status[x] = 0xF0u;
    Block_Y = Tank_Y[x];
    Block_X = Tank_X[x];
    draw_tsa_block(0x0Fu);
}

const uint8_t Respawn_Status[8] = { 0xA0, 0xA0, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2 };

/* ASM: EnemyType_ROMArray (6477). 35 levels x 4 enemy types.
 * C-порт раньше имел только 20 строк (+ строки 16-20 содержали
 * данные не из своих позиций), что приводило к зависанию/некорректным
 * врагам на уровнях 21+ (особенно уровень 28, где 15 врагов типа 3). */
const uint8_t EnemyType_ROMArray[35 * 4] = {
    0x80, 0xA0, 0xC0, 0xE0,  /* 1  */
    0xE0, 0xA0, 0xC0, 0x80,  /* 2  */
    0x80, 0xA0, 0xC0, 0xE0,  /* 3  */
    0xC0, 0xA0, 0x80, 0xE0,  /* 4  */
    0xC0, 0xE0, 0x80, 0xA0,  /* 5  */
    0xC0, 0xA0, 0x80, 0xE0,  /* 6  */
    0x80, 0xA0, 0xC0, 0x80,  /* 7  */
    0xC0, 0xE0, 0xA0, 0x80,  /* 8  */
    0x80, 0xA0, 0xC0, 0xE0,  /* 9  */
    0x80, 0xA0, 0xC0, 0xE0,  /* 10 */
    0xA0, 0xE0, 0xC0, 0xA0,  /* 11 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 12 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 13 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 14 */
    0x80, 0xC0, 0xA0, 0xE0,  /* 15 */
    0x80, 0xC0, 0xA0, 0xE0,  /* 16 */
    0xE0, 0xA0, 0xC0, 0x80,  /* 17 */
    0xE0, 0x80, 0xC0, 0xA0,  /* 18 */
    0xA0, 0xE0, 0x80, 0xC0,  /* 19 */
    0xA0, 0x80, 0xC0, 0xE0,  /* 20 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 21 */
    0xA0, 0x80, 0xC0, 0xE0,  /* 22 */
    0xE0, 0x80, 0xC0, 0xA0,  /* 23 */
    0xC0, 0xE0, 0xA0, 0x80,  /* 24 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 25 */
    0xA0, 0xE0, 0x80, 0xC0,  /* 26 */
    0xC0, 0xE0, 0xA0, 0x80,  /* 27 */
    0xA0, 0xE0, 0x80, 0xC0,  /* 28 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 29 */
    0x80, 0xA0, 0xC0, 0xE0,  /* 30 */
    0xC0, 0xA0, 0xE0, 0xC0,  /* 31 */
    0xE0, 0x80, 0xC0, 0xA0,  /* 32 */
    0xA0, 0xE0, 0xC0, 0xA0,  /* 33 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 34 */
    0xC0, 0xA0, 0x80, 0xE0,  /* 35 (and Demo-level) */
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


