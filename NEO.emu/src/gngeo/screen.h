
#ifndef _SCREEN_H_
#define _SCREEN_H_

//#include "SDL.h"
#include <gngeoTypes.h>
#include "list.h"

typedef struct RGB2YUV
{
  Uint16 y;
  Uint8  u;
  Uint8  v;
  Uint32 yuy2;
}RGB2YUV;

extern RGB2YUV rgb2yuv[65536];

void init_rgb2yuv_table(void);

//SDL_Surface *screen;
extern GN_Surface *buffer;//, *sprbuf, *fps_buf, *scan, *fontbuf;
//SDL_Surface *triplebuf[2];

extern GN_Rect visible_area;
extern int interpolation;

//int yscreenpadding;

Uint8 get_effect_by_name(char *name);
Uint8 get_blitter_by_name(char *name);
void print_blitter_list(void);
void print_effect_list(void);
//void screen_change_blitter_and_effect(char *bname,char *ename);
LIST* create_effect_list(void);
LIST* create_blitter_list(void);

int screen_init();
int screen_reinit(void);
int screen_resize(int w, int h);
void screen_update();
void screen_close();

void screen_fullscreen();

#endif
