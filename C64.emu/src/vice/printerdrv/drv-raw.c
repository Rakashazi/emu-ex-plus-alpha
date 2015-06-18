/*
 * drv-raw.c - RAW printer driver.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  groepaz <groepaz@gmx.net>
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

#include "driver-select.h"
#include "drv-raw.h"
#include "log.h"
#include "output-select.h"
#include "output.h"
#include "types.h"

/* #define DEBUG_PRINTER */

struct raw_s {
    int mode;
};
typedef struct raw_s raw_t;

static raw_t drv_raw[NUM_OUTPUT_SELECT];
static log_t drv_raw_log = LOG_ERR;

static int print_char(raw_t *raw, unsigned int prnr, const BYTE c)
{
    switch (c) {
        case 17: /* lowercase */
            if (raw->mode == 1) {
                return 0;
            }
            raw->mode = 1;
            break;
        case 145: /* uppercase */
            if (raw->mode == 0) {
                return 0;
            }
            raw->mode = 0;
            break;
    }
    if (output_select_putc(prnr, c) < 0) {
        return -1;
    }
    return 0;
}

static int drv_raw_open(unsigned int prnr, unsigned int secondary)
{
    raw_t *raw = &drv_raw[prnr];

    if (secondary == 7) {
        print_char(raw, prnr, 17);
    } else if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        /* these are unused for non gfx output */
        output_parameter.maxcol = 480;
        output_parameter.maxrow = 66 * 9;
        output_parameter.dpi_x = 100;
        output_parameter.dpi_y = 100;

        return output_select_open(prnr, &output_parameter);
    }

    return 0;
}

static void drv_raw_close(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_LAST_CLOSE) {
        output_select_close(prnr);
    }
}

static int drv_raw_putc(unsigned int prnr, unsigned int secondary, BYTE b)
{
#ifdef DEBUG_PRINTER
    log_message(drv_raw_log, "Print device #%i secondary %i data %02x.",
                prnr + 4, secondary, b);
#endif

    if (print_char(&drv_raw[prnr], prnr, b) < 0) {
        return -1;
    }

    return 0;
}

static int drv_raw_getc(unsigned int prnr, unsigned int secondary, BYTE *b)
{
    return output_select_getc(prnr, b);
}

static int drv_raw_flush(unsigned int prnr, unsigned int secondary)
{
    return output_select_flush(prnr);
}

static int drv_raw_formfeed(unsigned int prnr)
{
    return 0;
}

int drv_raw_init_resources(void)
{
    driver_select_t driver_select;

    driver_select.drv_name = "raw";
    driver_select.drv_open = drv_raw_open;
    driver_select.drv_close = drv_raw_close;
    driver_select.drv_putc = drv_raw_putc;
    driver_select.drv_getc = drv_raw_getc;
    driver_select.drv_flush = drv_raw_flush;
    driver_select.drv_formfeed = drv_raw_formfeed;

    driver_select_register(&driver_select);

    return 0;
}

void drv_raw_init(void)
{
    drv_raw_log = log_open("Drv-Raw");
}
