/*
 * video.h - Common video API.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_VIDEO_H
#define VICE_VIDEO_H

#include "types.h"

/* video chip type */
#define VIDEO_CHIP_VICII 0
#define VIDEO_CHIP_VDC   1

/* video filter type, resource "CHIPFilter" */
#define VIDEO_FILTER_NONE         0
#define VIDEO_FILTER_CRT          1
#define VIDEO_FILTER_SCALE2X      2

/* video filter type, resource "CHIPGLFilter" */
#define VIDEO_GLFILTER_NEAREST      0
#define VIDEO_GLFILTER_BILINEAR     1
#define VIDEO_GLFILTER_BICUBIC      2

/* These constants are used to configure the video output.  */

/* no video output (dummy) */
#define VIDEO_RENDER_NULL             0
/* PAL or NTSC TV/Monitor (like VIC, VIC-II or TED) */
#define VIDEO_RENDER_PAL_NTSC_1X1     1
#define VIDEO_RENDER_PAL_NTSC_2X2     2
/* monochrome Monitor (CRTC) */
#define VIDEO_RENDER_CRT_MONO_1X1     3
#define VIDEO_RENDER_CRT_MONO_1X2     4 /* needed for y-stretch */
#define VIDEO_RENDER_CRT_MONO_2X2     5
#define VIDEO_RENDER_CRT_MONO_2X4     6 /* needed for y-stretch */
/* RGB(I) Monitor (VDC) */
#define VIDEO_RENDER_RGBI_1X1         7
#define VIDEO_RENDER_RGBI_1X2         8 /* needed for y-stretch */
#define VIDEO_RENDER_RGBI_2X2         9
#define VIDEO_RENDER_RGBI_2X4        10 /* needed for y-stretch */

/* type of monitor/display that is connected */
#define VIDEO_CRT_TYPE_NTSC           0
#define VIDEO_CRT_TYPE_PAL            1
#define VIDEO_CRT_TYPE_RGB            2
#define VIDEO_CRT_TYPE_MONO           3

/* aspect ration mode (<CHIP>AspectMode) */
#define VIDEO_ASPECT_MODE_NONE          0
#define VIDEO_ASPECT_MODE_CUSTOM        1
#define VIDEO_ASPECT_MODE_TRUE          2

struct video_canvas_s;
struct video_cbm_palette_s;
struct viewport_s;
struct geometry_s;
struct palette_s;

struct canvas_refresh_s {
    uint8_t *draw_buffer;
    int draw_buffer_line_size;
    int x;
    int y;
};
typedef struct canvas_refresh_s canvas_refresh_t;

struct draw_buffer_s {
    /* The real drawing buffers, with padding bytes on either side to workaround CRT and Scale2x bugs */
    uint8_t *draw_buffer_padded_allocations[2];
    /* Pointers into the non padded buffers within the padded buffers */
    uint8_t *draw_buffer_non_padded[2];
    /* The memory buffer where the screen of the emulated machine is drawn. Palettized, 1 byte per pixel */
    uint8_t *draw_buffer;
    /* Width of draw_buffer in pixels */
    unsigned int draw_buffer_width;
    /* Height of draw_buffer in pixels. Typically same as geometry->screen_size.height */
    unsigned int draw_buffer_height;
    unsigned int draw_buffer_pitch;
    /* Width of emulator screen (physical screen on the machine where the emulator runs) in pixels */
    unsigned int canvas_physical_width;
    /* Height of emulator screen (physical screen on the machine where the emulator runs) in pixels */
    unsigned int canvas_physical_height;
    /* Maximum theoretical width of draw_buffer that would fit in the emulator screen.
    Typically, it is the same as canvas_physical_width if no horizontal stretch is used (videoconfig->scalex == 1) and smaller if it is used.
    TODO do we really need it? */
    unsigned int canvas_width;
    /* Maximum theoretical height of draw_buffer that would fit in the emulator screen.
    Typically, it is the same as canvas_physical_width if no vertical stretch is used (videoconfig->scaley == 1) and smaller if it is used.
    TODO do we really need it? */
    unsigned int canvas_height;
    /* Width of the visible subset of draw_buffer, in pixels. Typically same as geometry->screen_size.width */
    unsigned int visible_width;
    /* Height of the visible subset of draw_buffer, in pixels */
    unsigned int visible_height;
};
typedef struct draw_buffer_s draw_buffer_t;

struct cap_render_s {
    unsigned int sizex;
    unsigned int sizey;
    unsigned int rmode;
};
typedef struct cap_render_s cap_render_t;

/* FIXME: get rid of this */
#define FULLSCREEN_MAXDEV 4

struct cap_fullscreen_s {
    /* FIXME: get rid of as much as possible of this. */
    int (*enable)(struct video_canvas_s *canvas, int enable);
    int (*mode[FULLSCREEN_MAXDEV])(struct video_canvas_s *canvas, int mode);
};
typedef struct cap_fullscreen_s cap_fullscreen_t;

struct video_chip_cap_s {
    unsigned int dsize_allowed;
    unsigned int dsize_default;
    unsigned int dsize_limit_width;
    unsigned int dsize_limit_height;
    unsigned int dscan_allowed;
    unsigned int interlace_allowed;
    const char *external_palette_name;
    cap_render_t single_mode;
    cap_render_t double_mode;
    cap_fullscreen_t fullscreen;
    unsigned int video_has_palntsc; /* 1 if options for pal/ntsc/secam/whatever colour system are needed */
};
typedef struct video_chip_cap_s video_chip_cap_t;

#define VIDEO_MAX_OUTPUT_WIDTH  2048

struct video_render_color_tables_s {
    int updated;                /* tables here are up to date */
    uint32_t physical_colors[256];
    int32_t ytableh[256];        /* y for current pixel */
    int32_t ytablel[256];        /* y for neighbouring pixels */
    int32_t cbtable[256];        /* b component */
    int32_t cbtable_odd[256];    /* b component + phase shift */
    int32_t crtable[256];        /* r component */
    int32_t crtable_odd[256];    /* r component + phase shift */
    int32_t cutable[256];        /* u component */
    int32_t cutable_odd[256];    /* u component + phase shift */
    int32_t cvtable[256];        /* v component */
    int32_t cvtable_odd[256];    /* v component + phase shift */

    /* YUV table for hardware rendering: (Y << 16) | (U << 8) | V */
    int yuv_updated;            /* yuv table updated for packed mode */
    uint32_t yuv_table[512];
    int32_t line_yuv_0[VIDEO_MAX_OUTPUT_WIDTH * 3];
    int16_t prevrgbline[VIDEO_MAX_OUTPUT_WIDTH * 3];
    uint8_t rgbscratchbuffer[VIDEO_MAX_OUTPUT_WIDTH * 4];

    /*
     * All values below here formerly were globals in video-color.h.
     * This resulted in palette leaks between multiple rendering windows (VDC / VICII)
     */
    uint32_t gamma_red[256 * 3];
    uint32_t gamma_grn[256 * 3];
    uint32_t gamma_blu[256 * 3];

    uint32_t gamma_red_fac[256 * 3 * 2];
    uint32_t gamma_grn_fac[256 * 3 * 2];
    uint32_t gamma_blu_fac[256 * 3 * 2];

    /* optional alpha value for 32bit rendering */
    uint32_t alpha;

    uint32_t color_red[256];
    uint32_t color_grn[256];
    uint32_t color_blu[256];
};
typedef struct video_render_color_tables_s video_render_color_tables_t;

/* options for the color generator and crt emulation */
typedef struct video_resources_s {
    /* parameters for color generation */
    int color_saturation;
    int color_contrast;
    int color_brightness;
    int color_gamma;
    int color_tint;
    /* additional parameters for CRT emulation */
    int pal_scanlineshade;      /* amount of scanline shade */
    int pal_blur;               /* luma blur */
    int pal_oddlines_phase;     /* oddlines UV phase offset */
    int pal_oddlines_offset;    /* oddlines UV multiplier */
    int delaylinetype;          /* type of delayline, UV or just U (1084 style) */

    int audioleak;              /* flag: enable video->audio leak emulation */
} video_resources_t;

/* render config for a specific canvas and video chip */
struct video_render_config_s {
    char *chip_name;               /* chip name prefix, (use to build resource names) */
    video_resources_t video_resources; /* options for the color generator and crt emulation */
    video_chip_cap_t *cap;         /* Which renderers are allowed?  */
    int rendermode;                /* What renderer is active?  */
    int double_size_enabled;       /* Double size enabled?  */
    int scalex;                    /* Horizontal scaling */
    int scaley;                    /* Vertical scaling */
    int doublescan;                /* Doublescan enabled?  */
    int filter;                    /* VIDEO_FILTER_NONE, VIDEO_FILTER_CRT, VIDEO_FILTER_SCALE2X */
    int glfilter;                  /* <CHIP>GLFilter */
    int vsync;                     /* <CHIP>VSync */
    int external_palette;          /* Use an external palette?  */
    char *external_palette_name;   /* Name of the external palette.  */
    int readable;                  /* reading of frame buffer is safe and fast */
    int interlaced;                /* Is the output currently interlaced? */
    int interlace_field;           /* Which of the two interlaced frames is current? */
    struct video_cbm_palette_s *cbm_palette; /* Internal palette.  */
    struct video_render_color_tables_s color_tables;
    int show_statusbar;            /**< Show statusbar in the UI (boolean) */
    int aspect_mode;
    double aspect_ratio;
    char *aspect_ratio_s;               /* custom aspect ratio <CHIP>AspectRatio */
    char *aspect_ratio_factory_value_s; /* custom aspect ratio <CHIP>AspectRatio */
    int flipx, flipy, rotate;           /* mirroring; <CHIP>FlipX, <CHIP>FlipY, <CHIP>Rotate */
    /* FIXME: we can probably get rid of this: */
    int double_buffer;             /* Double buffering enabled? */
    /* FIXME: get rid of as much as possible of the following: */
    int fullscreen_enabled;
    int fullscreen_mode[FULLSCREEN_MAXDEV];
    int fullscreen_custom_width; /* currently used only in the SDL port */
    int fullscreen_custom_height; /* currently used only in the SDL port */
};
typedef struct video_render_config_s video_render_config_t;

void video_render_initconfig(video_render_config_t *config);
void video_render_setphysicalcolor(video_render_config_t *config, int index, uint32_t color, int depth);
void video_render_setrawrgb(video_render_color_tables_t *color_tab, unsigned int index, uint32_t r, uint32_t g, uint32_t b);
void video_render_setrawalpha(video_render_color_tables_t *color_tab, uint32_t a);
void video_render_initraw(struct video_render_config_s *videoconfig);

/**************************************************************/

int video_arch_cmdline_options_init(void);
int video_cmdline_options_init(void);
int video_init(void);
void video_shutdown(void);

struct video_canvas_s *video_canvas_create(struct video_canvas_s *canvas, unsigned int *width, unsigned int *height, int mapped);
void video_arch_canvas_init(struct video_canvas_s *canvas);
int video_arch_get_active_chip(void);
void video_canvas_shutdown(struct video_canvas_s *canvas);
struct video_canvas_s *video_canvas_init(void);
void video_canvas_refresh_all_tracked(void);
void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi,
                          unsigned int w, unsigned int h);
int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette);

/* This will go away.  */
int video_canvas_palette_set(struct video_canvas_s *canvas, struct palette_s *palette);
void video_canvas_create_set(struct video_canvas_s *canvas);
void video_canvas_destroy(struct video_canvas_s *canvas);
void video_canvas_map(struct video_canvas_s *canvas);
void video_canvas_unmap(struct video_canvas_s *canvas);
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas);
void video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg, int width, int height, int xs, int ys, int xt, int yt, int pitcht);
void video_canvas_refresh_all(struct video_canvas_s *canvas);
char video_canvas_can_resize(struct video_canvas_s *canvas);
void video_viewport_get(struct video_canvas_s *canvas, struct viewport_s **viewport, struct geometry_s **geometry);
void video_viewport_resize(struct video_canvas_s *canvas, char resize_canvas);

struct raster_s;

int video_resources_init(void);
void video_resources_shutdown(void);
int video_resources_chip_init(const char *chipname, struct video_canvas_s **canvas, video_chip_cap_t *video_chip_cap);
void video_resources_chip_shutdown(struct video_canvas_s *canvas);
int video_cmdline_options_chip_init(const char *chipname, video_chip_cap_t *video_chip_cap);
int video_arch_resources_init(void);
void video_arch_resources_shutdown(void);

/* Video render interface */

/* Videochip related color/palette types */

#define CBM_PALETTE_YUV  0
#define CBM_PALETTE_RGB  1

typedef struct video_cbm_color_s {
    float luminance;        /* (R) luminance                      */
    float angle;            /* (G) angle on color wheel           */
    float saturation;       /* (B) */
    int direction;          /* +1 (pos), -1 (neg) or 0 (grey) */
    char *name;             /* name of this color             */
} video_cbm_color_t;
/* note: to handle chips that output RGB (such as the VDC), the above structure
         is currently abused for RGB colors also. */

typedef struct video_cbm_palette_s {
    unsigned int num_entries;           /* number of colors in palette  */
    video_cbm_color_t *entries;         /* array of colors              */
    video_cbm_color_t *entries_odd;     /* array of colors (odd lines)  */
    video_cbm_color_t *entries_even;    /* array of colors (even lines) */
    float phase;      /* color phase (will be added to all color angles) */
    int type;
} video_cbm_palette_t;

void video_color_palette_internal(struct video_canvas_s *canvas, struct video_cbm_palette_s *cbm_palette);
int video_color_update_palette(struct video_canvas_s *canvas);
void video_color_palette_free(struct palette_s *palette);

#include <viceVideoAPI.h>

#endif
