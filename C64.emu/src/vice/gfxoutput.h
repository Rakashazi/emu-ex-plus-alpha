/*
 * gfxoutput.h - Graphics output driver.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_GFXOUTPUT_H
#define VICE_GFXOUTPUT_H

#include "types.h"

struct screenshot_s;

typedef struct gfxoutputdrv_codec_s {
    int id;
    const char *name;
} gfxoutputdrv_codec_t;

/* bits for flags in gfxoutputdrv_format_t */
#define GFXOUTPUTDRV_HAS_AUDIO_CODECS           (1 << 0)
#define GFXOUTPUTDRV_HAS_VIDEO_CODECS           (1 << 1)
#define GFXOUTPUTDRV_HAS_AUDIO_BITRATE          (1 << 2)
#define GFXOUTPUTDRV_HAS_VIDEO_BITRATE          (1 << 3)
#define GFXOUTPUTDRV_HAS_HALF_VIDEO_FRAMERATE   (1 << 4)

typedef struct gfxoutputdrv_format_s {
    char *name;
    gfxoutputdrv_codec_t *audio_codecs;
    gfxoutputdrv_codec_t *video_codecs;
    unsigned int flags;
} gfxoutputdrv_format_t;

typedef struct gfxoutputdrv_s {
    const char *name;
    const char *displayname;
    const char *default_extension;
    gfxoutputdrv_format_t *formatlist;
    int (*open)(struct screenshot_s *, const char *);
    int (*close)(struct screenshot_s *);
    int (*write)(struct screenshot_s *);
    int (*save)(struct screenshot_s *, const char *);
    int (*save_native)(struct screenshot_s *, const char *);
    int (*record)(struct screenshot_s *);
    void (*shutdown)(void);
    int (*resources_init)(void);
    int (*cmdline_options_init)(void);
#ifdef FEATURE_CPUMEMHISTORY
    int (*savememmap)(const char *, int, int, uint8_t *, uint8_t *);
#endif
} gfxoutputdrv_t;

/* Functions called by external emulator code.  */
int gfxoutput_resources_init(void);
int gfxoutput_cmdline_options_init(void);
int gfxoutput_early_init(int help);
int gfxoutput_init(void);
void gfxoutput_shutdown(void);
int gfxoutput_num_drivers(void);
gfxoutputdrv_t *gfxoutput_drivers_iter_init(void);
gfxoutputdrv_t *gfxoutput_drivers_iter_next(void);
gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname);

/* Functions called by graphic output driver modules.  */
int gfxoutput_register(gfxoutputdrv_t *drv);

/* FFMPEG bitrate constants. */
#define VICE_FFMPEG_VIDEO_RATE_MIN      100000
#define VICE_FFMPEG_VIDEO_RATE_MAX      10000000
#define VICE_FFMPEG_VIDEO_RATE_DEFAULT  800000
#define VICE_FFMPEG_AUDIO_RATE_MIN      16000
#define VICE_FFMPEG_AUDIO_RATE_MAX      384000
#define VICE_FFMPEG_AUDIO_RATE_DEFAULT  64000

/* Native screenshot drivers definitions */
enum {
    NATIVE_SS_OVERSIZE_SCALE = 0,
    NATIVE_SS_OVERSIZE_CROP_LEFT_TOP,
    NATIVE_SS_OVERSIZE_CROP_CENTER_TOP,
    NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP,
    NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER,
    NATIVE_SS_OVERSIZE_CROP_CENTER,
    NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER,
    NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM,
    NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM,
    NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM
};

enum {
    NATIVE_SS_UNDERSIZE_SCALE = 0,
    NATIVE_SS_UNDERSIZE_BORDERIZE
};

enum {
    NATIVE_SS_MC2HR_BLACK_WHITE = 0,
    NATIVE_SS_MC2HR_2_COLORS,
    NATIVE_SS_MC2HR_4_COLORS,
    NATIVE_SS_MC2HR_GRAY,
    NATIVE_SS_MC2HR_DITHER
};

enum {
    NATIVE_SS_TED_LUM_IGNORE = 0,
    NATIVE_SS_TED_LUM_DITHER
};

enum {
    NATIVE_SS_CRTC_WHITE = 0,
    NATIVE_SS_CRTC_AMBER,
    NATIVE_SS_CRTC_GREEN
};

#endif
