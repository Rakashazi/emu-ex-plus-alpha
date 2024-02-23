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

/* #define DEBUG_PRINTER */

#include "driver-select.h"
#include "drv-raw.h"
#include "log.h"
#include "output-select.h"
#include "output.h"
#include "types.h"

#ifdef DEBUG_PRINTER
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

static int drv_raw_open(unsigned int prnr, unsigned int secondary)
{
    DBG(("drv_raw_open device:%u", 4 + prnr));
    if (secondary == DRIVER_FIRST_OPEN) {
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
    DBG(("drv_raw_close device:%u", 4 + prnr));
    if (secondary == DRIVER_LAST_CLOSE) {
        output_select_close(prnr);
    }
}

static int drv_raw_putc(unsigned int prnr, unsigned int secondary, uint8_t b)
{
    DBG(("Print device #%u secondary %u data %02x.", prnr + 4, secondary, b));

    if (output_select_putc(prnr, b) < 0) {
        return -1;
    }
    return 0;
}

static int drv_raw_getc(unsigned int prnr, unsigned int secondary, uint8_t *b)
{
    return output_select_getc(prnr, b);
}

static int drv_raw_flush(unsigned int prnr, unsigned int secondary)
{
    DBG(("drv_raw_flush device #%u secondary %u.", prnr + 4, secondary));

    return output_select_flush(prnr);
}

static int drv_raw_formfeed(unsigned int prnr)
{
    DBG(("drv_raw_formfeed device #%u.", prnr + 4));

    return output_select_formfeed(prnr);
}

static int drv_raw_select(unsigned int prnr)
{
    DBG(("drv_raw_select device:%u", 4 + prnr));
    return 0;
}

int drv_raw_init_resources(void)
{
    driver_select_t driver_select = {
        .drv_name     = "raw",
        .ui_name      = "RAW",
        .drv_open     = drv_raw_open,
        .drv_close    = drv_raw_close,
        .drv_putc     = drv_raw_putc,
        .drv_getc     = drv_raw_getc,
        .drv_flush    = drv_raw_flush,
        .drv_formfeed = drv_raw_formfeed,
        .drv_select   = drv_raw_select,

        .printer      = true,
        .plotter      = true,
        .iec          = true,
        .ieee488      = true,
        .userport     = true,
        .text         = true,
        .graphics     = false
    };
    DBG(("drv_raw_init_resources"));
    driver_select_register(&driver_select);

    return 0;
}

void drv_raw_init(void)
{
    DBG(("drv_raw_init"));
}
