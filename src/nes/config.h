#ifndef NES_CONFIG_H
#define NES_CONFIG_H

#include <stdint.h>

typedef enum {
    REGION_NTSC,
    REGION_PAL,
    REGION_COUNT
} Region;

extern Region current_region;
extern int apu_filters_enabled;  /* 1 = NES-accurate HPF/LPF chain, 0 = simple DC removal */

#define FRAME_PERIOD_NTSC_NS 16639270L
#define FRAME_PERIOD_PAL_NS  19997200L

#define NES_SCREEN_W 256
#define NES_SCREEN_H 240
#define NES_SCREEN_TOTAL (NES_SCREEN_W * NES_SCREEN_H)

/* All region-dependent timing/data lives here, indexed by Region.
   Read via `region_params(current_region)`. */
typedef struct {
    uint32_t       cpu_clock_hz;        /* CPU master clock */
    float          frame_seq_hz;        /* APU frame counter step rate (240 or 200) */
    uint64_t       frame_period_ns;     /* one PPU frame in ns */
    int            samples_per_frame;   /* audio samples per video frame at 44.1 kHz */
    const uint16_t *noise_period;       /* 16-entry APU noise period table (CPU cycles) */
} RegionParams;

const RegionParams *region_params(Region r);

uint64_t frame_period_ns(void);
int samples_per_frame(void);

extern const uint32_t nes_palette[64];

void nes_init(const char **chr_paths);
int nes_thread(void *unused);

#endif
