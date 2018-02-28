/*
 * userport.h - userport abstraction system.
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

#ifndef VICE_USERPORT_H
#define VICE_USERPORT_H

#include "snapshot.h"
#include "types.h"

#define USERPORT_COLLISION_METHOD_DETACH_ALL    0
#define USERPORT_COLLISION_METHOD_DETACH_LAST   1
#define USERPORT_COLLISION_METHOD_AND_WIRES     2

#define USERPORT_DEVICE_PRINTER             0
#define USERPORT_DEVICE_JOYSTICK_CGA        1
#define USERPORT_DEVICE_JOYSTICK_PET        2
#define USERPORT_DEVICE_JOYSTICK_HUMMER     3
#define USERPORT_DEVICE_JOYSTICK_OEM        4
#define USERPORT_DEVICE_JOYSTICK_HIT        5
#define USERPORT_DEVICE_JOYSTICK_KINGSOFT   6
#define USERPORT_DEVICE_JOYSTICK_STARBYTE   7
#define USERPORT_DEVICE_DAC                 8
#define USERPORT_DEVICE_DIGIMAX             9
#define USERPORT_DEVICE_4BIT_SAMPLER        10
#define USERPORT_DEVICE_8BSS                11
#define USERPORT_DEVICE_RTC_58321A          12
#define USERPORT_DEVICE_RTC_DS1307          13
#define USERPORT_DEVICE_DIAG_586220_HARNESS 14

typedef struct userport_device_s {
    /* ID of the device */
    int id;

    /* Name of the device */
    char *name;

    /* Translated name of the device */
    int trans_name;

    /* Read pb0-7 pins */
    void (*read_pbx)(void);

    /* Store pb0-7 pins */
    void (*store_pbx)(BYTE val);

    /* Read pa2 pin */
    void (*read_pa2)(void);

    /* Store pa2 pin */
    void (*store_pa2)(BYTE val);

    /* Read pa3 pin */
    void (*read_pa3)(void);

    /* Store pa3 pin */
    void (*store_pa3)(BYTE val);

    /* Device needs pc pin */
    int needs_pc;

    /* Store sp1 pin */
    void (*store_sp1)(BYTE val);

    /* Read sp1 pin */
    void (*read_sp1)(void);

    /* Store sp2 pin */
    void (*store_sp2)(BYTE val);

    /* Read sp2 pin */
    void (*read_sp2)(void);

    /* detach device */
    char *resource;

    /* return value of a read */
    BYTE retval;

    /* validity mask of a read */
    BYTE mask;

    /* involved on collision */
    int collision;

    /* a tag to indicate the order of insertion */
    unsigned int order;
} userport_device_t;

typedef struct userport_port_props_s {
    int has_pa2;
    int has_pa3;
    void (*set_flag)(BYTE val);
    int has_pc;
    int has_sp12;
} userport_port_props_t;

typedef struct userport_device_list_s {
    struct userport_device_list_s *previous;
    userport_device_t *device;
    struct userport_device_list_s *next;
} userport_device_list_t;

extern userport_device_list_t *userport_device_register(userport_device_t *device);
extern void userport_device_unregister(userport_device_list_t *device);

extern void userport_port_register(userport_port_props_t *props);

extern BYTE read_userport_pbx(BYTE mask, BYTE orig);
extern void store_userport_pbx(BYTE val);
extern BYTE read_userport_pa2(BYTE orig);
extern void store_userport_pa2(BYTE val);
extern BYTE read_userport_pa3(BYTE orig);
extern void store_userport_pa3(BYTE val);
extern void set_userport_flag(BYTE val);
extern BYTE read_userport_pc(BYTE orig);
extern BYTE read_userport_sp1(BYTE orig);
extern void store_userport_sp1(BYTE val);
extern BYTE read_userport_sp2(BYTE orig);
extern void store_userport_sp2(BYTE val);

extern int userport_resources_init(void);
extern void userport_resources_shutdown(void);
extern int userport_cmdline_options_init(void);

extern void userport_enable(int val);

typedef struct userport_snapshot_s {
    /* ID of the device */
    int id;

    /* write snapshot */
    int (*write_snapshot)(snapshot_t *s);

    /* read snapshot */
    int (*read_snapshot)(snapshot_t *s);
} userport_snapshot_t;

typedef struct userport_snapshot_list_s {
    struct userport_snapshot_list_s *previous;
    userport_snapshot_t *snapshot;
    struct userport_snapshot_list_s *next;
} userport_snapshot_list_t;

extern void userport_snapshot_register(userport_snapshot_t *s);

extern int userport_snapshot_write_module(snapshot_t *s);
extern int userport_snapshot_read_module(snapshot_t *s);

#endif
