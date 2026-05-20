#ifndef BATTLE_TANK_H
#define BATTLE_TANK_H

#include <stdint.h>


/* ASM: Detect_Motion (4556) */
uint8_t detect_motion(uint8_t slot);
/* ASM: Check_BorderReach (1662) */
void check_border_reach(uint8_t slot);
/* ASM: Ice_Detect (5813) */
void ice_detect(uint8_t slot);
/* ASM: Ice_Move (4611) */
void ice_move(uint8_t slot);
/* ASM: Motion_Handle (4698) */
void motion_handle(void);
/* ASM: Invisible_Timer_Handle (5995) */
void invisible_timer_handle(uint8_t player_slot);
/* ASM: Rise_Nt_HighBit (5885) */
void rise_nt_high_bit(void);
/* ASM: HideHiBit_Under_Tank (5895) */
void hide_hi_bit_under_tank(uint8_t slot);
/* ASM: HideHiBit_InBuffer (5936) */
void hide_hi_bit_in_buffer(uint8_t slot);
/* ASM: Button_To_DirectionIndex (6373) */
uint8_t button_to_direction_index(uint8_t buttons);
/* ASM: Null_Status (XXXX) */
void null_status(void);
/* ASM: Rise_TankStatus_Bit (6331) */
void rise_tank_status_bit(uint8_t slot);

#endif // BATTLE_TANK_H
