#include "bss.h"

uint8_t EnemyFreeze_Timer;
uint8_t Player_Type[2];
uint8_t Player_Ice_Status[2];
uint8_t GameOverStr_X;
uint8_t GameOverStr_Y;
uint8_t GameOverScroll_Type;
uint8_t GameOverStr_Timer;
uint8_t ZeroPage_Offset;
uint8_t StaffString_RAM[16];
uint8_t Screen_Buffer[128];
uint8_t SprBuffer[256];
uint8_t Sound_PlaybackState[28];
uint8_t Sound_DataBlocks[28 * 8];
uint8_t Sound_Tmp_Unused;
uint8_t NT_Buffer[1024];
