#ifndef PTS_SCREEN_H
#define PTS_SCREEN_H

#include <stdint.h>

/* ASM: Draw_Pts_Screen_Template (2611) */
void draw_pts_screen_template(void);
/* ASM: Draw_Pts_Screen (2331) */
void draw_pts_screen(void);
/* ASM: Draw_Tank_Column (2826) */
void draw_tank_column(void);
/* ASM: DrawTankColumn_XTimes (3069) */
void draw_tank_column_x_times(uint8_t count);
/* ASM: Draw_Spr_InColumn (2886). Принимает tile (=A до JSR в ASM). */
void draw_spr_in_column(uint8_t tile);
/* ASM: Fill_Attrib_Table (2848) */
void fill_attrib_table(void);

#endif // PTS_SCREEN_H
