/*
 * tapeport.h - tape/datasette port abstraction system.
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

#ifndef VICE_TAPEPORT_H
#define VICE_TAPEPORT_H

#include "snapshot.h"
#include "types.h"

/* #define TAPEPORT_EXPERIMENTAL_DEVICES */

#define TAPEPORT_DEVICE_NONE                      0
#define TAPEPORT_DEVICE_DATASETTE                 1
#define TAPEPORT_DEVICE_CP_CLOCK_F83              2
#define TAPEPORT_DEVICE_DTL_BASIC_DONGLE          3
#define TAPEPORT_DEVICE_SENSE_DONGLE              4
#define TAPEPORT_DEVICE_TAPE_DIAG_586220_HARNESS  5
#define TAPEPORT_DEVICE_TAPECART                  6

#define TAPEPORT_MAX_DEVICES                      7

enum {
    TAPEPORT_DEVICE_TYPE_NONE = 0,
    TAPEPORT_DEVICE_TYPE_TAPE,
    TAPEPORT_DEVICE_TYPE_STORAGE,
    TAPEPORT_DEVICE_TYPE_RTC,
#ifdef TAPEPORT_EXPERIMENTAL_DEVICES
    TAPEPORT_DEVICE_TYPE_HARNESS,
#endif
    TAPEPORT_DEVICE_TYPE_DONGLE
};

enum {
    TAPEPORT_PORT_1 = 0,
    TAPEPORT_PORT_2,
    TAPEPORT_MAX_PORTS
};

enum {
    TAPEPORT_UNIT_1 = 1,    /**< tape port 1 unit number */
    TAPEPORT_UNIT_2 = 2,    /**< tape port 2 unit number */
};

#define TAPEPORT_PORT_1_MASK   (1 << TAPEPORT_PORT_1)
#define TAPEPORT_PORT_2_MASK   (1 << TAPEPORT_PORT_2)
#define TAPEPORT_PORT_ALL_MASK (TAPEPORT_PORT_1_MASK | TAPEPORT_PORT_2_MASK)

/* This struct holds all the information about the tapeport devices */
typedef struct tapeport_device_s {
    /* Name of the device */
    char *name;

    /* device type */
    int device_type;

    /* machine mask, device only works on the machines in the mask */
    int machine_mask;

    /* port mask, device only works on the ports in the mask */
    int port_mask;

    /* enable device function */
    int (*enable)(int port, int val);

    /* hard reset the device / powerup */
    void (*powerup)(int port);

    /* shutdown device */
    void (*shutdown)(void);

    /* set motor line */
    void (*set_motor)(int port, int flag);

    /* set write line */
    void (*toggle_write_bit)(int port, int write_bit);

    /* set sense line */
    void (*set_sense_out)(int port, int sense);

    /* set read line */
    void (*set_read_out)(int port, int val);

    /* Snapshot write */
    int (*write_snapshot)(int port, struct snapshot_s *s, int write_image);

    /* Snapshot read */
    int (*read_snapshot)(int port, struct snapshot_s *s);
} tapeport_device_t;

extern int tapeport_device_register(int id, tapeport_device_t *device);

extern void tapeport_set_motor(int port, int flag);
extern void tapeport_toggle_write_bit(int port, int write_bit);
extern void tapeport_set_sense_out(int port, int sense);

extern void tapeport_powerup(void);

extern int tapeport_valid_port(int port);

extern void tapeport_trigger_flux_change(unsigned int on, int port);
extern void tapeport_set_tape_sense(int sense, int port);
extern void tapeport_set_write_in(int val, int port);
extern void tapeport_set_motor_in(int val, int port);

extern int tapeport_resources_init(int amount);
extern void tapeport_resources_shutdown(void);
extern int tapeport_cmdline_options_init(void);

extern void tapeport_enable(int val);

typedef struct tapeport_desc_s {
    char *name;
    int id;
    int device_type;
} tapeport_desc_t;

extern tapeport_desc_t *tapeport_get_valid_devices(int port, int sort);
extern const char *tapeport_get_device_type_desc(int type);

extern int tapeport_snapshot_write_module(struct snapshot_s *s, int save_image);
extern int tapeport_snapshot_read_module(struct snapshot_s *s);

#endif
