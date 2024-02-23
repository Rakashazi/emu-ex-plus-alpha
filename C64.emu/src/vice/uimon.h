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

struct console_s *uimon_window_open(bool display_now);
void uimon_window_suspend(void);
struct console_s *uimon_window_resume(void);
void uimon_window_close(void);

int uimon_out(const char *buffer);

char *uimon_in(const char *prompt);

void uimon_notify_change(void);
void uimon_set_interface(struct monitor_interface_s **, int);
char *uimon_get_in(char **, const char *);

int uimon_get_columns(struct console_private_s *t);
void uimon_write_to_terminal(struct console_private_s *t, const char *data, long length);
int uimon_get_string(struct console_private_s *t, char* string, int string_len);

/* Gtk3-specific, so far */
bool uimon_set_font(void);
bool uimon_set_foreground_color(const char *color);
bool uimon_set_background_color(const char *color);


#endif
