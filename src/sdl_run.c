#include "sdl_run.h"
#include "sdl_init.h"
#ifdef DEBUG_SCREENS
#include "debug.h"
#endif
#include "nes/config.h"
#include "nes/ppu_sim.h"
#include "game/nmi.h"
#include "game/draw.h"
#include "game/sound_engine.h"
#include <SDL.h>
#include <stdatomic.h>
#include <stdio.h>


#ifdef USE_SDL3
#undef SDL_RenderCopy
#define SDL_RenderCopy SDL_RenderTexture
#undef SDL_QUIT
#define SDL_QUIT SDL_EVENT_QUIT
#endif

static void update_game(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            plat_running = 0;
            plat_sem_post(plat_get_wake_sem());
            plat_sem_post(plat_get_vblank_sem());
            return;
        }
    }

    ppu_set_vblank_flag();
    plat_sem_post(plat_get_vblank_sem());

    if (atomic_load_explicit(&game_in_nmi_wait, memory_order_acquire)) {
        ppu_render(sdl_get_pixels());
        SDL_UpdateTexture(sdl_get_texture(), NULL, sdl_get_pixels(),
                          NES_SCREEN_W * sizeof(uint32_t));
        SDL_RenderClear(sdl_get_renderer());
        SDL_RenderCopy(sdl_get_renderer(), sdl_get_texture(), NULL, NULL);
        SDL_RenderPresent(sdl_get_renderer());

        nmi();
#ifdef DEBUG_SCREENS
        debug();
#endif
        plat_sem_post(plat_get_wake_sem());
    } else {
        SDL_RenderClear(sdl_get_renderer());
        SDL_RenderCopy(sdl_get_renderer(), sdl_get_texture(), NULL, NULL);
        SDL_RenderPresent(sdl_get_renderer());
        play_sound();
    }
}

void sdl_run(void) {
    SDL_Thread *game_thread = SDL_CreateThread(nes_thread, "game", NULL);
    if (!game_thread) {
        fprintf(stderr, "SDL_CreateThread failed: %s\n", SDL_GetError());
        return;
    }

    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t next_tick = SDL_GetPerformanceCounter();

    while (plat_running) {
        uint64_t now = SDL_GetPerformanceCounter();
        uint64_t period_ticks = (uint64_t)(((double)freq * frame_period_ns()) / 1.0e9);

        if (next_tick > now) {
            uint64_t wait_ticks = next_tick - now;
            uint32_t wait_ms = (uint32_t)((wait_ticks * 1000) / freq);
            if (wait_ms > 1) SDL_Delay(wait_ms - 1);
            while (SDL_GetPerformanceCounter() < next_tick);
        } else if (now - next_tick > 5 * period_ticks) {
            next_tick = now;
        }
        next_tick += period_ticks;

        update_game();
    }

    SDL_WaitThread(game_thread, NULL);
}
