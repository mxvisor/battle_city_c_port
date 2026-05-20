#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

/* ASM: Draw_Tile (3821) */
void draw_tile(void);
/* ASM: Draw_TSABlock (3928) */
void draw_tsa_block(uint8_t block_num);
/* ASM: DrawPtrTile (3781) */
void draw_ptr_tile(void);
/* ASM: Inc_Ptr_on_A (3847) */
void inc_ptr_on_a(uint8_t a);
/* ASM: Draw_Char (3974) */
void draw_char(uint8_t char_index);
/* ASM: Draw_BrickStr (4052) */
void draw_brick_str(const uint8_t *str);
/* ASM: NT_Buffer_Process_XOR (3764) */
void nt_buffer_process_xor(uint8_t value);
/* ASM: NT_Buffer_Process_OR (3791) */
void nt_buffer_process_or(uint8_t value);
/* ASM: Fill_NTBuffer (3886) */
void fill_nt_buffer(uint8_t value);
/* ASM: Fill_NTAttribBuffer (3896) */
void fill_nt_attrib_buffer(uint8_t value);
/* ASM: Null_Upper_NT (2933) */
void null_upper_nt(void);
/* ASM: Null_NT_Buffer (3276) */
void null_nt_buffer(void);
/* ASM: Clear_NT (627) */
void clear_nt(void);
/* ASM: FillScr_Single_Row (2242) */
void fill_scr_single_row(uint8_t value);
/* ASM: FillNT_with_Grey (2280) */
void fill_nt_with_grey(void);
/* ASM: FillNT_with_Black (2306) */
void fill_nt_with_black(void);
/* ASM: Draw_BlackRow (3906) */
void draw_black_row(void);
/* ASM: Draw_GrayFrame (3882) */
void draw_gray_frame(void);
/* ASM: Make_GrayFrame (1813) */
void make_gray_frame(void);
/* ASM: Copy_AttribToScrnBuff (2206) */
void copy_attrib_to_scrn_buff(void);
/* ASM: AttribToScrBuffer (3482) */
void attrib_to_scr_buffer(void);
/* ASM: TSA_Pal_Ops (3506) */
uint8_t tsa_pal_ops(uint8_t x, uint8_t y);
/* ASM: OR_Pal (3558) */
uint8_t or_pal(uint8_t a);
/* ASM: Set_PPU (3254) */
void set_ppu(void);
/* ASM: Store_NT_Buffer_InVRAM (3862) */
void store_nt_buffer_in_vram(void);
/* ASM: Save_To_VRAM (3875) */
void save_to_vram(void);
/* ASM: Screen_Off (3264) */
void screen_off(void);
/* ASM: SaveSprTo_SprBuffer (4356) */
void save_spr_to_spr_buffer(uint8_t x, uint8_t y);
/* ASM: Indexed_SaveSpr (4396) */
void indexed_save_spr(uint8_t direction, uint8_t x, uint8_t y);
/* ASM: Spr_TileIndex_Add (4413) */
void spr_tile_index_add(uint8_t direction);
/* ASM: Draw_WholeSpr (4426) */
void draw_whole_spr(void);
/* ASM: Spr_Invisible (4446) */
void spr_invisible(void);


/* ASM: String_to_Screen_Buffer (3602) */
void string_to_screen_buffer(uint8_t x, uint8_t y, const uint8_t *str);
/* ASM: Save_Str_To_ScrBuffer (3635) */
void save_str_to_scr_buffer(uint8_t x, uint8_t y, const uint8_t *str);
/* ASM: PtrToNonzeroStrElem */
uint8_t* ptr_to_nonzero_str_elem(uint8_t *str);
/* Right-aligns a numeric string: walks past leading zeros and shifts the
 * destination column by the same number of positions skipped (matches the
 * ASM convention where PtrToNonzeroStrElem advanced both Y (str ptr) and
 * X (column) in lockstep). */
void save_aligned_str_to_scr_buffer(uint8_t col, uint8_t row, uint8_t *base_str);
/* ASM: Num_To_NumString (4291) */
void num_to_num_string(uint8_t value);
/* ASM: ByteTo_Num_String (4333) */
void byte_to_num_string(uint8_t value);

/* ASM: Null_8Bytes_String (664) */
void null_8bytes_string(uint8_t *str);

/* ASM: Zero_Page_Viewer (1395) */
void zero_page_viewer(void);



/* Register the PPU write backend (called once at startup from outside game/) */
void ppu_data_write(uint16_t addr, uint8_t val);
uint8_t ppu_status_read(void);
void ppu_set_vblank_flag(void);


#endif // DRAW_H
