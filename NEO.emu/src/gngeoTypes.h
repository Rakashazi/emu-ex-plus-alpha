#pragma once
#include <imagine/util/ansiTypes.h>
#include <imagine/util/builtins.h>
#include <imagine/logger/logger.h>

// SDL wrapper, renamed with GN prefix to avoid conflicts on WebOS, which includes some parts of SDL

#define Uint32 uint32
#define Uint16 uint16
#define Uint8 uint8
#define Sint32 sint32
#define Sint16 sint16
#define Sint8 sint8

typedef struct
{
	Sint16 x, y;
	Uint16 w, h;
} GN_Rect;

typedef struct GN_Surface {
	uint pitch;
	int w;
	void *pixels;
} GN_Surface;

static Uint16 SDL_Swap16(Uint16 x)
{
	return ((x << 8) | (x >> 8));
}

static Uint32 SDL_Swap32(Uint32 x)
{
	return ((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) |
	(x >> 24));
}

#define SDL_TRUE 1

static int GN_FillRect(GN_Surface *dst, GN_Rect *dstrect, Uint32 color) { return 0; }
static int GN_LockSurface(GN_Surface *surface) { return 0; }
static void GN_UnlockSurface(GN_Surface *surface) { }
static void GN_SetClipRect(GN_Surface *surface, GN_Rect *rect) { }

// unused Gngeo functions

//static void gn_reset_pbar(void) { }
static void gn_terminate_pbar(void) { }
static int init_sdl_audio(void) { return 1; }
static void close_sdl_audio(void) { }
static void pause_audio(int on) { }
static void reset_frame_skip(void) { }
static int handle_event(void) { return 0; }
static Uint32 run_menu(void) { return 0; }
static void reset_event(void) { }
static int frame_skip(int init) { return 0; }
static void trans_pack_free(void) { }
static void trans_pack_open(char *filename) { }
static void sdl_set_title(char *name) { }
static void cf_reset_to_default(void) { }
static int cf_open_file(char *filename) { return 0; }
static int screen_reinit(void) { return 1; }
