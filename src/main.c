#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes/config.h"
#include "sdl_init.h"
#include "sdl_run.h"
#include "test_mode.h"

int main(int argc, char **argv) {
    current_region = REGION_NTSC;
    unsigned scale = 1;
    unsigned test_frames = 0, test_every = 0;
    const char *test_input = NULL, *test_out = NULL, *test_dump = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--region") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "pal") == 0)
                current_region = REGION_PAL;
            else if (strcmp(argv[i+1], "ntsc") == 0)
                current_region = REGION_NTSC;
        } else if (strcmp(argv[i], "--apu-filters") == 0) {
            apu_filters_enabled = 1;
        } else if (strcmp(argv[i], "--test-frames") == 0 && i + 1 < argc) {
            test_frames = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--test-every") == 0 && i + 1 < argc) {
            test_every = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--test-input") == 0 && i + 1 < argc) {
            test_input = argv[++i];
        } else if (strcmp(argv[i], "--test-out") == 0 && i + 1 < argc) {
            test_out = argv[++i];
        } else if (strcmp(argv[i], "--test-dump") == 0 && i + 1 < argc) {
            test_dump = argv[++i];
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            int s = atoi(argv[i+1]);
            if (s > 0 && s <= 10) scale = (unsigned)s;
        }
    }

    if (test_frames) {
        /* Детерминированный прогон: встроенный CHR (не data/chr.*), без SDL. */
        nes_init(NULL);
        return test_mode_run(test_frames, test_every, test_input, test_out, test_dump);
    }

    printf("Options: --region ntsc|pal, --apu-filters (90Hz+440Hz HPF, 14kHz LPF), --scale N (1-10)\n");

    const char *chr_paths[] = {
        "data/chr.bin",
        "data/chr.bmp",
        NULL
    };

    nes_init(chr_paths);

    if (sdl_init(scale) < 0) return 1;

    printf("Controls: A = X, B = Z, Select = Right Shift, Start = Enter, Arrows = Up/Down/Left/Right\n");


    sdl_run();
    sdl_cleanup();

    return 0;
}
