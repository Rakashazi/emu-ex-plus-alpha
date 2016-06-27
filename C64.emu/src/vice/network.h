/*
 * network.c - Connecting emulators via network.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifndef VICE_NETWORK_H
#define VICE_NETWORK_H

typedef enum {
    NETWORK_IDLE,
    NETWORK_SERVER,
    NETWORK_SERVER_CONNECTED,
    NETWORK_CLIENT
} network_mode_t;

#define NETWORK_CONTROL_KEYB (1 << 0)
#define NETWORK_CONTROL_JOY1 (1 << 1)
#define NETWORK_CONTROL_JOY2 (1 << 2)
#define NETWORK_CONTROL_DEVC (1 << 3)
#define NETWORK_CONTROL_RSRC (1 << 4)
#define NETWORK_CONTROL_CLIENTOFFSET 8

#define NETWORK_CONTROL_DEFAULT \
    ( NETWORK_CONTROL_KEYB      \
      | NETWORK_CONTROL_JOY2    \
      | NETWORK_CONTROL_RSRC    \
      | NETWORK_CONTROL_DEVC    \
      | (NETWORK_CONTROL_KEYB   \
      | NETWORK_CONTROL_JOY1)   \
        << NETWORK_CONTROL_CLIENTOFFSET)

extern int network_resources_init(void);
extern int network_cmdline_options_init(void);
extern int network_start_server(void);
extern int network_connect_client(void);
extern void network_disconnect(void);
extern void network_suspend(void);
extern void network_hook(void);
extern int network_connected(void);
extern int network_get_mode(void);
extern void network_hook(void);
extern void network_event_record(unsigned int type, void *data, unsigned int size);
extern void network_attach_image(unsigned int unit, const char *filename);

extern void network_shutdown(void);

#endif
