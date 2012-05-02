
#ifndef _EFFECT_H_
#define _EFFECT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "SDL.h"

typedef struct {
    const char *name;
    const char *desc;
    Uint8 x_ratio,y_ratio;
    SDL_bool(*init) ();
    void (*update) ();
} effect_func;

/* Scanline effect */
SDL_bool effect_scanline_init();
void effect_scanline_update();
void effect_scanline50_update();

void effect_doublex_update();

/* Scale2x effect */
SDL_bool effect_scale2x_init();
SDL_bool effect_scale3x_init();
SDL_bool effect_scale4x_init();
void effect_scale2x_update();
void effect_scale3x_update();
void effect_scale4x_update();
void effect_scale2x50_update();
void effect_scale2x75_update();

/* hqx effect */
SDL_bool effect_hq2x_init();
void effect_hq2x_update();
SDL_bool effect_hq3x_init();
void effect_hq3x_update();
SDL_bool effect_hq4x_init();
void effect_hq4x_update();

SDL_bool effect_lq2x_init();
void effect_lq2x_update();
SDL_bool effect_lq3x_init();
void effect_lq3x_update();

/* Sai effect */
SDL_bool effect_sai_init();
void effect_sai_update();
void effect_eagle_update();
void effect_supersai_update();


extern effect_func effect[];


#endif
