#ifndef CHR_H
#define CHR_H

#include <stdint.h>
#include <stddef.h>

int load_chr(const char *path);
int load_chr_bmp(const char *path);
int load_chr_bmp_mem(const uint8_t *data, size_t size, const char *label);
uint8_t* gets_chr_ptr(void);
uint8_t get_blank_tile_idx(void);
uint16_t get_bg_bank_offset(void);

#endif // CHR_H
