
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SDL.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "screen.h"
#include "emu.h"
#include "video.h"
#include "conf.h"

#include "blitter.h"
#include "effect.h"

SDL_bool effect_none_init(void);

SDL_bool effect_smooth_init(void);

blitter_func blitter[] = {
	{"soft", "Software blitter", blitter_soft_init, NULL, blitter_soft_update, blitter_soft_fullscreen,
		blitter_soft_close},
#ifndef GP2X
#ifndef WII
#ifdef HAVE_GL_GL_H
	{"opengl", "Opengl blitter", blitter_opengl_init, blitter_opengl_resize, blitter_opengl_update,
		blitter_opengl_fullscreen, blitter_opengl_close},
#endif
	{"yuv", "YUV blitter (YV12)", blitter_overlay_init, blitter_overlay_resize, blitter_overlay_update,
		blitter_overlay_fullscreen, blitter_overlay_close},
#endif
#endif
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

effect_func effect[] = {

	{"none", "No effect", 1, 1, effect_none_init, NULL},
#ifdef PANDORA
	// Fake effect to setup video filter
		{"smooth", "Default Pandora Slightly blurred filter", 1, 1, effect_smooth_init, NULL},
#endif
#ifndef GP2X
#ifndef WII
	{"scanline", "Scanline effect", 2, 2, effect_scanline_init, effect_scanline_update}, // 1
	{"scanline50", "Scanline 50% effect", 2, 2, effect_scanline_init, effect_scanline50_update}, // 2
	{"scale2x", "Scale2x effect", 2, 2, effect_scale2x_init, effect_scale2x_update}, // 3
#ifndef PANDORA
	{"scale3x", "Scale3x effect", 3, 3, effect_scale3x_init, effect_scale3x_update}, // 3
	{"scale4x", "Scale4x effect", 4, 4, effect_scale4x_init, effect_scale4x_update}, // 3
#endif
	{"scale2x50", "Scale2x effect with 50% scanline", 2, 2, effect_scale2x_init, effect_scale2x50_update}, // 4
	{"scale2x75", "Scale2x effect with 75% scanline", 2, 2, effect_scale2x_init, effect_scale2x75_update}, // 5
	{"hq2x", "HQ2X effect. High quality", 2, 2, effect_hq2x_init, effect_hq2x_update},
	{"lq2x", "LQ2X effect. Low quality", 2, 2, effect_lq2x_init, effect_lq2x_update},
#ifndef PANDORA
	{"hq3x", "HQ3X effect. High quality", 3, 3, effect_hq3x_init, effect_hq3x_update},
	//    {"hq4x","HQ4X effect. High quality",4,4,effect_hq4x_init, effect_hq4x_update},
	{"lq3x", "LQ3X effect. Low quality", 3, 3, effect_lq3x_init, effect_lq3x_update},
	{"doublex", "Double the x resolution (soft blitter only)", 2, 1, effect_scanline_init, effect_doublex_update}, //6
#endif
#ifdef I386_ASM
	{"sai", "SAI effect", 2, 2, effect_sai_init, effect_sai_update}, //7
	{"supersai", "SuperSAI effect", 2, 2, effect_sai_init, effect_supersai_update}, //8
	{"eagle", "Eagle effect", 2, 2, effect_sai_init, effect_eagle_update}, //9
#endif
#endif
#endif
	{NULL, NULL, 0, 0, NULL, NULL}
};

/* Interpolation */
static SDL_Surface *tmp, *blend;

RGB2YUV rgb2yuv[65536];

void
init_rgb2yuv_table(void) {
	static char init = 0;
	Uint32 i;
	Uint8 y, u, v, r, g, b;
	if (init == 0) {
		init = 1;
		for (i = 0; i <= 65535; i++) {
			r = ((i & 0xF800) >> 11) << 3;
			g = ((i & 0x7E0) >> 5) << 2;
			b = (i & 0x1F) << 3;

			y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
			u = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
			v = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;

			rgb2yuv[i].y = (y << 8) | y;
			rgb2yuv[i].u = u;
			rgb2yuv[i].v = v;
			rgb2yuv[i].yuy2 = (y << 24) | (v << 16) | (y << 8) | u;
		}
	}
}

#ifdef I386_ASM
Uint32 scan_mask = 0xf7def7de;
void do_inner_interpolation_i386(Uint16 * dst, Uint16 * src);
#endif

void print_blitter_list(void) {
	int i = 0;
	while (blitter[i].name != NULL) {
		printf("%-12s : %s\n", blitter[i].name, blitter[i].desc);
		i++;
	}
}

void print_effect_list(void) {
	int i = 0;
	while (effect[i].name != NULL) {
		printf("%-12s : %s\n", effect[i].name, effect[i].desc);
		i++;
	}
}

LIST* create_effect_list(void) {
	LIST *el = NULL;
	int i = 0;
	while (effect[i].name != NULL) {
		el = list_append(el, &effect[i]);
		i++;
	}
	return el;
}

LIST* create_blitter_list(void) {
	LIST *bl = NULL;
	int i = 0;
	while (blitter[i].name != NULL) {
		bl = list_append(bl, &blitter[i]);
		i++;
	}
	return bl;
}

Uint8 get_effect_by_name(char *name) {
	int i = 0;

	while (effect[i].name != NULL) {
		if (!strcmp(effect[i].name, name)) {
			return i;
		}
		i++;
	}
	/* invalid effect */
	printf("Invalid effect.\n");
	return 0;
}

Uint8 get_blitter_by_name(char *name) {
	int i = 0;

	while (blitter[i].name != NULL) {
		if (!strcmp(blitter[i].name, name)) {
			return i;
		}
		i++;
	}
	/* invalid blitter */
	printf("Invalid blitter, using soft blitter.\n");
	return 0;
}

SDL_bool screen_init() {
	CONF_ITEM *cf_blitter, *cf_effect, *cf_interpol, *cf_scale, *cf_fs;

	/* screen configuration init */
	cf_blitter = cf_get_item_by_name("blitter");
	cf_effect = cf_get_item_by_name("effect");
	cf_interpol = cf_get_item_by_name("interpolation");
	cf_scale = cf_get_item_by_name("scale");
	cf_fs = cf_get_item_by_name("fullscreen");

/*
	if (CF_BOOL(cf_get_item_by_name("screen320"))) {
		visible_area.x = 16;
		visible_area.y = 16;
		visible_area.w = 320;
		visible_area.h = 224;
	} else {
		visible_area.x = 24;
		visible_area.y = 16;
		visible_area.w = 304;
		visible_area.h = 224;
	}
*/
		visible_area.x = 16;
		visible_area.y = 16;
		visible_area.w = 320;
		visible_area.h = 224;


	/* Initialization of some variables */
	/*
		interpolation = conf.interpolation;
		nblitter = conf.nblitter;
		neffect = conf.neffect;
	 */
	interpolation = CF_BOOL(cf_interpol);
	nblitter = get_blitter_by_name(CF_STR(cf_blitter));
	neffect = get_effect_by_name(CF_STR(cf_effect));
	fullscreen = CF_BOOL(cf_fs);
	conf.res_x = 304;
	conf.res_y = 224;

	if (CF_VAL(cf_scale) == 0)
		scale = 1;
	else
		scale = CF_VAL(cf_scale);

	/* Init of video blitter */
	if ((*blitter[nblitter].init) () == SDL_FALSE)
		return SDL_FALSE;

	/* Init of effect */
	//if (neffect > 0)
	if ((*effect[neffect].init) () == SDL_FALSE)
		return SDL_FALSE;

	/* Interpolation surface */
	blend = SDL_CreateRGBSurface(SDL_SWSURFACE/*(conf.hw_surface ? SDL_HWSURFACE : SDL_SWSURFACE)*/,
			352, 256, 16, 0xF800, 0x7E0, 0x1F, 0);
	printf("CURSOR=%d\n", SDL_ShowCursor(SDL_QUERY));
	if (SDL_ShowCursor(SDL_QUERY) == 1)
		SDL_ShowCursor(SDL_DISABLE);
	printf("CURSOR=%d\n", SDL_ShowCursor(SDL_QUERY));
	return SDL_TRUE;
}

SDL_bool effect_none_init(void) {
#ifdef PANDORA
	system("sudo /usr/pandora/scripts/op_videofir.sh none");
#endif
	return SDL_TRUE;
}
#ifdef PANDORA
SDL_bool effect_smooth_init(void) {
	system("sudo /usr/pandora/scripts/op_videofir.sh default");
	return SDL_TRUE;
}
#endif
void screen_change_blitter_and_effect(void) {
	CONF_ITEM *cf_blitter, *cf_effect;
/*
			if (bname == NULL) bname = CF_STR(cf_get_item_by_name("blitter"));
			if (ename == NULL) ename = CF_STR(cf_get_item_by_name("effect"));
	 */

	cf_blitter = cf_get_item_by_name("blitter");
	cf_effect = cf_get_item_by_name("effect");

	(*blitter[nblitter].close) ();

	nblitter = get_blitter_by_name(CF_STR(cf_blitter));
	neffect = get_effect_by_name(CF_STR(cf_effect));
//	printf("set %s %s \n", bname, ename);

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	if ((*blitter[nblitter].init) () == SDL_FALSE) {
		nblitter = 0;
		sprintf(CF_STR(cf_get_item_by_name("blitter")), "soft");
		printf("revert to soft\n");
		if ((*blitter[nblitter].init) () == SDL_FALSE)
			exit(-1);
	} /*else
		snprintf(CF_STR(cf_get_item_by_name("blitter")), 255, "%s", bname);
*/
	if ((*effect[neffect].init) () == SDL_FALSE) {
		printf("revert to none\n");
		neffect = 0;
		sprintf(CF_STR(cf_get_item_by_name("effect")), "none");
	} /*else
		snprintf(CF_STR(cf_get_item_by_name("effect")), 255, "%s", ename);
*/
	SDL_InitSubSystem(SDL_INIT_VIDEO);


	printf("CURSOR=%d\n", SDL_ShowCursor(SDL_QUERY));
	if (SDL_ShowCursor(SDL_QUERY) == 1)
		SDL_ShowCursor(SDL_DISABLE);
	printf("CURSOR=%d\n", SDL_ShowCursor(SDL_QUERY));
}

SDL_bool screen_reinit(void) {



/*
	if (CF_BOOL(cf_get_item_by_name("screen320"))) {
		visible_area.x = 16;
		visible_area.y = 16;
		visible_area.w = 320;
		visible_area.h = 224;
	} else {
		visible_area.x = 24;
		visible_area.y = 16;
		visible_area.w = 304;
		visible_area.h = 224;
	}
*/

		visible_area.x = 16;
		visible_area.y = 16;
		visible_area.w = 320;
		visible_area.h = 224;

	/* Initialization of some variables */
	/*
		interpolation = conf.interpolation;
		nblitter = conf.nblitter;
		neffect = conf.neffect;
	 */
	interpolation = CF_BOOL(cf_get_item_by_name("interpolation"));
	fullscreen = CF_BOOL(cf_get_item_by_name("fullscreen"));
	conf.res_x = 304;
	conf.res_y = 224;

	if (CF_VAL(cf_get_item_by_name("scale")) == 0)
		scale = 1;
	else
		scale = CF_VAL(cf_get_item_by_name("scale"));
printf("AA Blitter %s effect %s\n",CF_STR(cf_get_item_by_name("blitter")),CF_STR(cf_get_item_by_name("effect")));
	screen_change_blitter_and_effect();

	return SDL_TRUE;
}

SDL_bool screen_resize(int w, int h) {
	//nblitter = conf.nblitter;
	if ((*blitter[nblitter].resize) (w, h) == SDL_FALSE)
		return SDL_FALSE;
	return SDL_TRUE;
}

static inline void do_interpolation() {
	Uint16 *dst = (Uint16 *) blend->pixels + 16 + (352 << 4);
	Uint16 *src = (Uint16 *) buffer->pixels + 16 + (352 << 4);
	Uint32 s, d;
	Uint8 w, h;
#ifdef I386_ASM
	do_inner_interpolation_i386(dst, src);
#else
	/* we copy pixels from buffer surface to blend surface */
	for (w = 224; w > 0; w--) {
		for (h = 160; h > 0; h--) {
			s = *(Uint32 *) src;
			d = *(Uint32 *) dst;

			*(Uint32 *) dst =
					((d & 0xf7def7de) >> 1) + ((s & 0xf7def7de) >> 1) +
					(s & d & 0x08210821);

			dst += 2;
			src += 2;
		}
		src += 32; //(visible_area.x<<1);
		dst += 32; //(visible_area.x<<1);
	}
#endif
	/* Swap Buffers */
	tmp = blend;
	blend = buffer;
	buffer = tmp;
}

static SDL_Rect left_border={16,16,8,224};
static SDL_Rect right_border={16+312,16,8,224};


void screen_update() {
	if (interpolation == 1)
		do_interpolation();
	if (!conf.screen320) {
		SDL_FillRect(buffer, &left_border, 0);
		SDL_FillRect(buffer, &right_border, 0);
	}



	if (effect[neffect].update != NULL)
		(*effect[neffect].update) ();

	(*blitter[nblitter].update) ();
}

void screen_close() {
	SDL_FreeSurface(blend);
}

void screen_fullscreen() {
	fullscreen ^= 1;
	blitter[nblitter].fullscreen();
}
