#include <stdio.h>
#include <string.h>
#include "nes/config.h"
#include "sdl_init.h"
#include "sdl_run.h"

int main(int argc, char **argv) {
    current_region = REGION_NTSC;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--region") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "pal") == 0)
                current_region = REGION_PAL;
            else if (strcmp(argv[i+1], "ntsc") == 0)
                current_region = REGION_NTSC;
        } else if (strcmp(argv[i], "--apu-filters") == 0) {
            apu_filters_enabled = 1;
        }
    }

    printf("Options: --region ntsc|pal, --apu-filters (90Hz+440Hz HPF, 14kHz LPF)\n");    

    const char *chr_paths[] = {
        "data/chr.bin",
        "data/chr.bmp",
        NULL
    };

    nes_init(chr_paths);

    if (sdl_init() < 0) return 1;

    printf("Controls: A = X, B = Z, Select = Right Shift, Start = Enter, Arrows = Up/Down/Left/Right\n");


    sdl_run();
    sdl_cleanup();

    return 0;
}
