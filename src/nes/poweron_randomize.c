#include "poweron_randomize.h"
#include <stddef.h>
#include <stdint.h>
#include "game/bss.h"
#include "game/zeropage.h"

static uint8_t next_rand8(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return (uint8_t)(x & 0xFF);
}

static void randomize_bytes(uint8_t *dst, size_t len, uint32_t *state) {
    for (size_t i = 0; i < len; i++) {
        dst[i] = next_rand8(state);
    }
}

void bss_poweron_randomize(uint32_t *state) {
    randomize_bytes(&EnemyFreeze_Timer, 1, state);
    randomize_bytes(Player_Type, sizeof(Player_Type), state);
    randomize_bytes(Player_Ice_Status, sizeof(Player_Ice_Status), state);
    randomize_bytes(&GameOverStr_X, 1, state);
    randomize_bytes(&GameOverStr_Y, 1, state);
    randomize_bytes(&GameOverScroll_Type, 1, state);
    randomize_bytes(&GameOverStr_Timer, 1, state);
    randomize_bytes(&ZeroPage_Offset, 1, state);
    randomize_bytes(StaffString_RAM, sizeof(StaffString_RAM), state);
    randomize_bytes(Screen_Buffer, sizeof(Screen_Buffer), state);
    randomize_bytes(SprBuffer, sizeof(SprBuffer), state);
    randomize_bytes(Sound_PlaybackState, sizeof(Sound_PlaybackState), state);
    randomize_bytes(Sound_DataBlocks, sizeof(Sound_DataBlocks), state);
    randomize_bytes(&Sound_Tmp_Unused, 1, state);
    randomize_bytes(NT_Buffer, sizeof(NT_Buffer), state);
}

void zeropage_poweron_randomize(uint32_t *state) {
#define R1(v) randomize_bytes(&(v), 1, state)
#define RA(v) randomize_bytes((v), sizeof(v), state)
    R1(Temp);
    R1(Tmp_Pal);
    R1(CHR_Byte);
    R1(Mask_CHR_Byte);
    R1(TSA_Pal);
    R1(PPU_Addr_Ptr);
    R1(Joypad1_Buttons);
    R1(Joypad2_Buttons);
    R1(Joypad1_Differ);
    R1(Joypad2_Differ);
    R1(Seconds_Counter);
    R1(Frame_Counter);
    R1(ScrBuffer_Pos);
    R1(SprBuffer_Position);
    R1(Gap);
    R1(Random_Lo);
    R1(Random_Hi);
    R1(LowPtr_Byte);
    R1(HighPtr_Byte);
    R1(LowStrPtr_Byte);
    R1(HighStrPtr_Byte);
    RA(HiScore_1P_String);
    RA(HiScore_2P_String);
    RA(Temp_1PPts_String);
    RA(Temp_2PPts_String);
    RA(Num_String);
    RA(HiScore_String);
    R1(HQArmour_Timer);
    R1(Level_Mode);
    R1(Spr_X);
    R1(Spr_Y);
    R1(Tank_Num);
    R1(Joy_Counter);
    R1(Construction_Flag);
    R1(EnterGame_Flag);
    R1(BkgPal_Number);
    R1(Scroll_Byte);
    R1(PPU_REG1_Stts);
    R1(Player1_Lives);
    R1(Player2_Lives);
    R1(Spr_TileIndex);
    R1(Temp_X);
    R1(Temp_Y);
    R1(Block_X);
    R1(Block_Y);
    R1(Tmp_Status1);
    R1(Tmp_Status2);
    R1(Counter);
    R1(Counter2);
    R1(TSA_BlockNumber);
    R1(BrickChar_X);
    R1(BrickChar_Y);
    R1(String_Position);
    R1(Char_Index_Base);
    R1(BonusPts_TimeCounter);
    R1(Iterative_Byte);
    R1(AI_X_DifferFlag);
    R1(AI_Y_DifferFlag);
    RA(AddLife_Flag);
    R1(HQ_Status);
    R1(HQExplode_SprBase);
    R1(EnemyRespawn_PlaceIndex);
    R1(Tmp_CharIndexBase);
    R1(TanksOnScreen);
    R1(Pause_Flag);
    R1(Spr_Attrib);
    RA(Player_Blink_Timer);
    R1(AI_X_Aim);
    R1(AI_Y_Aim);
    RA(Enmy_KlledBy1P_Count);
    RA(Enmy_KlledBy2P_Count);
    R1(Joypad_Delay);
    R1(EndCount_Flag);
    R1(TotalEnmy_KilledBy1P);
    R1(TotalEnmy_KilledBy2P);
    R1(Enemy_Reinforce_Count);
    R1(Enemy_Counter);
    RA(Enemy_Count);
    R1(Enemy_TypeNumber);
    RA(Tank_X);
    RA(Tank_Y);
    RA(Tank_Status);
    RA(Tank_Type);
    RA(Track_Pos);
    RA(Bullet_X);
    RA(Bullet_Y);
    RA(Bullet_Status);
    RA(Bullet_Property);
    RA(NTAddr_Coord_Lo);
    RA(NTAddr_Coord_Hi);
    RA(Sound_CurrentData_Ptr);
    RA(Sound_DataPtr);
    R1(Sound_CurrentSlot);
    R1(Sound_NumberOfSlots);
    R1(Sound_LoopCounter0);
    R1(Sound_LoopCounter1);
    R1(Sound_LoopCounter2);
    RA(SoundChannels);
    R1(Sound_Temp);
    R1(Sound_Temp2);
#undef R1
#undef RA
}
