/*
 * video-cmdline-options.c
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "util.h"
#include "video.h"

int video_cmdline_options_init(void)
{
    return video_arch_cmdline_options_init();
}

static const char * const cname_chip_size[] =
{
    "-", "dsize", "DoubleSize",
    "+", "dsize", "DoubleSize",
    NULL
};

static cmdline_option_t cmdline_options_chip_size[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable double size" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable double size" },
    CMDLINE_LIST_END
};

static const char * const cname_chip_scan[] =
{
    "-", "dscan", "DoubleScan",
    "+", "dscan", "DoubleScan",
    NULL
};

static cmdline_option_t cmdline_options_chip_scan[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable double scan" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable double scan" },
    CMDLINE_LIST_END
};

static const char * const cname_chip_audioleak[] =
{
    "-", "audioleak", "AudioLeak",
    "+", "audioleak", "AudioLeak",
    NULL
};

static cmdline_option_t cmdline_options_chip_audioleak[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable audio leak emulation" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable audio leak emulation" },
    CMDLINE_LIST_END
};

static const char * const cname_chip_render_filter[] =
{
    "-", "filter", "Filter",
    NULL
};

static cmdline_option_t cmdline_options_chip_render_filter[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Mode>", "Select rendering filter: (0: none, 1: CRT emulation, 2: scale2x)" },
    CMDLINE_LIST_END
};

static const char * const cname_chip_internal_palette[] =
{
    "-", "intpal", "ExternalPalette",
    "-", "extpal", "ExternalPalette",
    NULL
};

static cmdline_option_t cmdline_options_chip_internal_palette[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Use an internal calculated palette" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Use an external palette (file)" },
    CMDLINE_LIST_END
};

static const char * const cname_chip_palette[] =
{
    "-", "palette", "PaletteFile",
    NULL
};

static cmdline_option_t cmdline_options_chip_palette[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Name>", "Specify name of file of external palette" },
    CMDLINE_LIST_END
};

#if defined(USE_SDLUI) || defined(USE_SDL2UI) || defined(USE_GTK3UI)
static const char * const cname_chip_fullscreen[] =
{
    "-", "full", "Fullscreen",
    "+", "full", "Fullscreen",
    NULL
};
#endif

#if defined(USE_SDLUI) || defined(USE_SDL2UI) || defined(USE_GTK3UI)
static cmdline_option_t cmdline_options_chip_fullscreen[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable fullscreen" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable fullscreen" },
    CMDLINE_LIST_END
};
#endif

#if defined(USE_SDLUI) || defined(USE_SDL2UI)
static const char * const cname_chip_fullscreen_mode[] =
{
    "-", "fullmode", "FullscreenMode",
    "-", "fullwidth", "FullscreenCustomWidth",
    "-", "fullheight", "FullscreenCustomHeight",
    NULL
};

static cmdline_option_t cmdline_options_chip_fullscreen_mode[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Mode>", "Select fullscreen mode" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<width>", "Set custom fullscreen resolution width" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<height>", "Set custom fullscreen resolution height" },
    CMDLINE_LIST_END
};
#endif

/** \brief  Template for [-+]CHIPshowstatusbar command line options
 */
static cmdline_option_t cmdline_options_chip_show_statusbar[] =
{
    /* -CHIPshowstatusbar */
    { NULL,                 /* option name: filled in */
      SET_RESOURCE,         /* set provided resource */
      CMDLINE_ATTRIB_NONE,  /* boolean option, no arg */
      NULL,                 /* function to set the resource (not used) */
      NULL,                 /* extra param for the setter function (not used) */
      NULL,                 /* resource name: filled in */
      (void*)1,             /* resource value */
      NULL,                 /* param name: none */
      "Show status bar",    /* option description */
    },
    /* +CHIPshowstatusbar */
    { NULL,                 /* option name: filled in */
      SET_RESOURCE,         /* set provided resource */
      CMDLINE_ATTRIB_NONE,  /* boolean option, no arg */
      NULL,                 /* function to set the resource (not used) */
      NULL,                 /* extra param for the setter function (not used) */
      NULL,                 /* resource name: filled in */
      (void*)0,             /* resource value */
      NULL,                 /* param name: none */
      "Hide status bar",    /* option description */
    },
    CMDLINE_LIST_END
};

/* aspect options */
static const char * const cname_chip_gloptions[] =
{
    "-", "aspectmode", "AspectMode",
    "-", "aspect", "AspectRatio",
    "-", "glfilter", "GLFilter",
    "-", "flipx", "FLipX",
    "+", "flipx", "FLipX",
    "-", "flipy", "FLipY",
    "+", "flipy", "FLipY",
    "-", "rotate", "Rotate",
    "+", "rotate", "Rotate",
    "-", "vsync", "VSync",
    "+", "vsync", "VSync",
    NULL
};

static cmdline_option_t cmdline_options_chip_gloptions[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<mode>", "Set aspect ratio mode (0 = off, 1 = custom, 2 = true)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<aspect ratio>", "Set custom aspect ratio (0.5 - 2.0)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, (resource_value_t)VIDEO_GLFILTER_BICUBIC,
      "<mode>", "Set OpenGL filtering mode (0 = nearest, 1 = linear, 2 = bicubic)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)1,
      NULL, "Enable X flip" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)0,
      NULL, "Disable X flip" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)1,
      NULL, "Enable Y flip" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)0,
      NULL, "Disable Y flip" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)1,
      NULL, "Rotate 90 degrees clockwise" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)0,
      NULL, "Do not rotate" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)1,
      NULL, "Enable vsync to prevent screen tearing" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (resource_value_t)0,
      NULL, "Disable vsync to allow screen tearing" },
    CMDLINE_LIST_END
};

/* CRT emulation options */
static const char * const cname_chip_colors[] =
{
    "-", "saturation", "ColorSaturation",
    "-", "contrast", "ColorContrast",
    "-", "brightness", "ColorBrightness",
    "-", "gamma", "ColorGamma",
    "-", "tint", "ColorTint",
    NULL
};

static cmdline_option_t cmdline_options_chip_colors[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set saturation of internal calculated palette" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set contrast of internal calculated palette" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set brightness of internal calculated palette" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-4000>", "Set gamma of internal calculated palette" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set tint of internal calculated palette" },
    CMDLINE_LIST_END
};

/* PAL/NTSC emulation options */
static const char * const cname_chip_crtemu_palntsc[] =
{
    "-", "oddlinesphase", "PALOddLinePhase",
    "-", "oddlinesoffset", "PALOddLineOffset",
    "-", "crtdelaylinetype", "PALDelaylineType",
    NULL
};

static cmdline_option_t cmdline_options_chip_crtemu_palntsc[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set phase for color carrier in odd lines" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-2000>", "Set phase offset for color carrier in odd lines" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<type>", "Set type of delay line used in the CRT (0: normal, 1: U only (1084 style))." },
    CMDLINE_LIST_END
};

/* CRT emulation options */
static const char * const cname_chip_crtemu[] =
{
    "-", "crtblur", "PALBlur",
    "-", "crtscanlineshade", "PALScanLineShade",
    NULL
};

static cmdline_option_t cmdline_options_chip_crtemu[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-1000>", "Amount of horizontal blur for the CRT emulation." },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<0-1000>", "Amount of scan line shading for the CRT emulation" },
    CMDLINE_LIST_END
};

int video_cmdline_options_chip_init(const char *chipname,
                                    video_chip_cap_t *video_chip_cap)
{
    unsigned int i;

    if (machine_class == VICE_MACHINE_VSID) {
        return 0;
    }

    if (video_chip_cap->dsize_allowed) {
        for (i = 0; cname_chip_size[i * 3] != NULL; i++) {
            cmdline_options_chip_size[i].name
                = util_concat(cname_chip_size[i * 3], chipname,
                              cname_chip_size[i * 3 + 1], NULL);
            cmdline_options_chip_size[i].resource_name
                = util_concat(chipname, cname_chip_size[i * 3 + 2], NULL);
        }

        if (cmdline_register_options(cmdline_options_chip_size) < 0) {
            return -1;
        }

        for (i = 0; cname_chip_size[i * 3] != NULL; i++) {
            lib_free(cmdline_options_chip_size[i].name);
            lib_free(cmdline_options_chip_size[i].resource_name);
        }
    }

    if (video_chip_cap->dscan_allowed) {
        for (i = 0; cname_chip_scan[i * 3] != NULL; i++) {
            cmdline_options_chip_scan[i].name
                = util_concat(cname_chip_scan[i * 3], chipname,
                              cname_chip_scan[i * 3 + 1], NULL);
            cmdline_options_chip_scan[i].resource_name
                = util_concat(chipname, cname_chip_scan[i * 3 + 2], NULL);
        }

        if (cmdline_register_options(cmdline_options_chip_scan) < 0) {
            return -1;
        }

        for (i = 0; cname_chip_scan[i * 3] != NULL; i++) {
            lib_free(cmdline_options_chip_scan[i].name);
            lib_free(cmdline_options_chip_scan[i].resource_name);
        }
    }

    for (i = 0; cname_chip_audioleak[i * 3] != NULL; i++) {
        cmdline_options_chip_audioleak[i].name
            = util_concat(cname_chip_audioleak[i * 3], chipname,
                          cname_chip_audioleak[i * 3 + 1], NULL);
        cmdline_options_chip_audioleak[i].resource_name
            = util_concat(chipname, cname_chip_audioleak[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_audioleak) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_audioleak[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_audioleak[i].name);
        lib_free(cmdline_options_chip_audioleak[i].resource_name);
    }

    /* video render filters */

    /* <CHIP>Filter */
    for (i = 0; cname_chip_render_filter[i * 3] != NULL; i++) {
        cmdline_options_chip_render_filter[i].name
            = util_concat(cname_chip_render_filter[i * 3], chipname,
                            cname_chip_render_filter[i * 3 + 1], NULL);
        cmdline_options_chip_render_filter[i].resource_name
            = util_concat(chipname, cname_chip_render_filter[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_render_filter) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_render_filter[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_render_filter[i].name);
        lib_free(cmdline_options_chip_render_filter[i].resource_name);
    }

    /* <CHIP>ExternalPalette */
    for (i = 0; cname_chip_internal_palette[i * 3] != NULL; i++) {
        cmdline_options_chip_internal_palette[i].name
            = util_concat(cname_chip_internal_palette[i * 3], chipname,
                            cname_chip_internal_palette[i * 3 + 1], NULL);
        cmdline_options_chip_internal_palette[i].resource_name
            = util_concat(chipname, cname_chip_internal_palette[i * 3 + 2],
                            NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_internal_palette)
        < 0) {
        return -1;
    }

    for (i = 0; cname_chip_internal_palette[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_internal_palette[i].name);
        lib_free(cmdline_options_chip_internal_palette[i].resource_name);
    }

    /* <CHIP>PaletteFile */
    for (i = 0; cname_chip_palette[i * 3] != NULL; i++) {
        cmdline_options_chip_palette[i].name
            = util_concat(cname_chip_palette[i * 3], chipname,
                          cname_chip_palette[i * 3 + 1], NULL);
        cmdline_options_chip_palette[i].resource_name
            = util_concat(chipname, cname_chip_palette[i * 3 + 2],
                          NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_palette) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_palette[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_palette[i].name);
        lib_free(cmdline_options_chip_palette[i].resource_name);
    }

    /* fullscreen options */
#if defined(USE_SDLUI) || defined(USE_SDL2UI) || defined(USE_GTK3UI)
    /* <CHIP>Fullscreen */
    for (i = 0; cname_chip_fullscreen[i * 3] != NULL; i++) {
        cmdline_options_chip_fullscreen[i].name
            = util_concat(cname_chip_fullscreen[i * 3], chipname,
                          cname_chip_fullscreen[i * 3 + 1], NULL);
        cmdline_options_chip_fullscreen[i].resource_name
            = util_concat(chipname, cname_chip_fullscreen[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_fullscreen) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_fullscreen[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_fullscreen[i].name);
        lib_free(cmdline_options_chip_fullscreen[i].resource_name);
    }
#endif

#if defined(USE_SDLUI) || defined(USE_SDL2UI)
    /* <CHIP>FullscreenMode (SDL only) */
    for (i = 0; cname_chip_fullscreen_mode[i * 3] != NULL; i++) {
        cmdline_options_chip_fullscreen_mode[i].name
            = util_concat(cname_chip_fullscreen_mode[i * 3], chipname,
                          cname_chip_fullscreen_mode[i * 3 + 1], NULL);
        cmdline_options_chip_fullscreen_mode[i].resource_name
            = util_concat(chipname,
                          cname_chip_fullscreen_mode[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_fullscreen_mode)
        < 0) {
        return -1;
    }

    for (i = 0; cname_chip_fullscreen_mode[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_fullscreen_mode[i].name);
        lib_free(cmdline_options_chip_fullscreen_mode[i].resource_name);
    }
#endif

    /* show status bar */

    /* <CHIP>ShowStatusbar */
    cmdline_options_chip_show_statusbar[0].name
        = util_concat("-", chipname, "showstatusbar", NULL);
    cmdline_options_chip_show_statusbar[0].resource_name
        = util_concat(chipname, "ShowStatusbar", NULL);
    /* hide status bar */
    cmdline_options_chip_show_statusbar[1].name
        = util_concat("+", chipname, "showstatusbar", NULL);
    cmdline_options_chip_show_statusbar[1].resource_name
        = util_concat(chipname, "ShowStatusbar", NULL);
    if (cmdline_register_options(cmdline_options_chip_show_statusbar) < 0) {
        lib_free(cmdline_options_chip_show_statusbar[0].name);
        lib_free(cmdline_options_chip_show_statusbar[0].resource_name);
        lib_free(cmdline_options_chip_show_statusbar[1].name);
        lib_free(cmdline_options_chip_show_statusbar[1].resource_name);
        return -1;
    }
    lib_free(cmdline_options_chip_show_statusbar[0].name);
    lib_free(cmdline_options_chip_show_statusbar[0].resource_name);
    lib_free(cmdline_options_chip_show_statusbar[1].name);
    lib_free(cmdline_options_chip_show_statusbar[1].resource_name);

    /* GL options */
    for (i = 0; cname_chip_gloptions[i * 3] != NULL; i++) {
        cmdline_options_chip_gloptions[i].name
            = util_concat(cname_chip_gloptions[i * 3], chipname,
                          cname_chip_gloptions[i * 3 + 1], NULL);
        cmdline_options_chip_gloptions[i].resource_name
            = util_concat(chipname, cname_chip_gloptions[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_gloptions) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_gloptions[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_gloptions[i].name);
        lib_free(cmdline_options_chip_gloptions[i].resource_name);
    }

    /* color generator */
    for (i = 0; cname_chip_colors[i * 3] != NULL; i++) {
        cmdline_options_chip_colors[i].name
            = util_concat(cname_chip_colors[i * 3], chipname,
                          cname_chip_colors[i * 3 + 1], NULL);
        cmdline_options_chip_colors[i].resource_name
            = util_concat(chipname, cname_chip_colors[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_colors) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_colors[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_colors[i].name);
        lib_free(cmdline_options_chip_colors[i].resource_name);
    }

    /* crt emulation */

    for (i = 0; cname_chip_crtemu[i * 3] != NULL; i++) {
        cmdline_options_chip_crtemu[i].name
            = util_concat(cname_chip_crtemu[i * 3], chipname,
                          cname_chip_crtemu[i * 3 + 1], NULL);
        cmdline_options_chip_crtemu[i].resource_name
            = util_concat(chipname, cname_chip_crtemu[i * 3 + 2], NULL);
    }

    if (cmdline_register_options(cmdline_options_chip_crtemu) < 0) {
        return -1;
    }

    for (i = 0; cname_chip_crtemu[i * 3] != NULL; i++) {
        lib_free(cmdline_options_chip_crtemu[i].name);
        lib_free(cmdline_options_chip_crtemu[i].resource_name);
    }

    /* PAL/NTSC emulation */
    if (video_chip_cap->video_has_palntsc) {
        for (i = 0; cname_chip_crtemu_palntsc[i * 3] != NULL; i++) {
            cmdline_options_chip_crtemu_palntsc[i].name
                = util_concat(cname_chip_crtemu_palntsc[i * 3], chipname,
                            cname_chip_crtemu_palntsc[i * 3 + 1], NULL);
            cmdline_options_chip_crtemu_palntsc[i].resource_name
                = util_concat(chipname, cname_chip_crtemu_palntsc[i * 3 + 2], NULL);
        }

        if (cmdline_register_options(cmdline_options_chip_crtemu_palntsc) < 0) {
            return -1;
        }

        for (i = 0; cname_chip_crtemu_palntsc[i * 3] != NULL; i++) {
            lib_free(cmdline_options_chip_crtemu_palntsc[i].name);
            lib_free(cmdline_options_chip_crtemu_palntsc[i].resource_name);
        }
    }

    return 0;
}
