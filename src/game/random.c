#include "random.h"
#include "zeropage.h"

/* Псевдо-zero-page 256 байт для индексированного доступа `Temp,X` в ASM.
 * Полный union по zp в этом порте не сделан; этот буфер заменяет реальную
 * NES-zeropage только для PRNG. Сидируется детерминированно — этого достаточно,
 * т.к. ASM-алгоритм всё равно опирается не на конкретные значения, а на наличие
 * меняющейся «фоновой» энтропии на чужих zp-байтах. */
static uint8_t zp_bytes[256];
static uint8_t zp_initialized;

static void init_zp_bytes(void) {
    /* Произвольный нестабильный сид (на реальном NES — мусор после reset). */
    for (int i = 0; i < 256; i++) {
        zp_bytes[i] = (uint8_t)((i * 31u) + 17u);
    }
    zp_initialized = 1u;
}

/* ASM: Get_Random_A (3226).
 * TXA; PHA — сохранение X (в C не требуется).
 * Алгоритм: Random_Lo = (Random_Lo * 7) + Seconds_Counter + zp[Random_Hi],
 * где X = Random_Hi и `ADC Temp,X` это чтение байта zero-page по этому смещению.
 */
uint8_t get_random_a(void) {
    uint8_t val;
    if (zp_initialized == 0u) init_zp_bytes();

    /* LDA Random_Lo; ASL; ASL; ASL — A = Random_Lo*8 */
    val = Random_Lo;
    val = (uint8_t)(val << 3u);
    /* SEC; SBC Random_Lo — A = (Random_Lo*8) - Random_Lo = Random_Lo*7 */
    val = (uint8_t)(val - Random_Lo);
    /* CLC; ADC Seconds_Counter */
    val = (uint8_t)(val + Seconds_Counter);
    /* INC Random_Hi; LDX Random_Hi; ADC Temp,X */
    Random_Hi++;
    val = (uint8_t)(val + zp_bytes[Random_Hi]);
    /* STA Random_Lo; PLA; TAX; LDA Random_Lo; RTS */
    Random_Lo = val;
    return Random_Lo;
}
