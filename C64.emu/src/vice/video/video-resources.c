/** \file   video-resource.s
 * \brief   Resources for the video layer
 *
 * \author  John Selck <graham@cruise.de>
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

#include "videoarch.h"

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "video-color.h"
#include "video.h"
#include "viewport.h"
#include "util.h"

/*-----------------------------------------------------------------------*/
/* global resources.  */

int video_resources_init(void)
{
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


/** \brief  Setter for boolean resource "${CHIP}DoubleSize"
 *
 * \param[in]   double_size enable double size rendering
 * \param[in]   canvas      video canvas
 *
 * \return  0 on success, -1 on failure
 */
static int set_double_size_enabled(int double_size, void *canvas)
{
    cap_render_t *cap_render;
    video_canvas_t *cv = canvas;
    int old_scalex;
    int old_scaley;
    video_chip_cap_t *video_chip_cap = cv->videoconfig->cap;
    int val = double_size ? 1 : 0;

    if (val) {
        cap_render = &video_chip_cap->double_mode;
    } else {
        cap_render = &video_chip_cap->single_mode;
    }

    cv->videoconfig->rendermode = cap_render->rmode;

    old_scalex = cv->videoconfig->scalex;
    old_scaley = cv->videoconfig->scaley;

    if (cap_render->sizex > 1
        && (video_chip_cap->dsize_limit_width == 0
            || (cv->draw_buffer->canvas_width
                <= video_chip_cap->dsize_limit_width))
        ) {
        cv->videoconfig->scalex = cap_render->sizex;
    } else {
        cv->videoconfig->scalex = 1;
    }

    if (cap_render->sizey > 1
        && (video_chip_cap->dsize_limit_height == 0
            || (cv->draw_buffer->canvas_height
                <= video_chip_cap->dsize_limit_height))
        ) {
        cv->videoconfig->scaley = cap_render->sizey;
    } else {
        cv->videoconfig->scaley = 1;
    }


    DBG(("set_double_size_enabled sizex:%d sizey:%d scalex:%d scaley:%d rendermode:%d",
                cap_render->sizex, cap_render->sizey, canvas->videoconfig->scalex,
                canvas->videoconfig->scaley, canvas->videoconfig->rendermode));

    cv->videoconfig->color_tables.updated = 0;
    if ((cv->videoconfig->double_size_enabled != val
         || old_scalex != cv->videoconfig->scalex
         || old_scaley != cv->videoconfig->scaley)
        && cv->viewport->update_canvas > 0) {
        video_viewport_resize(cv, 1);
    }

    cv->videoconfig->double_size_enabled = val;
    return 0;
}

/** \brief  Resource init template for "${CHIP}DoubleSize"
 */
static resource_int_t resources_chip_double_size[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_double_size_enabled, NULL },
    RESOURCE_INT_LIST_END
};


/** \brief  Setter for the boolean resource "${CHIP}DoubleScan"
 *
 * \param[in]   double_scan enable double scan
 * \param[in]   canvas      video canvas
 *
 * \return  0
 */
static int set_double_scan_enabled(int double_scan, void *canvas)
{
    video_canvas_t *cv = canvas;

    cv->videoconfig->doublescan = double_scan ? 1 : 0;
    cv->videoconfig->color_tables.updated = 0;

    video_canvas_refresh_all(cv);

    return 0;
}

/** \brief  Resource registration template for "${CHIP}DoubleScan"
 */
static resource_int_t resources_chip_scan[] =
{
    { NULL, 1, RES_EVENT_NO, NULL,
      NULL, set_double_scan_enabled, NULL },
    RESOURCE_INT_LIST_END
};


/** \brief  Setter for integer resource "${CHIP}Filter"
 *
 * Sets the "render mode" to `VIDEO_FILTER_NONE`, `VIDEO_FILTER_CRT` or
 * `VIDEO_FILTER_SCALE2X`.
 *
 * \param[in]   filter  filter to use
 * \param[in]   canvas  video canvas
 *
 * \return  0 on success, -1 on failure
 *
 * FIXME:   Function name does not match the resource name at all!
 */
static int set_chip_rendermode(int filter, void *canvas)
{
    char *chip;
    char *dsize;
    int old;
    int err;
    video_canvas_t *cv = canvas;

    switch (filter) {
        case VIDEO_FILTER_NONE:
        case VIDEO_FILTER_CRT:
        case VIDEO_FILTER_SCALE2X:
            break;
        default:
            return -1;
    }

    old = cv->videoconfig->filter;
    chip = cv->videoconfig->chip_name;

    DBG(("set_chip_rendermode %s (canvas:%p) (%d->%d)", chip, cv, old, filter));

    dsize = util_concat(chip, "DoubleSize", NULL);

    cv->videoconfig->filter = filter;
    cv->videoconfig->color_tables.updated = 0;
    err = 0;
    switch (filter) {
        case VIDEO_FILTER_NONE:
            break;
        case VIDEO_FILTER_CRT:
            break;
        case VIDEO_FILTER_SCALE2X:
            /* set double size */
            if (resources_set_int(dsize, 1) < 0) {
                err = 1;
            }
            break;
    }

    if (err) {
        cv->videoconfig->filter = old;
    }

    lib_free(dsize);
    video_canvas_refresh_all(cv);
    return 0;
}

/** \brief  Resource registration template for "${CHIP}Filter"
 */
static resource_int_t resources_chip_rendermode[] =
{
    { NULL, VIDEO_FILTER_NONE, RES_EVENT_NO, NULL,
      NULL, set_chip_rendermode, NULL },
    RESOURCE_INT_LIST_END
};


#if defined(USE_SDLUI) || defined(USE_SDL2UI) || defined(USE_GTK3UI)
/** \brief  Setter for the boolean resource "CHIPFullscreen"
 *
 * \param[in]   enabled full screen enabled (bool)
 * \param[in]   canvas  video canvas reference
 *
 * \return  0 on success, -1 on failure
 */
static int set_fullscreen_enabled(int enabled, void *canvas)
{
    video_canvas_t *cv = (video_canvas_t *)canvas;
    video_chip_cap_t *video_chip_cap = cv->videoconfig->cap;
    int (*enable_cb)(video_canvas_t *, int);

    cv->videoconfig->fullscreen_enabled = enabled ? 1 : 0;
    enable_cb = video_chip_cap->fullscreen.enable;
    /* The enable() callback isn't set in the Gt3k UI.
     * If we decide to use it, it must be called in a thread-safe manner! */
    if (enable_cb != NULL) {
        return enable_cb(cv, enabled ? 1 : 0);
    }
    return 0;
}

/** \brief  Resource registration template for "${CHIP}Fullscreen"
 */
static resource_int_t resources_chip_fullscreen_int[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_enabled, NULL },
    RESOURCE_INT_LIST_END
};
#endif

#if defined(USE_SDLUI) || defined(USE_SDL2UI)
/* TODO:    Remove `device` from the API: turn fullscreen_mode[] into an integer etc.
 */
static int set_fullscreen_mode(int val, void *param)
{
    video_resource_chip_mode_t *video_resource_chip_mode = (video_resource_chip_mode_t *)param;
    video_canvas_t *canvas = video_resource_chip_mode->resource_chip;
    video_chip_cap_t *video_chip_cap = canvas->videoconfig->cap;

    unsigned device = video_resource_chip_mode->device;

    canvas->videoconfig->fullscreen_mode[device] = val;

    return (video_chip_cap->fullscreen.mode[device])(canvas, val);
}

/* <CHIP>FullscreenMode (SDL only) */
static const char * const vname_chip_fullscreen_mode[] = { "FullscreenMode", NULL };

static resource_int_t resources_chip_fullscreen_mode[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_fullscreen_mode, NULL },
    RESOURCE_INT_LIST_END
};
#endif

/** \brief  Setter for the boolean resource "${CHIP}ExternalPalette"
 *
 * \param[in]   external    use external palette
 * \param[in]   canvas      video canvas
 *
 * \return  0
 */
static int set_palette_is_external(int external, void *canvas)
{
    video_canvas_t *cv = canvas;

    cv->videoconfig->external_palette = external ? 1 : 0;
    cv->videoconfig->color_tables.updated = 0;
    return 0;
}

/** \brief  Setter for the resource "${CHIP}PaletteFile"
 *
 * \param[in]   filename    palette filename
 * \param[in]   canvas      video canvas
 *
 * \return  0
 */
static int set_palette_file_name(const char *filename, void *canvas)
{
    video_canvas_t *cv = canvas;

    util_string_set(&(cv->videoconfig->external_palette_name), filename);
    cv->videoconfig->color_tables.updated = 0;
    return 0;
}

/** \brief  Resource registration template for "${CHIP}PaletteFile"
 */
static resource_string_t resources_chip_palette_string[] =
{
    { NULL, NULL, RES_EVENT_NO, NULL,
      NULL, set_palette_file_name, NULL },
    RESOURCE_STRING_LIST_END
};

/** \brief  Resource registration template for "${CHIP}ExternalPalette"
 */
static resource_int_t resources_chip_palette_int[] =
{
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_palette_is_external, NULL },
    RESOURCE_INT_LIST_END
};

/** \brief  Setter for the boolean resource "${CHIP}DoubleBuffer"
 *
 * \param[in]   double_buffer   enable double buffering
 * \param[in]   canvas          video canvas
 *
 * \return  0
 */
static int set_double_buffer_enabled(int double_buffer, void *canvas)
{
    video_canvas_t *cv = canvas;

    cv->videoconfig->double_buffer = double_buffer ? 1 : 0;
    return 0;
}

/** \brief  Resource registration template for "${CHIP}DoubleBuffer"
 */
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

static const char * const vname_chip_colors[] = {
    "ColorSaturation", "ColorContrast", "ColorBrightness", "ColorGamma", "ColorTint", NULL };

static resource_int_t resources_chip_colors[] =
{
    { NULL, 1000, RES_EVENT_NO, NULL,
      NULL, set_color_saturation, NULL },
    { NULL, 1000, RES_EVENT_NO, NULL,
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

static int set_delaylinetype(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    canvas->videoconfig->video_resources.delaylinetype = val ? 1 : 0;
    return 0;
}

static int set_audioleak(int val, void *param)
{
    video_canvas_t *canvas = (video_canvas_t *)param;
    canvas->videoconfig->video_resources.audioleak = val ? 1 : 0;
    return 0;
}

static const char * const vname_chip_crtemu[] = {
    "PALScanLineShade", "PALBlur", "PALOddLinePhase", "PALOddLineOffset", "PALDelaylineType", "AudioLeak", NULL };

static resource_int_t resources_chip_crtemu[] =
{
    { NULL, 750, RES_EVENT_NO, NULL,
      NULL, set_pal_scanlineshade, NULL },
    { NULL, 500, RES_EVENT_NO, NULL,
      NULL, set_pal_blur, NULL },
    { NULL, 1500, RES_EVENT_NO, NULL,
      NULL, set_pal_oddlinesphase, NULL },
    { NULL, 500, RES_EVENT_NO, NULL,
      NULL, set_pal_oddlinesoffset, NULL },
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_delaylinetype, NULL },
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_audioleak, NULL },
    RESOURCE_INT_LIST_END
};


/** \brief  Setter for the boolean resource "${CHIP}ShowStatusbar"
 *
 * \param[in]   show_statusbar  show status bar in the UI
 * \param[in]   canvas          video canvas reference
 *
 * \return  0   (success)
 */
static int set_show_statusbar(int show_statusbar, void *canvas)
{
    video_canvas_t *cv = canvas;
    cv->videoconfig->show_statusbar = show_statusbar ? 1 : 0;
    return 0;
}

/** \brief  Resource registration template for CHIPShowStatusbar
 *
 * Values are filled in during resource registration, dependent on CHIP and UI.
 */
static resource_int_t resources_chip_show_statusbar[] =
{
    { NULL,                             /* resource name: filled in */
      ARCHDEP_SHOW_STATUSBAR_FACTORY,   /* factory default */
      RES_EVENT_NO, NULL,               /* event stuff */
      NULL,                             /* resource value pointer: filled in */
      set_show_statusbar,               /* setter function */
      NULL                              /* video_canvas reference for the setter:
                                           filled in */
    },
    RESOURCE_INT_LIST_END
};


/*-----------------------------------------------------------------------*/
#define RES_CHIP_MODE_MAX (2*4) /* assume max 2 videochips, 4 fullscreen devices */
static video_resource_chip_mode_t *resource_chip_modes[RES_CHIP_MODE_MAX];
static int resource_chip_modes_num = 0;

#if defined(USE_SDLUI) || defined(USE_SDL2UI)
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
#endif

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
#if defined (USE_SDLUI) || defined(USE_SDL2UI)
    video_resource_chip_mode_t *resource_chip_mode;
#endif
    unsigned int i;
    int result;

    DBG(("video_resources_chip_init (%s) (canvas:%p) (cap:%p)", chipname, *canvas, video_chip_cap));

    video_render_initconfig((*canvas)->videoconfig);
    (*canvas)->videoconfig->cap = video_chip_cap;

    (*canvas)->videoconfig->chip_name = lib_strdup(chipname);

    /* Set single size render as default.  */
    (*canvas)->videoconfig->rendermode = video_chip_cap->single_mode.rmode;
    (*canvas)->videoconfig->scalex
        = video_chip_cap->single_mode.sizex > 1 ? 2 : 1;
    (*canvas)->videoconfig->scaley
        = video_chip_cap->single_mode.sizey > 1 ? 2 : 1;

    /* ${CHIP}DoubleScan (bool) */
    if (video_chip_cap->dscan_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_scan[0].name
                = util_concat(chipname, "DoubleScan", NULL);
            resources_chip_scan[0].value_ptr
                = &((*canvas)->videoconfig->doublescan);
            resources_chip_scan[0].param = *canvas;

            result = resources_register_int(resources_chip_scan);
            lib_free(resources_chip_scan[0].name);
            if (result < 0) {
                return -1;
            }

        } else {
            set_double_scan_enabled(0, *canvas);
        }
    }

    /* ${CHIP}DoubleSize (bool) */
    if (video_chip_cap->dsize_allowed) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_double_size[0].name
                = util_concat(chipname, "DoubleSize", NULL);
            resources_chip_double_size[0].factory_value
                = video_chip_cap->dsize_default;
            resources_chip_double_size[0].value_ptr
                = &((*canvas)->videoconfig->double_size_enabled);
            resources_chip_double_size[0].param = *canvas;

            result = resources_register_int(resources_chip_double_size);
            lib_free(resources_chip_double_size[0].name);
            if (result < 0) {
                return -1;
            }
        } else {
            set_double_size_enabled(0, *canvas);
        }
    }

    /* fullscreen options */
#if defined(USE_SDLUI) || defined(USE_SDL2UI) || defined(USE_GTK3UI)

    if (machine_class != VICE_MACHINE_VSID) {
        /* ${CHIP}Fullscreen */
        resources_chip_fullscreen_int[0].name
            = util_concat(chipname, "Fullscreen", NULL);
        resources_chip_fullscreen_int[0].value_ptr
            = &((*canvas)->videoconfig->fullscreen_enabled);
        resources_chip_fullscreen_int[0].param = (void *)*canvas;

        result = resources_register_int(resources_chip_fullscreen_int);
        lib_free(resources_chip_fullscreen_int[0].name);
        if (result < 0) {
            return -1;
        }
    } else {
        set_fullscreen_enabled(0, (void *)*canvas);
    }
#endif

#if defined(USE_SDLUI) || defined(USE_SDL2UI)

    resource_chip_mode = get_resource_chip_mode();
    resource_chip_mode->resource_chip = *canvas;
    resource_chip_mode->device = 0;

    if (machine_class != VICE_MACHINE_VSID) {
        /* <CHIP>FullscreenMode */
        resources_chip_fullscreen_mode[0].name
            = util_concat(chipname,
                          vname_chip_fullscreen_mode[0], NULL);
        resources_chip_fullscreen_mode[0].value_ptr
            = &((*canvas)->videoconfig->fullscreen_mode[0]);
        resources_chip_fullscreen_mode[0].param
            = (void *)resource_chip_mode;

        result = resources_register_int(resources_chip_fullscreen_mode);
        lib_free(resources_chip_fullscreen_mode[0].name);
        if (result < 0) {
            return -1;
        }
    } else {
        set_fullscreen_mode(0, (void *)resource_chip_mode);
    }
#endif

    /* Palette related */
    if (machine_class != VICE_MACHINE_VSID) {
        /* ${CHIP}PaletteFile: palette filename */
        resources_chip_palette_string[0].name
            = util_concat(chipname, "PaletteFile", NULL);
        resources_chip_palette_string[0].factory_value
            = video_chip_cap->external_palette_name;
        resources_chip_palette_string[0].value_ptr
            = &((*canvas)->videoconfig->external_palette_name);
        resources_chip_palette_string[0].param = *canvas;

        result = resources_register_string(resources_chip_palette_string);
        lib_free(resources_chip_palette_string[0].name);
        if (result < 0) {
            return -1;
        }

        /* ${CHIP}ExternalPalette: boolean specifying whether the palette is
         * a VICE-provided one or one provided by the user */
        resources_chip_palette_int[0].name
            = util_concat(chipname, "ExternalPalette", NULL);
        resources_chip_palette_int[0].value_ptr
            = &((*canvas)->videoconfig->external_palette);
        resources_chip_palette_int[0].param = *canvas;

        result = resources_register_int(resources_chip_palette_int);
        lib_free(resources_chip_palette_int[0].name);
        if (result < 0) {
            return -1;
        }
    } else {
        set_palette_file_name(video_chip_cap->external_palette_name, *canvas);
        set_palette_is_external(0, *canvas);
    }

    /* ${CHIP}DoubleBuffer: double buffering  */
    if (video_chip_cap->double_buffering_allowed != 0) {
        if (machine_class != VICE_MACHINE_VSID) {
            resources_chip_double_buffer[0].name
                = util_concat(chipname, "DoubleBuffer", NULL);
            resources_chip_double_buffer[0].value_ptr
                = &((*canvas)->videoconfig->double_buffer);
            resources_chip_double_buffer[0].param = *canvas;

            result = resources_register_int(resources_chip_double_buffer);
            lib_free(resources_chip_double_buffer[0].name);
            if (result < 0) {
                return -1;
            }
        } else {
            set_double_buffer_enabled(0, *canvas);
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

        resources_chip_colors[0].factory_value = 1000; /* saturation */
        resources_chip_colors[1].factory_value = 1000; /* contrast */
        if (!strcmp(chipname, "VIC")) {
            resources_chip_colors[0].factory_value = 1500; /* saturation */
            resources_chip_colors[1].factory_value = 1250; /* contrast */
        } else if (!strcmp(chipname, "VICII")) {
            resources_chip_colors[0].factory_value = 1250; /* saturation */
            resources_chip_colors[1].factory_value = 1250; /* contrast */
        } else if (!strcmp(chipname, "TED")) {
            resources_chip_colors[0].factory_value = 1250; /* saturation */
        } else if (!strcmp(chipname, "Crtc")) {
            resources_chip_colors[0].factory_value = 1250; /* saturation */
            resources_chip_colors[1].factory_value = 1250; /* contrast */
        }

        if (resources_register_int(resources_chip_colors) < 0) {
            return -1;
        }

        i = 0;
        while (vname_chip_colors[i]) {
            lib_free(resources_chip_colors[i].name);
            ++i;
        }
    } else {
        set_color_saturation(1000, (void *)*canvas);
        set_color_contrast(1000, (void *)*canvas);
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
        resources_chip_crtemu[4].value_ptr = &((*canvas)->videoconfig->video_resources.delaylinetype);
        resources_chip_crtemu[5].value_ptr = &((*canvas)->videoconfig->video_resources.audioleak);

        resources_chip_crtemu[2].factory_value = 1000; /* oddlines phase */
        resources_chip_crtemu[3].factory_value = 1000; /* oddlines offset */
        if (!strcmp(chipname, "VIC")) {
            resources_chip_crtemu[2].factory_value = 1125; /* oddlines phase */
            resources_chip_crtemu[3].factory_value = 1125; /* oddlines offset */
        } else if (!strcmp(chipname, "VICII")) {
            resources_chip_crtemu[2].factory_value = 1250; /* oddlines phase */
            resources_chip_crtemu[3].factory_value = 750; /* oddlines offset */
        } else if (!strcmp(chipname, "TED")) {
            resources_chip_crtemu[2].factory_value = 1250; /* oddlines phase */
            resources_chip_crtemu[3].factory_value = 750; /* oddlines offset */
        }

        if (resources_register_int(resources_chip_crtemu) < 0) {
            return -1;
        }

        i = 0;
        while (vname_chip_crtemu[i]) {
            lib_free(resources_chip_crtemu[i].name);
            ++i;
        }
    } else {
        set_pal_scanlineshade(1000, (void *)*canvas);
        set_pal_blur(0, (void *)*canvas);
        set_pal_oddlinesphase(1000, (void *)*canvas);
        set_pal_oddlinesoffset(1000, (void *)*canvas);
        set_delaylinetype(0, (void *)*canvas);
        set_audioleak(0, (void *)*canvas);
    }

    /* ${CHIP}Filter */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_chip_rendermode[0].name
            = util_concat(chipname, "Filter", NULL);
        resources_chip_rendermode[0].value_ptr
            = &((*canvas)->videoconfig->filter);
        resources_chip_rendermode[0].param = *canvas;
        if (resources_register_int(resources_chip_rendermode) < 0) {
            lib_free(resources_chip_rendermode[0].name);
            return -1;
        }

        lib_free(resources_chip_rendermode[0].name);
    } else {
        set_chip_rendermode(VIDEO_FILTER_NONE, *canvas);
    }

    /* CHIPShowStatusbar */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_chip_show_statusbar[0].name
            = util_concat(chipname, "ShowStatusbar", NULL);
        resources_chip_show_statusbar[0].value_ptr
            = &((*canvas)->videoconfig->show_statusbar);
        resources_chip_show_statusbar[0].param = (void *)*canvas;

        if (resources_register_int(resources_chip_show_statusbar) < 0) {
            lib_free(resources_chip_show_statusbar[0].name);
            return -1;
        }
        lib_free(resources_chip_show_statusbar[0].name);
    }

    return 0;
}

void video_resources_chip_shutdown(struct video_canvas_s *canvas)
{
    lib_free(canvas->videoconfig->external_palette_name);
    lib_free(canvas->videoconfig->chip_name);

    /* NOTE: in x128 this actually shuts down the respective resources of both
     *       videochips at once. this is not exactly clean, but shouldnt matter
     *       in practise either.
     */
    shutdown_resource_chip_mode();
}
