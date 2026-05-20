#ifndef POWERON_RANDOMIZE_H
#define POWERON_RANDOMIZE_H

#include <stdint.h>

void bss_poweron_randomize(uint32_t *state);
void zeropage_poweron_randomize(uint32_t *state);

#endif /* POWERON_RANDOMIZE_H */
