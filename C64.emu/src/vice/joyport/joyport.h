/*
 * joyport.h - control port abstraction system.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_JOYPORT_H
#define VICE_JOYPORT_H

#include "snapshot.h"
#include "types.h"

#define JOYPORT_ID_JOY1               -1
#define JOYPORT_ID_JOY2               -2
#define JOYPORT_ID_JOY3               -3
#define JOYPORT_ID_JOY4               -4
#define JOYPORT_ID_JOY5               -5

#define JOYPORT_ID_NONE                0
#define JOYPORT_ID_JOYSTICK            1
#define JOYPORT_ID_PADDLES             2
#define JOYPORT_ID_MOUSE_1351          3
#define JOYPORT_ID_MOUSE_NEOS          4
#define JOYPORT_ID_MOUSE_AMIGA         5
#define JOYPORT_ID_MOUSE_CX22          6
#define JOYPORT_ID_MOUSE_ST            7
#define JOYPORT_ID_MOUSE_SMART         8
#define JOYPORT_ID_MOUSE_MICROMYS      9
#define JOYPORT_ID_KOALAPAD           10
#define JOYPORT_ID_LIGHTPEN_U         11
#define JOYPORT_ID_LIGHTPEN_L         12
#define JOYPORT_ID_LIGHTPEN_DATEL     13
#define JOYPORT_ID_LIGHTGUN_Y         14
#define JOYPORT_ID_LIGHTGUN_L         15
#define JOYPORT_ID_LIGHTPEN_INKWELL   16
#define JOYPORT_ID_SAMPLER_2BIT       17
#define JOYPORT_ID_SAMPLER_4BIT       18
#define JOYPORT_ID_BBRTC              19
#define JOYPORT_ID_PAPERCLIP64        20
#define JOYPORT_ID_COPLIN_KEYPAD      21
#define JOYPORT_ID_CARDCO_KEYPAD      22
#define JOYPORT_ID_CX85_KEYPAD        23
#define JOYPORT_ID_RUSHWARE_KEYPAD    24
#define JOYPORT_ID_CX21_KEYPAD        25
#define JOYPORT_ID_SCRIPT64_DONGLE    26
#define JOYPORT_ID_VIZAWRITE64_DONGLE 27

#define JOYPORT_MAX_DEVICES           28

#define JOYPORT_RES_ID_NONE        0
#define JOYPORT_RES_ID_MOUSE       1
#define JOYPORT_RES_ID_SAMPLER     2
#define JOYPORT_RES_ID_KEYPAD      3
#define JOYPORT_RES_ID_RTC         4
#define JOYPORT_RES_ID_PAPERCLIP64 5
#define JOYPORT_RES_ID_SCRIPT64    6
#define JOYPORT_RES_ID_VIZAWRITE64 7

#define JOYPORT_1    0	/* c64/c128/c64dtv/cbm5x0/plus4 control port 1, vic20 control port */
#define JOYPORT_2    1	/* c64/c128/c64dtv/cbm5x0/plus4 control port 2 */
#define JOYPORT_3    2	/* c64/c128/c64dtv/cbm2/pet/plus4/vic20 userport joy adapter port 1 */
#define JOYPORT_4    3	/* c64/c128/cbm2/pet/plus4/vic20 userport joy adapter port 2 */
#define JOYPORT_5    4	/* plus4 sidcart control port */

#define JOYPORT_MAX_PORTS     5

#define JOYPORT_IS_NOT_LIGHTPEN   0
#define JOYPORT_IS_LIGHTPEN       1

#define JOYPORT_POT_REQUIRED   0
#define JOYPORT_POT_OPTIONAL   1

typedef struct joyport_s {
    char *name;
    int trans_name;
    int resource_id;
    int is_lp;
    int pot_optional;
    int (*enable)(int port, int val);
    BYTE (*read_digital)(int port);
    void (*store_digital)(BYTE val);
    BYTE (*read_potx)(void);
    BYTE (*read_poty)(void);
    int (*write_snapshot)(struct snapshot_s *s, int port);
    int (*read_snapshot)(struct snapshot_s *s, int port);
} joyport_t;

typedef struct joyport_desc_s {
    char *name;
    int trans_name;
    int id;
} joyport_desc_t;

typedef struct joyport_port_props_s {
    char *name;
    int trans_name;
    int has_pot;
    int has_lp_support;
    int active;
} joyport_port_props_t;

extern int joyport_device_register(int id, joyport_t *device);

extern BYTE read_joyport_dig(int port);
extern void store_joyport_dig(int port, BYTE val, BYTE mask);
extern BYTE read_joyport_potx(void);
extern BYTE read_joyport_poty(void);

extern void set_joyport_pot_mask(int mask);

extern int joyport_resources_init(void);
extern int joyport_cmdline_options_init(void);

extern int joyport_port_register(int port, joyport_port_props_t *props);

extern joyport_desc_t *joyport_get_valid_devices(int port);

extern void joyport_display_joyport(int id, BYTE status);

extern int joyport_get_port_trans_name(int port);
extern char *joyport_get_port_name(int port);

extern void joyport_clear_devices(void);

extern int joyport_snapshot_write_module(struct snapshot_s *s, int port);
extern int joyport_snapshot_read_module(struct snapshot_s *s, int port);

#endif
