#include "config.h"
#include "chr_load.h"
#include "poweron_randomize.h"
#include "game/nmi.h"
#include "game/begin.h"
#include "game/reset.h"
#include <stdbool.h>
#include <stdio.h>

Region current_region = REGION_NTSC;
int apu_filters_enabled = 0;  /* default: simple DC removal HPF */

/* APU_NOISE.md §Registers — period table per region (CPU cycles between LFSR clocks). */
static const uint16_t noise_period_ntsc[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};
static const uint16_t noise_period_pal[16] = {
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};

static const RegionParams k_region_params[REGION_COUNT] = {
    [REGION_NTSC] = {
        .cpu_clock_hz      = 1789773u,
        .frame_seq_hz      = 240.0f,
        .frame_period_ns   = FRAME_PERIOD_NTSC_NS,
        .samples_per_frame = 734,
        .noise_period      = noise_period_ntsc,
    },
    [REGION_PAL] = {
        .cpu_clock_hz      = 1662607u,
        .frame_seq_hz      = 200.0f,
        .frame_period_ns   = FRAME_PERIOD_PAL_NS,
        .samples_per_frame = 882,
        .noise_period      = noise_period_pal,
    },
};

const RegionParams *region_params(Region r) {
    if ((unsigned)r >= REGION_COUNT) r = REGION_NTSC;
    return &k_region_params[r];
}

uint64_t frame_period_ns(void) {
    return region_params(current_region)->frame_period_ns;
}

int samples_per_frame(void) {
    return region_params(current_region)->samples_per_frame;
}

const uint32_t nes_palette[64] = {
    0xFF7C7C7C, 0xFF0000FC, 0xFF0000BC, 0xFF4428BC, 0xFF940084, 0xFFA80020, 0xFFA81000, 0xFF881400,
    0xFF503000, 0xFF007800, 0xFF006800, 0xFF005800, 0xFF004058, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFF0078F8, 0xFF0058F8, 0xFF6844FC, 0xFFD800CC, 0xFFE40058, 0xFFF83800, 0xFFE45C10,
    0xFFAC7C00, 0xFF00B800, 0xFF00A800, 0xFF00A844, 0xFF008888, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFF8F8F8, 0xFF3CBCFC, 0xFF6888FC, 0xFF9878F8, 0xFFF878F8, 0xFFF85898, 0xFFF87858, 0xFFFCA044,
    0xFFF8B800, 0xFFB8F818, 0xFF58D854, 0xFF58F898, 0xFF00E8D8, 0xFF787878, 0xFF000000, 0xFF000000,
    0xFFFCFCFC, 0xFFA4E4FC, 0xFFB8B8F8, 0xFFD8B8F8, 0xFFF8B8F8, 0xFFF8A4C0, 0xFFF0D0B0, 0xFFFCE0A8,
    0xFFF8D878, 0xFFD8F878, 0xFFB8F8B8, 0xFFB8F8D8, 0xFF00FCFC, 0xFFF8D8F8, 0xFF000000, 0xFF000000
};

void nes_init(const char **chr_paths) {
    uint32_t seed = 0x1A2B3C4D;
    zeropage_poweron_randomize(&seed);
    bss_poweron_randomize(&seed);

    bool chr_loaded = false;
    if (chr_paths) {
        for (const char **p = chr_paths; *p != NULL; ++p) {
            if (load_chr(*p)) {
                chr_loaded = true;
                break;
            }
        }
    }
    if (!chr_loaded) {
        fprintf(stderr, "Failed to load CHR data\n");
    }
}



int nes_thread(void *unused) {
    (void)unused;
    reset();
    if (setjmp(game_exit_buf) == 0) {
        begin();
    }
    return 0;
}

