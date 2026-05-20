#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/* ASM: Sound_Stop (7315) */
void sound_stop(void);
/* ASM: Play_Sound (7349) */
void play_sound(void);

/* ASM: Load_Snd_Ptr */
void load_snd_ptr(void);
/* ASM: Sound_LoadNextByte */
uint8_t sound_load_next_byte(void);
/* ASM: Sound_DispatchCommand */
bool sound_dispatch_command(uint8_t cmd_idx);

/* ASM: Sound_Command_StopReset */
bool sound_command_stop_reset(void);
/* ASM: Sound_Command_SetDutyCycle */
bool sound_command_set_duty_cycle(void);
/* ASM: Sound_Command_SetVolume */
bool sound_command_set_volume(void);
/* ASM: Sound_Command_SetVolumeAlt */
bool sound_command_set_volume_alt(void);
/* ASM: Sound_Command_SetSweep */
bool sound_command_set_sweep(void);
/* ASM: Sound_Command_SetTimerHigh */
bool sound_command_set_timer_high(void);
/* ASM: Sound_Command_SetDutyVolume */
bool sound_command_set_duty_volume(void);
/* ASM: Sound_Command_ClearCounters */
bool sound_command_clear_counters(void);
/* ASM: Sound_Command_LoopCount0 */
bool sound_command_loop_count0(void);
/* ASM: Sound_Command_LoopCount1 */
bool sound_command_loop_count1(void);
/* ASM: Sound_Command_LoopCount2 */
bool sound_command_loop_count2(void);
/* ASM: Sound_Command_AdvancePointer */
bool sound_command_advance_pointer(void);
/* ASM: Sound_Command_JumpToOffset */
bool sound_command_jump_to_offset(void);

void apu_write(uint8_t reg, uint8_t value);
void apu_write_status(uint8_t value);

#endif // SOUND_ENGINE_H
