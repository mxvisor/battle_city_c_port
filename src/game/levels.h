#ifndef LEVELS_H
#define LEVELS_H

#include <stdint.h>

/* Number of levels: 35 normal + 1 demo */
#define LEVEL_COUNT     36
/* Size of one level data block in bytes (0x5B) */
#define LEVEL_SIZE      0x5B
/* Maximum level number for normal play */
#define LEVEL_MAX       0x24

/* ASM: Level_Data — flat concatenation of all level binaries */
extern const uint8_t Level_Data[LEVEL_COUNT * LEVEL_SIZE];

/* ASM: Load_Level (7888) */
void load_level(uint8_t level_num);

#endif // LEVELS_H
