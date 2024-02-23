/*
 * initcmdline.c - Initial command line options.
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

#ifndef VICE_INITCMDLINE_H
#define VICE_INITCMDLINE_H

int initcmdline_init(void);
int initcmdline_check_psid(void);
int initcmdline_check_args(int argc, char **argv);
void initcmdline_check_attach(void);
int cmdline_get_autostart_mode(void);
void cmdline_set_autostart_mode(int mode);
void initcmdline_shutdown(void);

#endif
