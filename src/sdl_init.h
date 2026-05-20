#ifndef SDL_INIT_H
#define SDL_INIT_H

#include <SDL.h>
#include <stdint.h>

int sdl_init(void);
void sdl_cleanup(void);

uint32_t* sdl_get_pixels(void);
SDL_Renderer* sdl_get_renderer(void);
SDL_Texture* sdl_get_texture(void);

#endif
