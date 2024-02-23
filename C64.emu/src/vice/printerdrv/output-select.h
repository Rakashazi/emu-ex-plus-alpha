/*
 * output-select.h - Select an output driver.
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

#ifndef VICE_OUTPUT_SELECT_H
#define VICE_OUTPUT_SELECT_H

#include "types.h"

struct output_parameter_s;

struct output_select_s {
    char *output_name;
    int (*output_open)(unsigned int prnr, struct output_parameter_s *output_parameter);
    void (*output_close)(unsigned int prnr);
    int (*output_putc)(unsigned int prnr, uint8_t b);
    int (*output_getc)(unsigned int prnr, uint8_t *b);
    int (*output_flush)(unsigned int prnr);
    int (*output_formfeed)(unsigned int prnr);
};
typedef struct output_select_s output_select_t;

#define NUM_OUTPUT_SELECT       4 /* serial dev 4, 5, 6 and user port */

int output_select_init_resources(void);
int output_select_userport_init_resources(void);
void output_select_shutdown(void);
int output_select_init_cmdline_options(void);
int output_select_userport_init_cmdline_options(void);

void output_select_register(output_select_t *output_select);

int output_select_open(unsigned int prnr, struct output_parameter_s *output_parameter);
void output_select_close(unsigned int prnr);
int output_select_putc(unsigned int prnr, uint8_t b);
int output_select_getc(unsigned int prnr, uint8_t *b);
int output_select_flush(unsigned int prnr);
int output_select_formfeed(unsigned int prnr);
void output_select_writeline(unsigned int prnr);

#endif
