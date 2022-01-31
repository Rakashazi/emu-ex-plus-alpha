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
#define JOYPORT_ID_JOY6               -6
#define JOYPORT_ID_JOY7               -7
#define JOYPORT_ID_JOY8               -8
#define JOYPORT_ID_JOY9               -9
#define JOYPORT_ID_JOY10              -10

/* #define JOYPORT_EXPERIMENTAL_DEVICES */

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
#define JOYPORT_ID_KOALAPAD            10
#define JOYPORT_ID_LIGHTPEN_U          11
#define JOYPORT_ID_LIGHTPEN_L          12
#define JOYPORT_ID_LIGHTPEN_DATEL      13
#define JOYPORT_ID_LIGHTGUN_Y          14
#define JOYPORT_ID_LIGHTGUN_L          15
#define JOYPORT_ID_LIGHTPEN_INKWELL    16
#define JOYPORT_ID_LIGHTGUN_GUNSTICK   17
#define JOYPORT_ID_SAMPLER_2BIT        18
#define JOYPORT_ID_SAMPLER_4BIT        19
#define JOYPORT_ID_BBRTC               20
#define JOYPORT_ID_PAPERCLIP64         21
#define JOYPORT_ID_COPLIN_KEYPAD       22
#define JOYPORT_ID_CARDCO_KEYPAD       23
#define JOYPORT_ID_CX85_KEYPAD         24
#define JOYPORT_ID_RUSHWARE_KEYPAD     25
#define JOYPORT_ID_CX21_KEYPAD         26
#define JOYPORT_ID_SCRIPT64_DONGLE     27
#define JOYPORT_ID_VIZAWRITE64_DONGLE  28
#define JOYPORT_ID_WAASOFT_DONGLE      29
#define JOYPORT_ID_TRAPTHEM_SNESPAD    30
#define JOYPORT_ID_NINJA_SNESPAD       31
#define JOYPORT_ID_SPACEBALLS          32
#define JOYPORT_ID_INCEPTION           33
#define JOYPORT_ID_MULTIJOY_JOYSTICKS  34
#define JOYPORT_ID_MULTIJOY_CONTROL    35
#define JOYPORT_ID_PROTOPAD            36
#define JOYPORT_ID_IO_SIMULATION       37
#define JOYPORT_ID_MF_JOYSTICK         38

#define JOYPORT_MAX_DEVICES            39

#define JOYPORT_RES_ID_NONE        0
#define JOYPORT_RES_ID_MOUSE       1
#define JOYPORT_RES_ID_SAMPLER     2
#define JOYPORT_RES_ID_KEYPAD      3

#define JOYPORT_1   0   /**< c64/c128/c64dtv/scpu64/cbm5x0/plus4 control port 1,
                             vic20 control port */
#define JOYPORT_2   1   /**< c64/c128/c64dtv/scpu64/cbm5x0/plus4 control port 2 */
#define JOYPORT_3   2   /**< c64/c128/c64dtv/scpu64/cbm2/pet/plus4/vic20 userport
                             joystick adapter port 1 */
#define JOYPORT_4   3   /**< c64/c128/scpu64/cbm2/pet/plus4/vic20 userport
                             joystick adapter port 2 */
#define JOYPORT_5   4   /**< c64/c128/scpu64/cbm2/pet/plus4/vic20 userport
                             joystick adapter port 3 */
#define JOYPORT_6   5   /**< plus4 sidcart control port */
#define JOYPORT_7   6
#define JOYPORT_8   7
#define JOYPORT_9   8
#define JOYPORT_10  9

#define JOYPORT_PLUS4_SIDCART JOYPORT_6

#define JOYPORT_MAX_PORTS     10

#define JOYPORT_IS_NOT_LIGHTPEN   0
#define JOYPORT_IS_LIGHTPEN       1

#define JOYPORT_POT_REQUIRED   0
#define JOYPORT_POT_OPTIONAL   1

#define JOYSTICK_ADAPTER_ID_NONE                  0
#define JOYSTICK_ADAPTER_ID_GENERIC_USERPORT      1
#define JOYSTICK_ADAPTER_ID_NINJA_SNES            2
#define JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES 3
#define JOYSTICK_ADAPTER_ID_USERPORT_SUPERPAD64   4
#define JOYSTICK_ADAPTER_ID_SPACEBALLS            5
#define JOYSTICK_ADAPTER_ID_MULTIJOY              6
#define JOYSTICK_ADAPTER_ID_INCEPTION             7

#define JOYPORT_DEVICE_TYPE_NONE          0
#define JOYPORT_DEVICE_JOYSTICK           1
#define JOYPORT_DEVICE_JOYSTICK_ADAPTER   2
#define JOYPORT_DEVICE_SNES_ADAPTER       3
#define JOYPORT_DEVICE_PADDLES            4
#define JOYPORT_DEVICE_MOUSE              5
#define JOYPORT_DEVICE_LIGHTPEN           6
#define JOYPORT_DEVICE_LIGHTGUN           7
#define JOYPORT_DEVICE_DRAWING_PAD        8
#define JOYPORT_DEVICE_KEYPAD             9
#define JOYPORT_DEVICE_SAMPLER            10
#define JOYPORT_DEVICE_RTC                11
#define JOYPORT_DEVICE_C64_DONGLE         12
#define JOYPORT_DEVICE_IO_SIMULATION      13

/* joystick bits */
#define JOYPORT_P0_BIT    0
#define JOYPORT_P1_BIT    1
#define JOYPORT_P2_BIT    2
#define JOYPORT_P3_BIT    3
#define JOYPORT_P4_BIT    4
#define JOYPORT_P5_BIT    5
#define JOYPORT_P6_BIT    6
#define JOYPORT_P7_BIT    7
#define JOYPORT_P8_BIT    8
#define JOYPORT_P9_BIT    9
#define JOYPORT_P10_BIT   10
#define JOYPORT_P11_BIT   11

#define JOYPORT_MAX_PINS  12
#define JOYPORT_MAX_POTS  4

/* joystick bit values */
#define JOYPORT_P0   (1 << JOYPORT_P0_BIT)
#define JOYPORT_P1   (1 << JOYPORT_P1_BIT)
#define JOYPORT_P2   (1 << JOYPORT_P2_BIT)
#define JOYPORT_P3   (1 << JOYPORT_P3_BIT)
#define JOYPORT_P4   (1 << JOYPORT_P4_BIT)
#define JOYPORT_P5   (1 << JOYPORT_P5_BIT)
#define JOYPORT_P6   (1 << JOYPORT_P6_BIT)
#define JOYPORT_P7   (1 << JOYPORT_P7_BIT)
#define JOYPORT_P8   (1 << JOYPORT_P8_BIT)
#define JOYPORT_P9   (1 << JOYPORT_P9_BIT)
#define JOYPORT_P10   (1 << JOYPORT_P10_BIT)
#define JOYPORT_P11   (1 << JOYPORT_P11_BIT)

/* descriptive bit alternatives */
#define JOYPORT_UP_BIT       JOYPORT_P0_BIT
#define JOYPORT_DOWN_BIT     JOYPORT_P1_BIT
#define JOYPORT_LEFT_BIT     JOYPORT_P2_BIT
#define JOYPORT_RIGHT_BIT    JOYPORT_P3_BIT
#define JOYPORT_FIRE_1_BIT   JOYPORT_P4_BIT
#define JOYPORT_FIRE_2_BIT   JOYPORT_P5_BIT
#define JOYPORT_FIRE_3_BIT   JOYPORT_P6_BIT
#define JOYPORT_FIRE_4_BIT   JOYPORT_P7_BIT
#define JOYPORT_FIRE_5_BIT   JOYPORT_P8_BIT
#define JOYPORT_FIRE_6_BIT   JOYPORT_P9_BIT
#define JOYPORT_FIRE_7_BIT   JOYPORT_P10_BIT
#define JOYPORT_FIRE_8_BIT   JOYPORT_P11_BIT

/* descriptive bit values alternatives */
#define JOYPORT_UP       JOYPORT_P0
#define JOYPORT_DOWN     JOYPORT_P1
#define JOYPORT_LEFT     JOYPORT_P2
#define JOYPORT_RIGHT    JOYPORT_P3
#define JOYPORT_FIRE_1   JOYPORT_P4
#define JOYPORT_FIRE_2   JOYPORT_P5
#define JOYPORT_FIRE_3   JOYPORT_P6
#define JOYPORT_FIRE_4   JOYPORT_P7
#define JOYPORT_FIRE_5   JOYPORT_P8
#define JOYPORT_FIRE_6   JOYPORT_P9
#define JOYPORT_FIRE_7   JOYPORT_P10
#define JOYPORT_FIRE_8   JOYPORT_P11

/* Alternative bit names as used for 3 button atari style joysticks */
#define JOYPORT_FIRE_BIT        JOYPORT_FIRE_1_BIT
#define JOYPORT_FIRE_POTX_BIT   JOYPORT_FIRE_2_BIT
#define JOYPORT_FIRE_POTY_BIT   JOYPORT_FIRE_3_BIT

/* Alternative bit value names as used for 3 button atari style joysticks */
#define JOYPORT_FIRE        JOYPORT_FIRE_1
#define JOYPORT_FIRE_POTX   JOYPORT_FIRE_2
#define JOYPORT_FIRE_POTY   JOYPORT_FIRE_3

/* Alternative bit names as used for snes pads */
#define JOYPORT_BUTTON_A_BIT             JOYPORT_FIRE_1_BIT
#define JOYPORT_BUTTON_B_BIT             JOYPORT_FIRE_2_BIT
#define JOYPORT_BUTTON_X_BIT             JOYPORT_FIRE_3_BIT
#define JOYPORT_BUTTON_Y_BIT             JOYPORT_FIRE_4_BIT
#define JOYPORT_BUTTON_LEFT_BUMBER_BIT   JOYPORT_FIRE_5_BIT
#define JOYPORT_BUTTON_RIGHT_BUMBER_BIT  JOYPORT_FIRE_6_BIT
#define JOYPORT_BUTTON_SELECT_BIT        JOYPORT_FIRE_7_BIT
#define JOYPORT_BUTTON_START_BIT         JOYPORT_FIRE_8_BIT

/* Alternative bit value names as used for snes pads */
#define JOYPORT_BUTTON_A             JOYPORT_FIRE_1
#define JOYPORT_BUTTON_B             JOYPORT_FIRE_2
#define JOYPORT_BUTTON_X             JOYPORT_FIRE_3
#define JOYPORT_BUTTON_Y             JOYPORT_FIRE_4
#define JOYPORT_BUTTON_LEFT_BUMBER   JOYPORT_FIRE_5
#define JOYPORT_BUTTON_RIGHT_BUMBER  JOYPORT_FIRE_6
#define JOYPORT_BUTTON_SELECT        JOYPORT_FIRE_7
#define JOYPORT_BUTTON_START         JOYPORT_FIRE_8

#define JOYPORT_BIT_BOOL(var, pos) ((var & (1 << pos)) ? 1 : 0)

#define JOYPORT_BIT_SHIFT(var, from, to) ((var & (1 << from)) ? (1 << to) : 0)

/* this structure is used for control port devices */
typedef struct joyport_s {
    char *name;                                            /* name of the device */
    int resource_id;                                       /* type of device, to determine if there can be multiple instances of the type of device */
    int is_lp;                                             /* flag to indicate the device is a lightpen */
    int pot_optional;                                      /* flag to indicate that the device can work without a potentiometer */
    int joystick_adapter_id;                               /* flag to indicate that the device is a joystick/pad adapter */
    int device_type;                                       /* device type */
    uint8_t output_bits;                                   /* flag to indicate which bits are output */
    int (*enable)(int port, int val);                      /* pointer to the device enable function */
    uint8_t (*read_digital)(int port);                     /* pointer to the device digital lines read function */
    void (*store_digital)(int port, uint8_t val);          /* pointer to the device digital lines store function */
    uint8_t (*read_potx)(int port);                        /* pointer to the device X potentiometer read function */
    uint8_t (*read_poty)(int port);                        /* pointer to the device Y potentiometer read function */
    void (*powerup)(int port);                             /* pointer to the device powerup function, called on hard reset */
    int (*write_snapshot)(struct snapshot_s *s, int port); /* pointer to the device snapshot write function */
    int (*read_snapshot)(struct snapshot_s *s, int port);  /* pointer to the device snapshot read function */
    void (*hook)(int port, uint16_t state);                /* pointer to the device hook function for state changing buttons */
    uint16_t hook_mask;                                    /* mask used for the device hook */
} joyport_t;

typedef struct joyport_desc_s {
    char *name;
    int id;
    int device_type;
} joyport_desc_t;

/* this structure is used for control ports */
typedef struct joyport_port_props_s {
    char *name;              /* name of the port */
    int has_pot;             /* flag to indicate that the port has potentiometer support */
    int has_lp_support;      /* flag to indicate that the port has lightpen support */
    int has_adapter_support; /* flag to indicate that the port can handle joystick adapters */
    int has_output_support;  /* flag to indicate that the port has output support */
    int active;              /* flag to indicate if the port is currently active */
} joyport_port_props_t;

extern int joyport_port_has_pot(int port);

/* this structure is used for host joystick to emulated input device mappings */
typedef struct joyport_mapping_s {
    char *name;   /* name of the device on the port */
    char *pin0;   /* name for the mapping of pin 0 (UP) */
    char *pin1;   /* name for the mapping of pin 1 (DOWN) */
    char *pin2;   /* name for the mapping of pin 2 (LEFT) */
    char *pin3;   /* name for the mapping of pin 3 (RIGHT) */
    char *pin4;   /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    char *pin5;   /* name for the mapping of pin 5 (FIRE-2/SNES-B) */
    char *pin6;   /* name for the mapping of pin 6 (FIRE-3/SNES-X) */
    char *pin7;   /* name for the mapping of pin 7 (SNES-Y) */
    char *pin8;   /* name for the mapping of pin 8 (SNES-LB) */
    char *pin9;   /* name for the mapping of pin 9 (SNES-RB) */
    char *pin10;  /* name for the mapping of pin 10 (SNES-SELECT) */
    char *pin11;  /* name for the mapping of pin 11 (SNES-START) */
    char *pot1;   /* name for the mapping of pot 1 (POT-X) */
    char *pot2;   /* name for the mapping of pot 2 (POT-Y) */
} joyport_mapping_t;

extern void joyport_set_mapping(joyport_mapping_t *mapping, int port);
extern void joyport_clear_mapping(int port);

typedef struct joyport_map_s {
    char *name;   /* name of the pin/pot */
    int pin;      /* pin/pot number */
} joyport_map_t;

typedef struct joyport_map_desc_s {
    char *name;              /* name of the device */
    joyport_map_t *pinmap;   /* mapping of the pins */
    joyport_map_t *potmap;   /* mapping of the pots */
} joyport_map_desc_t;

extern int joyport_has_mapping(int port);
extern joyport_map_desc_t *joyport_get_mapping(int port);

extern int joyport_device_register(int id, joyport_t *device);

extern uint8_t read_joyport_dig(int port);
extern void store_joyport_dig(int port, uint8_t val, uint8_t mask);
extern uint8_t read_joyport_potx(void);
extern uint8_t read_joyport_poty(void);

extern void set_joyport_pot_mask(int mask);

extern void joyport_powerup(void);

extern int joyport_resources_init(void);
extern int joyport_cmdline_options_init(void);

extern int joyport_port_register(int port, joyport_port_props_t *props);

extern joyport_desc_t *joyport_get_valid_devices(int port, int sort);

extern void joyport_display_joyport(int id, uint16_t status);

extern char *joyport_get_port_name(int port);

extern void joyport_clear_devices(void);

extern int joyport_port_is_active(int port);

extern void joyport_handle_joystick_hook(int port, uint16_t state);

extern char *joystick_adapter_get_name(void);
extern uint8_t joystick_adapter_get_id(void);
extern uint8_t joystick_adapter_activate(uint8_t id, char *name);
extern void joystick_adapter_deactivate(void);
extern int joystick_adapter_is_snes(void);

extern void joystick_adapter_set_ports(int ports);
extern int joystick_adapter_get_ports(void);
extern void joystick_adapter_set_add_ports(int ports);
extern void joystick_adapter_set_output_check_function(int (*function)(int port, uint8_t bits));

extern void joystick_set_hook(int port, int val, uint16_t mask);

extern int joyport_snapshot_write_module(struct snapshot_s *s, int port);
extern int joyport_snapshot_read_module(struct snapshot_s *s, int port);

#endif
