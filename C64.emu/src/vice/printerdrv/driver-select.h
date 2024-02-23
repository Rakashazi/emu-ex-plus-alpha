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
#include <stdbool.h>

/** \brief  Printer driver data */
typedef struct driver_select_s {
    char  *drv_name;    /**< driver name */
    char  *ui_name;     /**< driver name to display in UIs */
    int  (*drv_open)    (unsigned int prnr, unsigned int secondary);
    void (*drv_close)   (unsigned int prnr, unsigned int secondary);
    int  (*drv_putc)    (unsigned int prnr, unsigned int secondary, uint8_t b);
    int  (*drv_getc)    (unsigned int prnr, unsigned int secondary, uint8_t *b);
    int  (*drv_flush)   (unsigned int prnr, unsigned int secondary);
    int  (*drv_formfeed)(unsigned int prnr);
    int  (*drv_select)  (unsigned int prnr);

    bool   printer;     /**< device is a printer */
    bool   plotter;     /**< device is a plotter */
    bool   iec;         /**< device can be connected through IEC bus */
    bool   ieee488;     /**< device can be connected through IEEE-488 bus */
    bool   userport;    /**< device can be connected through userport */
    bool   text;        /**< device supports text mode */
    bool   graphics;    /**< device supports graphics mode */
} driver_select_t;

/** \brief  Linked list node for printer driver data */
typedef struct driver_select_list_s {
    driver_select_t              driver_select; /**< driver data */
    struct driver_select_list_s *next;          /**< next node in list */
} driver_select_list_t;


#define NUM_DRIVER_SELECT       4       /* same as NUM_OUTPUT_SELECT */
#define DRIVER_FIRST_OPEN       0xFFFF
#define DRIVER_LAST_CLOSE       0xFFFF

void driver_select_init(void);
int driver_select_init_resources(void);
int driver_select_userport_init_resources(void);
void driver_select_shutdown(void);

int driver_select_init_cmdline_options(void);
int driver_select_userport_init_cmdline_options(void);

void driver_select_register(driver_select_t *driver_select);

int driver_select_open(unsigned int prnr, unsigned int secondary);
void driver_select_close(unsigned int prnr, unsigned int secondary);
int driver_select_putc(unsigned int prnr, unsigned int secondary, uint8_t b);
int driver_select_getc(unsigned int prnr, unsigned int secondary, uint8_t *b);
int driver_select_flush(unsigned int prnr, unsigned int secondary);
int driver_select_formfeed(unsigned int prnr);

bool driver_select_is_printer         (const char *drv_name);
bool driver_select_is_plotter         (const char *drv_name);
bool driver_select_has_iec_bus        (const char *drv_name);
bool driver_select_has_ieee488_bus    (const char *drv_name);
bool driver_select_has_userport       (const char *drv_name);
bool driver_select_has_text_output    (const char *drv_name);
bool driver_select_has_graphics_output(const char *drv_name);

const driver_select_list_t *driver_select_get_drivers(void);

#endif
