/*
 * printer.h - Common external printer interface.
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

#ifndef VICE_PRINTER_H
#define VICE_PRINTER_H

#include "types.h"

/* Generic interface.  */
extern int printer_resources_init(void);
extern int printer_userport_resources_init(void);
extern void printer_resources_shutdown(void);
extern int printer_cmdline_options_init(void);
extern int printer_userport_cmdline_options_init(void);
extern void printer_init(void);
extern void printer_reset(void);
extern void printer_formfeed(unsigned int unit);
extern void printer_shutdown(void);

/* Serial interface.  */
#define PRINTER_IEC_NUM 2

#define PRINTER_DEVICE_NONE 0
#define PRINTER_DEVICE_FS   1
#define PRINTER_DEVICE_REAL 2

extern int printer_serial_init_resources(void);
extern int printer_serial_init_cmdline_options(void);
extern void printer_serial_init(void);
extern int printer_serial_close(unsigned int unit);
extern int printer_serial_late_init(void);
extern void printer_serial_shutdown(void);

/* Userport interface.  */
extern int printer_userport_init_resources(void);
extern int printer_userport_init_cmdline_options(void);
extern void printer_userport_init(void (*set_busy)(unsigned int));
extern void printer_userport_write_data(BYTE b);
extern void printer_userport_write_strobe(int s);

#endif
