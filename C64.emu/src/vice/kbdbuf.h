/*
 * kbdbuf.h - Kernal keyboard buffer handling for VICE.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_KBDBUF_H
#define VICE_KBDBUF_H

#include "types.h"

extern int kbdbuf_is_empty(void);
extern void kbdbuf_init(int location, int plocation, int buffer_size, CLOCK mincycles);
extern void kbdbuf_shutdown(void);
extern void kbdbuf_reset(int location, int plocation, int buffer_size, CLOCK mincycles);
extern int kbdbuf_feed(const char *s);
extern int kbdbuf_feed_runcmd(const char *string);
extern int kbdbuf_feed_string(const char *string);
extern void kbdbuf_feed_cmdline(void);
extern void kbdbuf_flush(void);
extern int kbdbuf_cmdline_options_init(void);
extern int kbdbuf_resources_init(void);

#endif
