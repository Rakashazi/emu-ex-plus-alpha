#pragma once
#include <stdint.h>

#define Uint32 uint32_t
#define Uint16 uint16_t
#define Uint8 uint8_t
#define Sint32 int32_t
#define Sint16 int16_t
#define Sint8 int8_t

typedef struct
{
	Sint16 x, y;
	Uint16 w, h;
} GN_Rect;

typedef struct GN_Surface {
	uint32_t pitch;
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

static int GN_FillRect(GN_Surface *dst, GN_Rect *dstrect, Uint32 color) { return 0; }
static int GN_LockSurface(GN_Surface *surface) { return 0; }
static void GN_UnlockSurface(GN_Surface *surface) { }
static void GN_SetClipRect(GN_Surface *surface, GN_Rect *rect) { }

// unused Gngeo functions

static void gn_terminate_pbar(void) { }
static void trans_pack_free(void) { }
static void trans_pack_open(char *filename) { }
static void cf_reset_to_default(void) { }
static int cf_open_file(char *filename) { return 0; }
static int screen_reinit(void) { return 1; }
