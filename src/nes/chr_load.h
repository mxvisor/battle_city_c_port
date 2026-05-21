#ifndef CHR_H
#define CHR_H

#include <stdint.h>
#include <stddef.h>

int load_chr(const char *path);
int load_chr_bmp(const char *path);
int load_chr_bmp_mem(const uint8_t *data, size_t size, const char *label);

#endif // CHR_H
