#include "sdl_init.h"
#include "nes/config.h"
#include "nes/apu_sim.h"
#include "game/nmi.h"
#include "version.h"
#include <stdio.h>





static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static uint32_t pixels[NES_SCREEN_TOTAL];

#ifdef USE_SDL3
#undef SDL_sem
#define SDL_sem SDL_Semaphore
#undef SDL_SemWait
#define SDL_SemWait SDL_WaitSemaphore
#undef SDL_SemPost
#define SDL_SemPost SDL_SignalSemaphore
#endif

#ifdef USE_SDL3
#define SDL_INIT_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO)
#else
#define SDL_INIT_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)
#endif

#ifdef USE_SDL3
typedef bool key_state_t;
#else
typedef Uint8 key_state_t;
#endif

static SDL_sem *wake_sem = NULL;
static SDL_sem *vblank_sem = NULL;

volatile int plat_running = 1;

plat_sem_t plat_get_wake_sem(void) { return (plat_sem_t)wake_sem; }
plat_sem_t plat_get_vblank_sem(void) { return (plat_sem_t)vblank_sem; }

void plat_sem_wait(plat_sem_t sem) { SDL_SemWait((SDL_sem*)sem); }
void plat_sem_post(plat_sem_t sem) { SDL_SemPost((SDL_sem*)sem); }

uint32_t* sdl_get_pixels(void) { return pixels; }
SDL_Renderer* sdl_get_renderer(void) { return renderer; }
SDL_Texture* sdl_get_texture(void) { return texture; }

#ifdef USE_SDL3
static void sdl3_audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    (void)userdata; (void)total_amount;
    if (additional_amount <= 0) return;
    uint8_t *buf = SDL_malloc((size_t)additional_amount);
    if (buf) {
        audio_callback(NULL, buf, additional_amount);
        SDL_PutAudioStreamData(stream, buf, additional_amount);
        SDL_free(buf);
    }
}
#endif

int sdl_init(unsigned scale) {
    if (scale == 0) scale = 1;

#ifdef USE_SDL3
    int v = SDL_GetVersion();
    printf("SDL3 version: %d.%d.%d\n", v / 1000000, (v / 1000) % 1000, v % 1000);
#else
    SDL_version v;
    SDL_GetVersion(&v);
    printf("SDL2 version: %d.%d.%d\n", v.major, v.minor, v.patch);
#endif

#ifdef USE_SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
#else     
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {   
#endif        
        const char *err = SDL_GetError();
        fprintf(stderr, "SDL_Init failed: %s\n", err && err[0] ? err : "unknown error");
        return -1;
    }

    int w = NES_SCREEN_W * scale;
    int h = NES_SCREEN_H * scale;

#ifdef USE_SDL3
    window = SDL_CreateWindow("Battle City C " BATTLE_CITY_VERSION, w, h, 0);
#else
    window = SDL_CreateWindow("Battle City C " BATTLE_CITY_VERSION,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w, h,
                              SDL_WINDOW_SHOWN);
#endif

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

#ifdef USE_SDL3
    renderer = SDL_CreateRenderer(window, NULL);
#else
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
#endif

    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return -1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, NES_SCREEN_W, NES_SCREEN_H);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return -1;
    }

#ifdef USE_SDL3
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
#else
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
#endif

    apu_init();

#ifdef USE_SDL3
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (dev == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
    } else {
        SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, &spec);
        if (stream) {
            SDL_SetAudioStreamGetCallback(stream, sdl3_audio_callback, NULL);
            SDL_BindAudioStream(dev, stream);
            SDL_ResumeAudioDevice(dev);
        }
    }
#else
    SDL_AudioSpec desired;
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 1024;
    desired.callback = audio_callback;
    desired.userdata = NULL;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (dev == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(dev, 0);
    }
#endif

    wake_sem = SDL_CreateSemaphore(0);
    if (!wake_sem) {
        fprintf(stderr, "SDL_CreateSemaphore failed: %s\n", SDL_GetError());
        return -1;
    }

    vblank_sem = SDL_CreateSemaphore(0);
    if (!vblank_sem) {
        fprintf(stderr, "SDL_CreateSemaphore failed: %s\n", SDL_GetError());
        SDL_DestroySemaphore(wake_sem);
        return -1;
    }

    return 0;
}

void sdl_cleanup(void) {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (vblank_sem) SDL_DestroySemaphore(vblank_sem);
    if (wake_sem) SDL_DestroySemaphore(wake_sem);
    SDL_Quit();
}

uint8_t plat_poll_buttons_p1(void) {
    const key_state_t *keys = SDL_GetKeyboardState(NULL);
    uint8_t btns = 0;
    if (keys[SDL_SCANCODE_X]) btns |= 0x01;
    if (keys[SDL_SCANCODE_Z]) btns |= 0x02;
    if (keys[SDL_SCANCODE_RSHIFT]) btns |= 0x04;
    if (keys[SDL_SCANCODE_RETURN]) btns |= 0x08;
    if (keys[SDL_SCANCODE_UP]) btns |= 0x10;
    if (keys[SDL_SCANCODE_DOWN]) btns |= 0x20;
    if (keys[SDL_SCANCODE_LEFT]) btns |= 0x40;
    if (keys[SDL_SCANCODE_RIGHT]) btns |= 0x80;
    return btns;
}

uint8_t plat_poll_buttons_p2(void) {
    return 0;
}
