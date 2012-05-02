
#ifndef _BLITTER_H_
#define _BLITTER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "SDL.h"

typedef struct {
    const char *name;
    const char *desc;
    SDL_bool(*init) ();
    SDL_bool(*resize) (int w, int h);
    void (*update) ();
    void (*fullscreen) ();
    void (*close) ();
} blitter_func;

/* Software blitter */
SDL_bool blitter_soft_init();
void blitter_soft_update();
void blitter_soft_fullscreen();
void blitter_soft_close();

/* OpenGL blitter */
SDL_bool blitter_opengl_init();
SDL_bool blitter_opengl_resize(int w, int h);
void blitter_opengl_update();
void blitter_opengl_fullscreen();
void blitter_opengl_close();

/* Overlay blitter */
SDL_bool blitter_overlay_init();
SDL_bool blitter_overlay_resize();
void blitter_overlay_update();
void blitter_overlay_fullscreen();
void blitter_overlay_close();

extern blitter_func blitter[];


#endif
