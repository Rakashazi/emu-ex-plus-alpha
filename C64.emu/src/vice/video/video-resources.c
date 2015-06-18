/*
 * video-resources.c - Resources for the video layer
 *
 * Written by
 *  John Selck <graham@cruise.de>
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

/* #define DEBUG_VIDEO */

#ifdef DEBUG_VIDEO
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "video-color.h"
#include "video.h"
#include "videoarch.h"
#include "viewport.h"
#include "util.h"

/*-----------------------------------------------------------------------*/
/* global resources.  */

#ifdef HAVE_HWSCALE
static int hwscale_possible;

static int set_hwscale_possible(int val, void *param)
{
    hwscale_possible = val ? 1 : 0;

    return 0;
}

static resource_int_t resources_hwscale_possible[] =
{
    { "HwScalePossible", 1, RES_EVENT_NO, NULL,
      &hwscale_possible, set_hwscale_possible, NULL },
    RESOURCE_INT_LIST_END
};
#endif

int video_resources_init(void)
{
#ifdef HAVE_HWSCALE
    if (machine_class != VICE_MACHINE_VSID) {
        if (resources_register_int(resources_hwscale_possible) < 0) {
            return -1;
        }
    }
#endif

    return video_arch_resources_init();
}

void video_resources_shutdown(void)
{
    video_arch_resources_shutdown();
}

/*-----------------------------------------------------------------------*/
/* Per chip resources.  */

struct video_resource_chip_mode_s {
    video_canvas_t *resource_chip;
    unsigned int device;
};
typedef struct video_resource_chip_mode_s video_resource_chip_mode_t;

static int set_double_size_enabled(int value, void *param)
{
    cap_render_t *cap_render;
    video_canvas_t *canvas = (video_canvas_t *)param;
    int old_doublesizex, old_doublesizey;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;
    int val = value ? 1 : 0;

    if (val) {
        cap_render = &video_chip_cap->double_mode;
    } else {
        cap_render = &video_chip_cap->single_mode;
    }

    canvas->videoconfig->rendermode = cap_render->rmode;

    old_doublesizex = canvas->videoconfig->doublesizex;
    old_doublesizey = canvas->videoconfig->doublesizey;

    if (cap_render->sizex > 1
        && (video_chip_cap->dsize_limit_width == 0
            || (canvas->draw_buffer->canvas_width
                <= video_chip_cap->dsize_limit_width))
        ) {
        canvas->videoconfig->doublesizex = (cap_render->sizex - 1);
    } else {
        canvas->videoconfig->doublesizex = 0;
    }

    if (cap_render->sizey > 1
        && (video_chip_cap->dsize_limit_height == 0
            || (canvas->draw_buffer->canvas_height
                <= video_chip_cap->dsize_limit_height))
        ) {
        canvas->videoconfig->doublesizey = (cap_render->sizey - 1);
    } else {
        canvas->videoconfig->doublesizey = 0;
    }


    DBG(("set_double_size_enabled sizex:%d sizey:%d doublesizex:%d doublesizey:%d rendermode:%d", cap_render->sizex, cap_render->sizey, canvas->videoconfig->doublesizex, canvas->videoconfig->doublesizey, canvas->videoconfig->rendermode));

    canvas->videoconfig->color_tables.updated = 0;
    if ((canvas->videoconfig->double_size_enabled != val
         || old_doublesizex != canvas->videoconfig->doublesizex
         || old_doublesizey != canvas->videoconfig->doublesizey)
        && canvas->initialized
        && canvas->viewport->update_canvas > 0) {
        video_viewport_resize(canvas, 1);
    }

    canvas->videoconfig->double_size_enabled = val;
    return 0;
}

static const char *vname_chip_size[] = { "DoubleSize", NULL };

static resource_int_t resources_chip_size[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_double_size_enabled, NULL },
    RESOURCE_INT_LIST_END
};

static int set_double_scan_enabled(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;

    canvas->videoconfig->doublescan = val ? 1 : 0;
    canvas->videoconfig->color_tables.updated = 0;

    if (canvas->initialized) {
        video_canvas_refresh_all(canvas);
    }
    return 0;
}

static const char *vname_chip_scan[] = { "DoubleScan", NULL };

static resource_int_t resources_chip_scan[] =
{
    { NULL, 1, RES_EVENT_NO, NULL,
      NULL, set_double_scan_enabled, NULL },
    RESOURCE_INT_LIST_END
};

static int set_hwscale_enabled(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;

#ifdef HAVE_HWSCALE
    if (val
        && !canvas->videoconfig->hwscale
        && !hwscale_possible)
#endif
    {
        log_message(LOG_DEFAULT, "HW scale not available, forcing to disabled");
        return 0;
    }

    canvas->videoconfig->hwscale = val ? 1 : 0;
    canvas->videoconfig->color_tables.updated = 0;

    if (canvas->initialized) {
        video_viewport_resize(canvas, 1);
    }
    return 0;
}

static const char *vname_chip_hwscale[] = { "HwScale", NULL };

static resource_int_t resources_chip_hwscale[] =
{
    { NULL, 1, RES_EVENT_NO, NULL, NULL, set_hwscale_enabled, NULL },
    RESOURCE_INT_LIST_END
};

static int set_chip_rendermode(int val, void *param)
{
    char *chip, *dsize;
    int old, err;
    video_canvas_t *canvas = (video_canvas_t *)param;

    switch (val) {
        case VIDEO_FILTER_NONE:
        case VIDEO_FILTER_CRT:
        case VIDEO_FILTER_SCALE2X:
            break;
        default:
            return -1;
    }

    old = canvas->videoconfig->filter;
    chip = canvas->videoconfig->chip_name;

    DBG(("set_chip_rendermode %s (canvas:%p) (%d->%d)", chip, canvas, old, val));

    dsize = util_concat(chip, "DoubleSize", NULL);

    canvas->videoconfig->filter = val;
    canvas->videoconfig->scale2x = 0; /* FIXME: remove this */
    canvas->videoconfig->color_tables.updated = 0;
    err = 0;
    switch (val) {
        case VIDEO_FILTER_NONE:
            break;
        case VIDEO_FILTER_CRT:
            break;
        case VIDEO_FILTER_SCALE2X:
            /* set double size */
            if (resources_set_int(dsize, 1) < 0) {
                err = 1;
            }
            canvas->videoconfig->scale2x = 1; /* FIXME: remove this */
            break;
    }

    if (err) {
        canvas->videoconfig->filter = old;
    }

    lib_free(dsize);

    if (canvas->initialized) {
        video_canvas_refresh_all(canvas);
    }
    return 0;
}

static const char *vname_chip_rendermode[] = { "Filter", NULL };

static resource_int_t resources_chip_rendermode[] =
{
    { NULL, VIDEO_FILTER_CRT, RES_EVENT_NO, NULL,
      NULL, set_chip_rendermode, NULL },
    RESOURCE_INT_LIST_END
};

static int set_fullscreen_enabled(int value, void *param)
{
    int val = value ? 1 : 0;
    int r = 0;
    video_canvas_t *canvas = (video_canvas_t *)param;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    canvas->videoconfig->fullscreen_enabled = val;

#ifndef USE_SDLUI
    if (canvas->initialized)
#endif
    {
        if (val) {
            r = (video_chip_cap->fullscreen.enable)(canvas, val);
            (void) (video_chip_cap->fullscreen.statusbar)
                (canvas, canvas->videoconfig->fullscreen_statusbar_enabled);
        } else {
            /* always show statusbar when coming back to window mode */
            (void) (video_chip_cap->fullscreen.statusbar)(canvas, 1);
            r = (video_chip_cap->fullscreen.enable)(canvas, val);
        }
    }
    return r;
}

static int set_fullscreen_statusbar(int value, void *param)
{
    int val = value ? 1 : 0;
    video_canvas_t *canvas = (video_canvas_t *)param;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    canvas->videoconfig->fullscreen_statusbar_enabled = val;

    return (video_chip_cap->fullscreen.statusbar)(canvas, val);
}

#if 0
/* FIXME: unused ?? */
static int set_fullscreen_double_size_enabled(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    canvas->videoconfig->fullscreen_double_size_enabled = val;

    return (video_chip_cap->fullscreen.double_size)(canvas, val);
}

static int set_fullscreen_double_scan_enabled(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    canvas->videoconfig->fullscreen_double_scan_enabled = val;

    return (video_chip_cap->fullscreen.double_scan)(canvas, val);
}
#endif

static int set_fullscreen_device(const char *val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    if (canvas->videoconfig->fullscreen_enabled) {
        log_message(LOG_DEFAULT,
                    "Fullscreen (%s) already active - disable first.",
                    canvas->videoconfig->fullscreen_device);
        return 0;
    }

    if (util_string_set(&canvas->videoconfig->fullscreen_device, val)) {
        return 0;
    }

    return (video_chip_cap->fullscreen.device)(canvas, val);
}

static const char *vname_chip_fullscreen[] = {
    "Fullscreen", "FullscreenStatusbar", "FullscreenDevice", NULL
};

static resource_string_t resources_chip_fullscreen_string[] =
{
    { NULL, NULL, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_device, NULL },
    RESOURCE_STRING_LIST_END
};

static resource_int_t resources_chip_fullscreen_int[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_enabled, NULL },
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_statusbar, NULL },
#if 0
    /* if 0'ed, because they don't seem to get initialized at all */
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_double_size_enabled, NULL },
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_double_scan_enabled, NULL },
#endif
    RESOURCE_INT_LIST_END
};

static int set_fullscreen_mode(int val, void *param)
{
    video_resource_chip_mode_t *video_resource_chip_mode = (video_resource_chip_mode_t *)param;
    video_canvas_t *canvas = video_resource_chip_mode->resource_chip;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    unsigned device = video_resource_chip_mode->device;


    canvas->videoconfig->fullscreen_mode[device] = val;

    return (video_chip_cap->fullscreen.mode[device])(canvas, val);
}

static const char *vname_chip_fullscreen_mode[] = { "FullscreenMode", NULL };

static resource_int_t resources_chip_fullscreen_mode[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_mode, NULL },
    RESOURCE_INT_LIST_END
};

static int set_ext_palette(int val, void *param)
{
    video_canvas_t *canvas;

    canvas = (video_canvas_t *)param;

    canvas->videoconfig->external_palette = (unsigned int)(val ? 1 : 0);
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_palette_file_name(const char *val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;

    util_string_set(&(canvas->videoconfig->external_palette_name), val);
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static const char *vname_chip_palette[] = { "PaletteFile", "ExternalPalette", NULL };

static resource_string_t resources_chip_palette_string[] =
{
    { NULL, NULL, RES_EVENT_NO, NULL,
      NULL, set_palette_file_name, NULL },
    RESOURCE_STRING_LIST_END
};

static resource_int_t resources_chip_palette_int[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_ext_palette, NULL },
    RESOURCE_INT_LIST_END
};

static int set_double_buffer_enabled(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;

    canvas->videoconfig->double_buffer = val ? 1 : 0;

    return 0;
}

static const char *vname_chip_double_buffer[] = { "DoubleBuffer", NULL };

static resource_int_t resources_chip_double_buffer[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_double_buffer_enabled, NULL },
    RESOURCE_INT_LIST_END
};

/*
      resources for the color/palette generator
*/

static int set_color_saturation(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.color_saturation = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_color_contrast(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.color_contrast = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_color_brightness(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.color_brightness = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_color_gamma(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 4000) {
        val = 4000;
    }
    canvas->videoconfig->video_resources.color_gamma = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_color_tint(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.color_tint = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static const char *vname_chip_colors[] = { "ColorSaturation", "ColorContrast", "ColorBrightness", "ColorGamma", "ColorTint", NULL };

static resource_int_t resources_chip_colors[] =
{
    { NULL, 1250, RES_EVENT_NO, NULL,
      NULL, set_color_saturation, NULL },
    { NULL, 1250, RES_EVENT_NO, NULL,
      NULL, set_color_contrast, NULL },
    { NULL, 1000, RES_EVENT_NO, NULL,
      NULL, set_color_brightness, NULL },
    { NULL, 2200, RES_EVENT_NO, NULL,
      NULL, set_color_gamma, NULL },
    { NULL, 1000, RES_EVENT_NO, NULL,
      NULL, set_color_tint, NULL },
    RESOURCE_INT_LIST_END
};

static int set_pal_scanlineshade(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 1000) {
        val = 1000;
    }
    canvas->videoconfig->video_resources.pal_scanlineshade = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_pal_oddlinesphase(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.pal_oddlines_phase = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_pal_oddlinesoffset(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 2000) {
        val = 2000;
    }
    canvas->videoconfig->video_resources.pal_oddlines_offset = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_pal_blur(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    if (val < 0) {
        val = 0;
    }
    if (val > 1000) {
        val = 1000;
    }
    canvas->videoconfig->video_resources.pal_blur = val;
    canvas->videoconfig->color_tables.updated = 0;
    return 0;
}

static int set_audioleak(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    canvas->videoconfig->video_resources.audioleak = val ? 1 : 0;
    return 0;
}

static const char *vname_chip_crtemu[] = { "PALScanLineShade", "PALBlur", "PALOddLinePhase", "PALOddLineOffset", "AudioLeak", NULL };

static resource_int_t resources_chip_crtemu[] =
{
    { NULL, 750, RES_EVENT_NO, NULL,
      NULL, set_pal_scanlineshade, NULL },
    { NULL, 500, RES_EVENT_NO, NULL,
      NULL, set_pal_blur, NULL },
    { NULL, 1125, RES_EVENT_NO, NULL,
      NULL, set_pal_oddlinesphase, NULL },
    { NULL, 875, RES_EVENT_NO, NULL,
      NULL, set_pal_oddlinesoffset, NULL },
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_audioleak, NULL },
    RESOURCE_INT_LIST_END
};

/*-----------------------------------------------------------------------*/
#define RES_CHIP_MODE_MAX (2*4) /* assume max 2 videochips, 4 fullscreen devices */
static video_resource_chip_mode_t *resource_chip_modes[RES_CHIP_MODE_MAX];
static int resource_chip_modes_num = 0;

static video_resource_chip_mode_t *get_resource_chip_mode(void)
{
    video_resource_chip_mode_t *p;
    p = lib_malloc(sizeof(video_resource_chip_mode_t));
    if (resource_chip_modes_num < RES_CHIP_MODE_MAX) {
        resource_chip_modes[resource_chip_modes_num] = p;
        resource_chip_modes_num += 1;
    } else {
        log_error(LOG_DEFAULT, "get_resource_chip_mode (increase RES_CHIP_MODE_MAX)");
    }
    return p;
}

static void shutdown_resource_chip_mode(void)
{
    int i;
    for (i = 0; i < resource_chip_modes_num; i++) {
        lib_free(resource_chip_modes[i]);
        resource_chip_modes[i] = NULL;
    }
    resource_chip_modes_num = 0;
}

int video_resources_chip_init(const char *chipname,
                              struct video_canvas_s **canvas,
                              video_chip_cap_t *video_chip_cap)
{
    unsigned int i;
    
    DBG(("video_resources_chip_init (%s) (canvas:%p) (cap:%p)", chipname, *canvas, video_chip_cap));

    video_render_initconfig((*canvas)->videoconfig);
    (*canvas)->videoconfig->cap = video_chip_cap;

    (*canvas)->videoconfig->chip_name = lib_stralloc(chipname);

    /* Set single size render as default.  */
    (*canvas)->videoconfig->rendermode = video_chip_cap->single_mode.rmode;
    (*canvas)->videoconfig->doublesizex
        = video_chip_cap->single_mode.sizex > 1 ? 1 : 0;
    (*canvas)->videoconfig->doublesizey
        = video_chip_cap->single_mode.sizey > 1 ? 1 : 0;

    /* CHIPDoubleScan */
    if (video_chip_cap->dscan_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_scan[0].name
                = util_concat(chipname, vname_chip_scan[0], NULL);
            resources_chip_scan[0].value_ptr
                = &((*canvas)->videoconfig->doublescan);
            resources_chip_scan[0].param = (void *)*canvas;
            if (resources_register_int(resources_chip_scan) < 0) {
                return -1;
            }

            lib_free((char *)(resources_chip_scan[0].name));
        } else {
            set_double_scan_enabled(0, (void *)*canvas);
        }
    }

    if (video_chip_cap->hwscale_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_hwscale[0].name
                = util_concat(chipname, vname_chip_hwscale[0], NULL);
            resources_chip_hwscale[0].value_ptr
                = &((*canvas)->videoconfig->hwscale);
            resources_chip_hwscale[0].param = (void *)*canvas;
            if (resources_register_int(resources_chip_hwscale) < 0) {
                return -1;
            }

            lib_free((char *)(resources_chip_hwscale[0].name));
        } else {
            set_hwscale_enabled(0, (void *)*canvas);
        }
    }

    /* CHIPDoubleSize */
    if (video_chip_cap->dsize_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_size[0].name
                = util_concat(chipname, vname_chip_size[0], NULL);
            resources_chip_size[0].factory_value
                = video_chip_cap->dsize_default;
            resources_chip_size[0].value_ptr
                = &((*canvas)->videoconfig->double_size_enabled);
            resources_chip_size[0].param = (void *)*canvas;
            if (resources_register_int(resources_chip_size) < 0) {
                return -1;
            }

            lib_free((char *)(resources_chip_size[0].name));
        } else {
            set_double_size_enabled(0, (void *)*canvas);
        }
    }

    if (video_chip_cap->fullscreen.device_num > 0) {
        video_resource_chip_mode_t *resource_chip_mode;

        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_fullscreen_int[0].name
                = util_concat(chipname, vname_chip_fullscreen[0], NULL);
            resources_chip_fullscreen_int[0].value_ptr
                = &((*canvas)->videoconfig->fullscreen_enabled);
            resources_chip_fullscreen_int[0].param = (void *)*canvas;

            resources_chip_fullscreen_int[1].name
                = util_concat(chipname, vname_chip_fullscreen[1], NULL);
            resources_chip_fullscreen_int[1].value_ptr
                = &((*canvas)->videoconfig->fullscreen_statusbar_enabled);
            resources_chip_fullscreen_int[1].param = (void *)*canvas;

            resources_chip_fullscreen_string[0].name
                = util_concat(chipname, vname_chip_fullscreen[2], NULL);
            resources_chip_fullscreen_string[0].factory_value
                = video_chip_cap->fullscreen.device_name[0];
            resources_chip_fullscreen_string[0].value_ptr
                = &((*canvas)->videoconfig->fullscreen_device);
            resources_chip_fullscreen_string[0].param = (void *)*canvas;

            if (resources_register_string(resources_chip_fullscreen_string) < 0) {
                return -1;
            }

            if (resources_register_int(resources_chip_fullscreen_int) < 0) {
                return -1;
            }

            lib_free((char *)(resources_chip_fullscreen_int[0].name));
            lib_free((char *)(resources_chip_fullscreen_int[1].name));
            lib_free((char *)(resources_chip_fullscreen_string[0].name));
        } else {
            set_fullscreen_enabled(0, (void *)*canvas);
            set_fullscreen_statusbar(0, (void *)*canvas);
            set_fullscreen_device(video_chip_cap->fullscreen.device_name[0], (void *)*canvas);
        }

        for (i = 0; i < video_chip_cap->fullscreen.device_num; i++) {
            resource_chip_mode = get_resource_chip_mode();
            resource_chip_mode->resource_chip = *canvas;
            resource_chip_mode->device = i;

            if (machine_class != VICE_MACHINE_VSID) {
                resources_chip_fullscreen_mode[0].name
                    = util_concat(chipname,
                                  video_chip_cap->fullscreen.device_name[i],
                                  vname_chip_fullscreen_mode[0], NULL);
                resources_chip_fullscreen_mode[0].value_ptr
                    = &((*canvas)->videoconfig->fullscreen_mode[i]);
                resources_chip_fullscreen_mode[0].param
                    = (void *)resource_chip_mode;

                if (resources_register_int(resources_chip_fullscreen_mode) < 0) {
                    return -1;
                }

                lib_free((char *)(resources_chip_fullscreen_mode[0].name));
            } else {
                set_fullscreen_mode(0, (void *)resource_chip_mode);
            }
        }
    }

    /* Palette related */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_chip_palette_string[0].name
            = util_concat(chipname, vname_chip_palette[0], NULL);
        resources_chip_palette_string[0].factory_value
            = video_chip_cap->external_palette_name;
        resources_chip_palette_string[0].value_ptr
            = &((*canvas)->videoconfig->external_palette_name);
        resources_chip_palette_string[0].param = (void *)*canvas;

        resources_chip_palette_int[0].name
            = util_concat(chipname, vname_chip_palette[1], NULL);
        resources_chip_palette_int[0].value_ptr
            = &((*canvas)->videoconfig->external_palette);
        resources_chip_palette_int[0].param = (void *)*canvas;

        if (resources_register_string(resources_chip_palette_string) < 0) {
            return -1;
        }

        if (resources_register_int(resources_chip_palette_int) < 0) {
            return -1;
        }

        lib_free((char *)(resources_chip_palette_string[0].name));
        lib_free((char *)(resources_chip_palette_int[0].name));
    } else {
        set_palette_file_name(video_chip_cap->external_palette_name, (void *)*canvas);
        set_ext_palette(0, (void *)*canvas);
    }

    /* double buffering */
    if (video_chip_cap->double_buffering_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_double_buffer[0].name
                = util_concat(chipname, vname_chip_double_buffer[0], NULL);
            resources_chip_double_buffer[0].value_ptr
                = &((*canvas)->videoconfig->double_buffer);
            resources_chip_double_buffer[0].param = (void *)*canvas;
            if (resources_register_int(resources_chip_double_buffer) < 0) {
                return -1;
            }

            lib_free((char *)(resources_chip_double_buffer[0].name));
        } else {
            set_double_buffer_enabled(0, (void *)*canvas);
        }
    }

    /* palette generator */
    if (machine_class != VICE_MACHINE_VSID) {
        i = 0;
        while (vname_chip_colors[i]) {
            resources_chip_colors[i].name = util_concat(chipname, vname_chip_colors[i], NULL);
            resources_chip_colors[i].param = (void *)*canvas;
            ++i;
        }
        resources_chip_colors[0].value_ptr = &((*canvas)->videoconfig->video_resources.color_saturation);
        resources_chip_colors[1].value_ptr = &((*canvas)->videoconfig->video_resources.color_contrast);
        resources_chip_colors[2].value_ptr = &((*canvas)->videoconfig->video_resources.color_brightness);
        resources_chip_colors[3].value_ptr = &((*canvas)->videoconfig->video_resources.color_gamma);
        resources_chip_colors[4].value_ptr = &((*canvas)->videoconfig->video_resources.color_tint);

        if (resources_register_int(resources_chip_colors) < 0) {
            return -1;
        }

        i = 0;
        while (vname_chip_colors[i]) {
            lib_free((char *)(resources_chip_colors[i].name));
            ++i;
        }
    } else {
        set_color_saturation(1250, (void *)*canvas);
        set_color_contrast(1250, (void *)*canvas);
        set_color_brightness(1000, (void *)*canvas);
        set_color_gamma(2200, (void *)*canvas);
        set_color_tint(1000, (void *)*canvas);
    }

    /* crt emulation */
    if (machine_class != VICE_MACHINE_VSID) {
        i = 0;
        while (vname_chip_crtemu[i]) {
            resources_chip_crtemu[i].name = util_concat(chipname, vname_chip_crtemu[i], NULL);
            resources_chip_crtemu[i].param = (void *)*canvas;
            ++i;
        }
        resources_chip_crtemu[0].value_ptr = &((*canvas)->videoconfig->video_resources.pal_scanlineshade);
        resources_chip_crtemu[1].value_ptr = &((*canvas)->videoconfig->video_resources.pal_blur);
        resources_chip_crtemu[2].value_ptr = &((*canvas)->videoconfig->video_resources.pal_oddlines_phase);
        resources_chip_crtemu[3].value_ptr = &((*canvas)->videoconfig->video_resources.pal_oddlines_offset);
        resources_chip_crtemu[4].value_ptr = &((*canvas)->videoconfig->video_resources.audioleak);

        if (resources_register_int(resources_chip_crtemu) < 0) {
            return -1;
        }

        i = 0;
        while (vname_chip_crtemu[i]) {
            lib_free((char *)(resources_chip_crtemu[i].name));
            ++i;
        }
    } else {
        set_pal_scanlineshade(750, (void *)*canvas);
        set_pal_blur(500, (void *)*canvas);
        set_pal_oddlinesphase(1125, (void *)*canvas);
        set_pal_oddlinesoffset(875, (void *)*canvas);
        set_audioleak(0, (void *)*canvas);
    }

    /* CHIPFilter */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_chip_rendermode[0].name
            = util_concat(chipname, vname_chip_rendermode[0], NULL);
        resources_chip_rendermode[0].value_ptr
            = &((*canvas)->videoconfig->filter);
        resources_chip_rendermode[0].param = (void *)*canvas;
        if (resources_register_int(resources_chip_rendermode) < 0) {
            return -1;
        }

        lib_free((char *)(resources_chip_rendermode[0].name));
    } else {
        set_chip_rendermode(VIDEO_FILTER_NONE, (void *)*canvas);
    }

    return 0;
}

void video_resources_chip_shutdown(struct video_canvas_s *canvas)
{
    lib_free(canvas->videoconfig->external_palette_name);
    lib_free(canvas->videoconfig->chip_name);

    if (canvas->videoconfig->cap->fullscreen.device_num > 0) {
        lib_free(canvas->videoconfig->fullscreen_device);
    }
    /* NOTE: in x128 this actually shuts down the respective resources of both
     *       videochips at once. this is not exactly clean, but shouldnt matter
     *       in practise either.
     */
    shutdown_resource_chip_mode();
}
