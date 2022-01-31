/*
 * uimon.h - Monitor access interface.
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifndef VICE_UIMON_H
#define VICE_UIMON_H

#include <stdbool.h>

struct console_s;
struct console_private_s;
struct monitor_interface_s;

extern struct console_s *uimon_window_open(bool display_now);
extern void uimon_window_suspend(void);
extern struct console_s *uimon_window_resume(void);
extern void uimon_window_close(void);

extern int uimon_out(const char *buffer);

extern char *uimon_in(const char *prompt);

extern void uimon_notify_change(void);
extern void uimon_set_interface(struct monitor_interface_s **, int);
extern char *uimon_get_in(char **, const char *);

extern int uimon_get_columns(struct console_private_s *t);
extern void uimon_write_to_terminal(struct console_private_s *t, const char *data, long length);
extern int uimon_get_string(struct console_private_s *t, char* string, int string_len);

/* Gtk3-specific, so far */
extern bool uimon_set_font(void);
extern bool uimon_set_foreground_color(const char *color);
extern bool uimon_set_background_color(const char *color);


#endif
