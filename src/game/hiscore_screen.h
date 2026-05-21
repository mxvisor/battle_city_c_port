#ifndef HISCORE_H
#define HISCORE_H

#include <stdint.h>

/* ASM: Null_both_HiScore (656) */
void null_both_hi_score(void);
/* ASM: Draw_Record_HiScore (908) */
void draw_record_hi_score(void);
/* ASM: Update_HiScore (4198) — возвращает Y: 0/1=1P record/$FF=2P record */
uint8_t update_hi_score(void);

#endif // HISCORE_H
