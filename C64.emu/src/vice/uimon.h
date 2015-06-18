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

struct console_s;
struct monitor_interface_s;

extern struct console_s *uimon_window_open(void);
extern void uimon_window_suspend(void);
extern struct console_s *uimon_window_resume(void);
extern void uimon_window_close(void);

extern int uimon_out(const char *buffer);

extern char *uimon_in(const char *prompt);

extern void uimon_notify_change(void);
extern void uimon_set_interface(struct monitor_interface_s **, int);
extern char *uimon_get_in(char **, const char *);

#endif
