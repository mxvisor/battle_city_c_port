#ifndef APU_SIM_H
#define APU_SIM_H

#include <stdint.h>

void apu_init(void);
void apu_write(uint8_t reg, uint8_t value);
void apu_write_status(uint8_t value);
void apu_write_frame(uint8_t value);
void audio_callback(void *user, uint8_t *stream, int len);

#endif // APU_H
