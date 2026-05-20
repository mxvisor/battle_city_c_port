#ifndef BSS_H
#define BSS_H

#include <stdint.h>

extern uint8_t EnemyFreeze_Timer;
extern uint8_t Player_Type[2];
extern uint8_t Player_Ice_Status[2];
extern uint8_t GameOverStr_X;
extern uint8_t GameOverStr_Y;
extern uint8_t GameOverScroll_Type;
extern uint8_t GameOverStr_Timer;
extern uint8_t ZeroPage_Offset;
extern uint8_t StaffString_RAM[16];
extern uint8_t Screen_Buffer[128];
extern uint8_t SprBuffer[256];
extern uint8_t Sound_PlaybackState[28];
/* Snd_* flags are aliases of Sound_PlaybackState[4..27] (NES BSS layout) */
#define Snd_Ancillary_Life1    Sound_PlaybackState[4]
#define Snd_Ancillary_Life2    Sound_PlaybackState[5]
#define Snd_BonusTaken         Sound_PlaybackState[6]
#define Snd_PlayerExplode      Sound_PlaybackState[7]
#define Snd_Unknown1           Sound_PlaybackState[8]
#define Snd_BonusAppears       Sound_PlaybackState[9]
#define Snd_EnemyExplode       Sound_PlaybackState[10]
#define Snd_HQExplode          Sound_PlaybackState[11]
#define Snd_Brick_Ricochet     Sound_PlaybackState[12]
#define Snd_ArmourRicochetWall Sound_PlaybackState[13]
#define Snd_ArmourRicochetTank Sound_PlaybackState[14]
#define Snd_Shoot              Sound_PlaybackState[15]
#define Snd_Ice                Sound_PlaybackState[16]
#define Snd_Move               Sound_PlaybackState[17]
#define Snd_Engine             Sound_PlaybackState[18]
#define Snd_PtsCount1          Sound_PlaybackState[19]
#define Snd_PtsCount2          Sound_PlaybackState[20]
#define Snd_RecordPts1         Sound_PlaybackState[21]
#define Snd_RecordPts2         Sound_PlaybackState[22]
#define Snd_RecordPts3         Sound_PlaybackState[23]
#define Snd_GameOver1          Sound_PlaybackState[24]
#define Snd_GameOver2          Sound_PlaybackState[25]
#define Snd_GameOver3          Sound_PlaybackState[26]
#define Snd_BonusPts           Sound_PlaybackState[27]
extern uint8_t Sound_DataBlocks[28 * 8];
extern uint8_t Sound_Tmp_Unused;
extern uint8_t NT_Buffer[1024];

#endif // BSS_H
