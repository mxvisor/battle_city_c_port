#ifndef RESET_H
#define RESET_H


#include <stdint.h>


/* ASM: Reset_ScreenStuff (3293) */
void reset_screen_stuff(void);

/* ASM: RESET (3321) */
void reset(void);


/* ASM: StaffStr_Store (3340) */
void staff_str_store(void);
/* ASM: StaffStr_Check (3356) */
uint8_t staff_str_check(void);

/* ASM: Load_Pals (3376) */
void load_pals(void);

#endif // RESET_H
