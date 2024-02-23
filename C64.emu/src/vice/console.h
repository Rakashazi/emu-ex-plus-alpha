/*
 * console.h - Console access interface.
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

#ifndef VICE_CONSOLE_H
#define VICE_CONSOLE_H

struct console_private_s;

typedef struct console_s {
    /* Console geometry.  */
    /* Be careful - geometry might change at run-time! */
    unsigned int console_xres;
    unsigned int console_yres;

    /* It is allowed to leave the console open after control is given back
       to the emulation.  */
    int console_can_stay_open;

    /* == 0 if the console can output anything;
     * != 0 if it cannot (for example, because it is closed).
     */
    int console_cannot_output;

    struct console_private_s *private_;
} console_t;

/* first set of functions, which will work with whatever "console" the respective
   (g)ui/port comes up with. may or may not be equal to the native console */
console_t *console_open(const char *id);
int console_close(console_t *log);

/* the following must be called before any other
  console_...() function is used */
int console_init(void);

/* The following should be called when quitting VICE
 after calling console_close_all(), the console_...()
 functions cannot be accessed unless console_init()
 is called again.
*/
int console_close_all(void);

int console_out(console_t *log, const char *format, ...) VICE_ATTR_PRINTF2;
int console_flush(console_t *log);
char *console_in(console_t *log, const char *prompt);

/* the same set of functions, which will work with a "native" console of the host */
console_t *native_console_open(const char *id);
int native_console_close(console_t *log);

/* the following must be called before any other
  native_console_...() function is used */
int native_console_init(void);

/* The following should be called when quitting VICE
 after calling native_console_close_all(), the native_console_...()
 functions cannot be accessed unless native_console_init()
 is called again.
*/
int native_console_close_all(void);

int native_console_out(console_t *log, const char *format, ...) VICE_ATTR_PRINTF2;
int native_console_petscii_out(console_t *log, const char *format, ...) VICE_ATTR_PRINTF2;
int native_console_flush(console_t *log);
char *native_console_in(console_t *log, const char *prompt);

#endif
