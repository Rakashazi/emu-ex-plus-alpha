/*
 * driver-select.h - Select a printer driver.
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

#ifndef VICE_DRIVER_SELECT_H
#define VICE_DRIVER_SELECT_H

#include "types.h"

struct driver_select_s {
    const char *drv_name;
    int (*drv_open)(unsigned int prnr, unsigned int secondary);
    void (*drv_close)(unsigned int prnr, unsigned int secondary);
    int (*drv_putc)(unsigned int prnr, unsigned int secondary, BYTE b);
    int (*drv_getc)(unsigned int prnr, unsigned int secondary, BYTE *b);
    int (*drv_flush)(unsigned int prnr, unsigned int secondary);
    int (*drv_formfeed)(unsigned int prnr);
};
typedef struct driver_select_s driver_select_t;

#define NUM_DRIVER_SELECT       4       /* same as NUM_OUTPUT_SELECT */
#define DRIVER_FIRST_OPEN       0xFFFF
#define DRIVER_LAST_CLOSE       0xFFFF

extern void driver_select_init(void);
extern int driver_select_init_resources(void);
extern int driver_select_userport_init_resources(void);
extern void driver_select_shutdown(void);
extern int driver_select_init_cmdline_options(void);
extern int driver_select_userport_init_cmdline_options(void);

extern void driver_select_register(driver_select_t *driver_select);

extern int driver_select_open(unsigned int prnr, unsigned int secondary);
extern void driver_select_close(unsigned int prnr, unsigned int secondary);
extern int driver_select_putc(unsigned int prnr, unsigned int secondary,
                              BYTE b);
extern int driver_select_getc(unsigned int prnr, unsigned int secondary,
                              BYTE *b);
extern int driver_select_flush(unsigned int prnr, unsigned int secondary);
extern int driver_select_formfeed(unsigned int prnr);

#endif
