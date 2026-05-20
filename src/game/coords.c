#include "coords.h"
#include "zeropage.h"

/* ASM: GetCoord_InTiles (3670).
   Прямая ASM-точка входа: `JSR XnY_div_8` + fallthrough в `CoordsToRAMPos`.
   Вход в ASM через CPU X/Y регистры — в C через параметры. Spr_X/Spr_Y не трогаются
   (соответствует ASM, где они меняются только в Get_SprCoord_InTiles префиксом). */
void get_coord_in_tiles_xy(uint8_t x, uint8_t y) {
    /* JSR XnY_div_8 */
    xny_div_8(&x, &y);
    /* fallthrough → CoordsToRAMPos */
    coords_to_ram_pos(x, y);
}

/* ASM: Get_SprCoord_InTiles (3709) / GetSprCoord_InTiles (6652).
   Дублирующая ASM-точка входа с STX Spr_X; STY Spr_Y префиксом, далее
   fallthrough в GetCoord_InTiles. */
void get_spr_coord_in_tiles(uint8_t x, uint8_t y) {
    /* STX Spr_X */
    Spr_X = x;
    /* STY Spr_Y */
    Spr_Y = y;
    /* JSR GetCoord_InTiles (вызов с теми же значениями, т.к. STX/STY не трогают регистры) */
    get_coord_in_tiles_xy(x, y);
}

void xny_div_8(uint8_t *x, uint8_t *y) {
    /* ASM: XnY_div_8 (3689)
       TYA; LSR LSR LSR; TAY  — Y / 8
       TXA; LSR LSR LSR; TAX  — X / 8 */
    *y = (uint8_t)(*y >> 3u);
    *x = (uint8_t)(*x >> 3u);
}

/* ASM: Temp_Coord_shl (3719)
   Temp = sub-tile bitmask по бит-2 от Spr_Y и Spr_X:
     bit2(Spr_Y)=0, bit2(Spr_X)=0 → Temp=1 (top-left)
     bit2(Spr_Y)=0, bit2(Spr_X)=1 → Temp=2 (top-right)
     bit2(Spr_Y)=1, bit2(Spr_X)=0 → Temp=4 (bottom-left)
     bit2(Spr_Y)=1, bit2(Spr_X)=1 → Temp=8 (bottom-right) */
void temp_coord_shl(void) {
    /* LDA #1; STA Temp */
    Temp = 1u;
    /* LDA Spr_Y; AND #4; BEQ @_ */
    if ((Spr_Y & 4u) == 0u) goto at_;
    /* ASL Temp; ASL Temp */
    Temp = (uint8_t)(Temp << 1u);
    Temp = (uint8_t)(Temp << 1u);

at_:
    /* LDA Spr_X; AND #4; BEQ @__ */
    if ((Spr_X & 4u) == 0u) goto at__;
    /* ASL Temp */
    Temp = (uint8_t)(Temp << 1u);

at__:
    return;
}


void coords_to_ram_pos(uint8_t x, uint8_t y) {
    /* ASM: CoordsToRAMPos (3677)
       JSR CoordTo_PPUaddress
       STA HighPtr_Byte; STY LowPtr_Byte; LDY #0 */
    uint16_t addr = coord_to_ppu_address(x, y);
    HighPtr_Byte = (uint8_t)(addr >> 8);
    LowPtr_Byte  = (uint8_t)addr;
}

/* ASM: CoordTo_PPUaddress (3459).
   Три младших бита Y вкручиваются в три старших бита Temp через 3× LSR A/ROR Temp.
   Low byte  = X | Temp  = X | ((Y & 7) << 5)
   High byte = A | #4    = (Y >> 3) | 4
   Возврат: A=high, Y=low (в C — упакованный uint16). Побочный эффект на Temp
   сохранён 1-в-1 с ASM (`STA Temp` в начале, `ROR Temp` 3 раза). */
uint16_t coord_to_ppu_address(uint8_t x, uint8_t y) {
    uint8_t a = y;
    uint8_t carry;
    uint8_t low;
    uint8_t high;

    /* LDA #0; STA Temp */
    Temp = 0u;
    /* TYA */
    /* LSR A; ROR Temp ×3 */
    carry = a & 1u; a >>= 1u; Temp = (uint8_t)((Temp >> 1) | (carry << 7));
    carry = a & 1u; a >>= 1u; Temp = (uint8_t)((Temp >> 1) | (carry << 7));
    carry = a & 1u; a >>= 1u; Temp = (uint8_t)((Temp >> 1) | (carry << 7));
    /* PHA; TXA; ORA Temp; TAY */
    low = (uint8_t)(x | Temp);
    /* PLA; ORA #4 */
    high = (uint8_t)(a | 0x04u);
    return ((uint16_t)high << 8) | low;
}
void cur_pos_to_pixel_coord(uint8_t pos) {
    Tank_Y[0] = (pos << 4) + 0x8B;
}
