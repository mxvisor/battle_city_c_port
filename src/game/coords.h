#ifndef COORDS_H
#define COORDS_H

#include <stdint.h>

/* ASM: GetCoord_InTiles (3670).
 * Вход через CPU X/Y регистры в ASM → в C принимаем как параметры.
 * `JSR XnY_div_8` + fallthrough в CoordsToRAMPos. Output: HighPtr/LowPtr. */
void get_coord_in_tiles_xy(uint8_t x, uint8_t y);
/* ASM: Get_SprCoord_InTiles (3709) / GetSprCoord_InTiles (6652).
 * STX Spr_X; STY Spr_Y — стартовый префикс ASM-функции; fallthrough в
 * GetCoord_InTiles. В C: записывает Spr_X/Spr_Y и делает то же что get_coord_in_tiles_xy. */
void get_spr_coord_in_tiles(uint8_t x, uint8_t y);
/* ASM: XnY_div_8 (3689) */
void xny_div_8(uint8_t *x, uint8_t *y);
/* ASM: Temp_Coord_shl (3719) */
void temp_coord_shl(void);
/* ASM: CoordsToRAMPos (3677) */
void coords_to_ram_pos(uint8_t x, uint8_t y);
/* ASM: CoordTo_PPUaddress (3459) */
uint16_t coord_to_ppu_address(uint8_t x, uint8_t y);
/* ASM: CurPos_To_PixelCoord (1962) */
void cur_pos_to_pixel_coord(uint8_t pos);

#endif // COORDS_H
