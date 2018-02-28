/** \file   src/screenshot.c
 * \brief   Screenshot/video recording module
 *
 * screenshot.c - Create a screenshot.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "translate.h"
#include "uiapi.h"
#include "video.h"


static log_t screenshot_log = LOG_ERR;
static gfxoutputdrv_t *recording_driver;
static struct video_canvas_s *recording_canvas;

static int reopen = 0;
static char *reopen_recording_drivername;
static struct video_canvas_s *reopen_recording_canvas;
static char *reopen_filename;


/** \brief  Initialize module
 *
 * \return  0 on success (always)
 */
int screenshot_init(void)
{
    /* Setup logging system.  */
    screenshot_log = log_open("Screenshot");
    recording_driver = NULL;
    recording_canvas = NULL;

    reopen_recording_drivername = NULL;
    reopen_filename = NULL;
    return 0;
}


/** \brief  Clean up memory resources used by the module
 */
void screenshot_shutdown(void)
{
    if (reopen_recording_drivername != NULL) {
        lib_free(reopen_recording_drivername);
    }
    if (reopen_filename != NULL) {
        lib_free(reopen_filename);
    }
}



/*-----------------------------------------------------------------------*/

static void screenshot_line_data(screenshot_t *screenshot, BYTE *data,
                                 unsigned int line, unsigned int mode)
{
    unsigned int i;
    BYTE *line_base;
    BYTE color;

    if (line > screenshot->height) {
        log_error(screenshot_log, "Invalild line `%i' request.", line);
        return;
    }

#define BUFFER_LINE_START(i, n) ((i)->draw_buffer + (n) * (i)->draw_buffer_line_size)

    line_base = BUFFER_LINE_START(screenshot,
                                  (line + screenshot->y_offset)
                                  * screenshot->size_height);

    switch (mode) {
        case SCREENSHOT_MODE_PALETTE:
            for (i = 0; i < screenshot->width; i++) {
                data[i] = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
            }
            break;
        case SCREENSHOT_MODE_RGB32:
            for (i = 0; i < screenshot->width; i++) {
                color = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
                data[i * 4] = screenshot->palette->entries[color].red;
                data[i * 4 + 1] = screenshot->palette->entries[color].green;
                data[i * 4 + 2] = screenshot->palette->entries[color].blue;
                data[i * 4 + 3] = 0;
            }
            break;
        case SCREENSHOT_MODE_RGB24:
            for (i = 0; i < screenshot->width; i++) {
                color = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
                data[i * 3] = screenshot->palette->entries[color].red;
                data[i * 3 + 1] = screenshot->palette->entries[color].green;
                data[i * 3 + 2] = screenshot->palette->entries[color].blue;
            }
            break;
        default:
            log_error(screenshot_log, "Invalid mode %i.", mode);
    }
}

/*-----------------------------------------------------------------------*/
static int screenshot_save_core(screenshot_t *screenshot, gfxoutputdrv_t *drv,
                                const char *filename)
{
    unsigned int i;

    screenshot->width = screenshot->max_width & ~3;
    screenshot->height = screenshot->last_displayed_line - screenshot->first_displayed_line + 1;
    screenshot->y_offset = screenshot->first_displayed_line;

    screenshot->color_map = lib_calloc(1, 256);

    for (i = 0; i < screenshot->palette->num_entries; i++) {
        screenshot->color_map[i] = i;
    }

    screenshot->convert_line = screenshot_line_data;

    if (drv != NULL) {
        if (drv->save_native != NULL) {
            /* It's a native screenshot. */
            if ((drv->save_native)(screenshot, filename) < 0) {
                log_error(screenshot_log, "Saving failed...");
                lib_free(screenshot->color_map);
                return -1;
            }
        } else {
            /* It's a usual screenshot. */
            if ((drv->save)(screenshot, filename) < 0) {
                log_error(screenshot_log, "Saving failed...");
                lib_free(screenshot->color_map);
                return -1;
            }
        }
    } else {
        /* We're recording a movie */
        if ((recording_driver->record)(screenshot) < 0) {
            log_error(screenshot_log, "Recording failed...");
            lib_free(screenshot->color_map);
            return -1;
        }
    }

    lib_free(screenshot->color_map);
    return 0;
}

/*-----------------------------------------------------------------------*/

int screenshot_save(const char *drvname, const char *filename,
                    struct video_canvas_s *canvas)
{
    screenshot_t screenshot;
    gfxoutputdrv_t *drv;
    int result;
    /* printf("screenshot_save(%s, %s, ...)\n", drvname, filename); */
    if ((drv = gfxoutput_get_driver(drvname)) == NULL) {
        return -1;
    }

    if (recording_driver == drv) {
        ui_error(translate_text(IDGS_SORRY_NO_MULTI_RECORDING));
        return -1;
    }

    if (machine_screenshot(&screenshot, canvas) < 0) {
        log_error(screenshot_log, "Retrieving screen geometry failed.");
        return -1;
    }

    if (drv->record != NULL) {
        recording_driver = drv;
        recording_canvas = canvas;

        reopen_recording_drivername = lib_stralloc(drvname);
        reopen_recording_canvas = canvas;
        reopen_filename = lib_stralloc(filename);
    }

    result = screenshot_save_core(&screenshot, drv, filename);

    if (result < 0) {
        recording_driver = NULL;
        recording_canvas = NULL;
    }

    return result;
}

#ifdef FEATURE_CPUMEMHISTORY
int memmap_screenshot_save(const char *drvname, const char *filename, int x_size, int y_size, BYTE *gfx, BYTE *palette)
{
    gfxoutputdrv_t *drv;

    if ((drv = gfxoutput_get_driver(drvname)) == NULL) {
        return -1;
    }

    if ((drv->savememmap)(filename, x_size, y_size, gfx, palette) < 0) {
        log_error(screenshot_log, "Saving failed...");
        return -1;
    }
    return 0;
}
#endif

int screenshot_record(void)
{
    screenshot_t screenshot;

    if (recording_driver == NULL) {
        return 0;
    }

    /* Retrive framebuffer and screen geometry.  */
    if (recording_canvas != NULL) {
        if (machine_screenshot(&screenshot, recording_canvas) < 0) {
            log_error(screenshot_log, "Retrieving screen geometry failed.");
            return -1;
        }
    } else {
        /* should not happen */
        log_error(screenshot_log, "Canvas is unknown.");
        return -1;
    }

    return screenshot_save_core(&screenshot, NULL, NULL);
}

void screenshot_stop_recording(void)
{
    if (recording_driver != NULL && recording_driver->close != NULL) {
        recording_driver->close(NULL);
    }

    recording_driver = NULL;
    recording_canvas = NULL;
}

int screenshot_is_recording(void)
{
    return (recording_driver == NULL ? 0 : 1);
}

void screenshot_prepare_reopen(void)
{
    reopen = (screenshot_is_recording() ? 1 : 0);
}

void screenshot_try_reopen(void)
{
    if (reopen == 1) {
        screenshot_save(reopen_recording_drivername,
                        reopen_filename,
                        reopen_recording_canvas);
    }
    reopen = 0;
}
