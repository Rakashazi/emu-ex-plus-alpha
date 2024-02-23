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

/* #define JOYPORT_EXPERIMENTAL_DEVICES */

#ifdef HAVE_EXPERIMENTAL_DEVICES
#define JOYPORT_EXPERIMENTAL_DEVICES
#endif

#define JOYPORT_ID_UNKNOWN             -1   /* used with joyport_display_joyport() */

/* IMPORTANT: Do NOT put #ifdef's in this enum,
              Do NOT change the order of the ID's,
              Add new devices at the end, before JOYPORT_MAX_DEVICES */
enum {
    JOYPORT_ID_NONE = 0,    /* CAUTION: some code relies on this being 0 */
    JOYPORT_ID_JOYSTICK,
    JOYPORT_ID_PADDLES,
    JOYPORT_ID_MOUSE_1351,
    JOYPORT_ID_MOUSE_NEOS,
    JOYPORT_ID_MOUSE_AMIGA,
    JOYPORT_ID_MOUSE_CX22,
    JOYPORT_ID_MOUSE_ST,
    JOYPORT_ID_MOUSE_SMART,
    JOYPORT_ID_MOUSE_MICROMYS,
    JOYPORT_ID_KOALAPAD,
    JOYPORT_ID_LIGHTPEN_U,
    JOYPORT_ID_LIGHTPEN_L,
    JOYPORT_ID_LIGHTPEN_DATEL,
    JOYPORT_ID_LIGHTGUN_Y,
    JOYPORT_ID_LIGHTGUN_L,
    JOYPORT_ID_LIGHTPEN_INKWELL,
    JOYPORT_ID_LIGHTGUN_GUNSTICK,
    JOYPORT_ID_SAMPLER_2BIT,
    JOYPORT_ID_SAMPLER_4BIT,
    JOYPORT_ID_BBRTC,
    JOYPORT_ID_PAPERCLIP64,
    JOYPORT_ID_COPLIN_KEYPAD,
    JOYPORT_ID_CARDCO_KEYPAD,
    JOYPORT_ID_CX85_KEYPAD,
    JOYPORT_ID_RUSHWARE_KEYPAD,
    JOYPORT_ID_CX21_KEYPAD,
    JOYPORT_ID_SCRIPT64_DONGLE,
    JOYPORT_ID_VIZAWRITE64_DONGLE,
    JOYPORT_ID_WAASOFT_DONGLE,
    JOYPORT_ID_TRAPTHEM_SNESPAD,
    JOYPORT_ID_NINJA_SNESPAD,
    JOYPORT_ID_SPACEBALLS,
    JOYPORT_ID_INCEPTION,
    JOYPORT_ID_MULTIJOY_JOYSTICKS,
    JOYPORT_ID_MULTIJOY_CONTROL,
    JOYPORT_ID_PROTOPAD,
    JOYPORT_ID_IO_SIMULATION,
    JOYPORT_ID_MF_JOYSTICK,
    JOYPORT_ID_PAPERCLIP64E,
    JOYPORT_ID_PAPERCLIP64SC,
    JOYPORT_ID_PAPERCLIP2,

    /* This item always needs to be at the end */
    JOYPORT_MAX_DEVICES
};

enum {
    JOYPORT_RES_ID_NONE,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_RES_ID_SAMPLER,
    JOYPORT_RES_ID_KEYPAD
};

enum {
    JOYPORT_1 = 0,   /**< c64/c128/c64dtv/scpu64/cbm5x0/plus4 control port 1, vic20 control port */
    JOYPORT_2,       /**< c64/c128/c64dtv/scpu64/cbm5x0/plus4 control port 2 */
    JOYPORT_3,       /**< c64/c128/c64dtv/scpu64/cbm2/pet/plus4/vic20 userport, joystick adapter port 1 */
    JOYPORT_4,       /**< c64/c128/scpu64/cbm2/pet/plus4/vic20 userport, joystick adapter port 2 */
    JOYPORT_5,       /**< c64/c128/scpu64/cbm2/pet/plus4/vic20 userport, joystick adapter port 3 */
    JOYPORT_6,
    JOYPORT_7,
    JOYPORT_8,
    JOYPORT_9,
    JOYPORT_10,
    JOYPORT_11,      /**< plus4 sidcart control port */

    /* This item always needs to be at the end */
    JOYPORT_MAX_PORTS
};

#define JOYPORT_PLUS4_SIDCART JOYPORT_11

#define JOYPORT_IS_NOT_LIGHTPEN   0
#define JOYPORT_IS_LIGHTPEN       1

#define JOYPORT_POT_REQUIRED   0
#define JOYPORT_POT_OPTIONAL   1

#define JOYPORT_5VDC_NOT_NEEDED   0
#define JOYPORT_5VDC_REQUIRED     1

enum {
    JOYSTICK_ADAPTER_ID_NONE = 0,
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,
    JOYSTICK_ADAPTER_ID_NINJA_SNES,
    JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES,
    JOYSTICK_ADAPTER_ID_USERPORT_SUPERPAD64,
    JOYSTICK_ADAPTER_ID_SPACEBALLS,
    JOYSTICK_ADAPTER_ID_MULTIJOY,
    JOYSTICK_ADAPTER_ID_INCEPTION
};

enum {
    JOYPORT_DEVICE_TYPE_NONE = 0,
    JOYPORT_DEVICE_JOYSTICK,
    JOYPORT_DEVICE_JOYSTICK_ADAPTER,
    JOYPORT_DEVICE_SNES_ADAPTER,
    JOYPORT_DEVICE_PADDLES,
    JOYPORT_DEVICE_MOUSE,
    JOYPORT_DEVICE_LIGHTPEN,
    JOYPORT_DEVICE_LIGHTGUN,
    JOYPORT_DEVICE_DRAWING_PAD,
    JOYPORT_DEVICE_KEYPAD,
    JOYPORT_DEVICE_SAMPLER,
    JOYPORT_DEVICE_RTC,
    JOYPORT_DEVICE_C64_DONGLE,
    JOYPORT_DEVICE_IO_SIMULATION
};

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

/* this structure is used to init all the joystick devices */
typedef struct joyport_init_s {
    int device_id;                                         /* device ID */
    int emu_mask;                                          /* which emulator does the device work on */
    int (*joyport_device_resources_init)(void);            /* device resources init function */
    void (*joyport_device_resources_shutdown)(void);       /* device resources shutdown function */
    int (*joyport_device_cmdline_options_init)(void);      /* device cmdline init function */
} joyport_init_t;

/* this structure is used for control port devices */
typedef struct joyport_s {
    char *name;                                            /* name of the device */
    int resource_id;                                       /* type of device, to determine if there can be multiple instances of the type of device */
    int is_lp;                                             /* flag to indicate the device is a lightpen */
    int pot_optional;                                      /* flag to indicate that the device can work without a potentiometer */
    int needs_5v;                                          /* flag to indicate the device needs the +5VDC line to work */
    int joystick_adapter_id;                               /* flag to indicate that the device is a joystick/pad adapter */
    int device_type;                                       /* device type */
    uint8_t output_bits;                                   /* flag to indicate which bits are output */
    int (*set_enabled)(int port, int joyport_id);          /* device enable/disable function, passes JOYPORT_ID_NONE to disable */
    uint8_t (*read_digital)(int port);                     /* device digital lines read function */
    void (*store_digital)(int port, uint8_t val);          /* device digital lines store function */
    uint8_t (*read_potx)(int port);                        /* device X potentiometer read function */
    uint8_t (*read_poty)(int port);                        /* device Y potentiometer read function */
    void (*powerup)(int port);                             /* device powerup function, called on hard reset */
    int (*write_snapshot)(struct snapshot_s *s, int port); /* device snapshot write function */
    int (*read_snapshot)(struct snapshot_s *s, int port);  /* device snapshot read function */
    void (*hook)(int port, uint16_t state);                /* device hook function for state changing buttons */
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
    int has_5vdc_support;    /* flag to indicate that the port has a +5VDC line */
    int active;              /* flag to indicate if the port is currently active */
} joyport_port_props_t;

int joyport_port_has_pot(int port);

/* this structure is used for host joystick to emulated input device mappings */
typedef struct joyport_mapping_s {
    char *name;   /* name of the device on the port */
    char *pin0;   /* name for the mapping of digital pin 0 (UP) */
    char *pin1;   /* name for the mapping of digital pin 1 (DOWN) */
    char *pin2;   /* name for the mapping of digital pin 2 (LEFT) */
    char *pin3;   /* name for the mapping of digital pin 3 (RIGHT) */
    char *pin4;   /* name for the mapping of digital pin 4 (FIRE-1/SNES-A) */
    char *pin5;   /* name for the mapping of digital pin 5 (FIRE-2/SNES-B) */
    char *pin6;   /* name for the mapping of digital pin 6 (FIRE-3/SNES-X) */
    char *pin7;   /* name for the mapping of digital pin 7 (SNES-Y) */
    char *pin8;   /* name for the mapping of digital pin 8 (SNES-LB) */
    char *pin9;   /* name for the mapping of digital pin 9 (SNES-RB) */
    char *pin10;  /* name for the mapping of digital pin 10 (SNES-SELECT) */
    char *pin11;  /* name for the mapping of digital pin 11 (SNES-START) */
    char *pot1;   /* name for the mapping of analog pot 1 (POT-X) */
    char *pot2;   /* name for the mapping of analog pot 2 (POT-Y) */
} joyport_mapping_t;

void joyport_set_mapping(joyport_mapping_t *mapping, int port);
void joyport_clear_mapping(int port);

typedef struct joyport_map_s {
    char *name;   /* name of the pin/pot */
    int pin;      /* pin/pot number */
} joyport_map_t;

typedef struct joyport_map_desc_s {
    char *name;              /* name of the device */
    joyport_map_t *pinmap;   /* mapping of the pins */
    joyport_map_t *potmap;   /* mapping of the pots */
} joyport_map_desc_t;

int joyport_has_mapping(int port);
joyport_map_desc_t *joyport_get_mapping(int port);

int joyport_device_register(int id, joyport_t *device);

uint8_t read_joyport_dig(int port);
void store_joyport_dig(int port, uint8_t val, uint8_t mask);
uint8_t read_joyport_potx(void);
uint8_t read_joyport_poty(void);

void set_joyport_pot_mask(int mask);

void joyport_powerup(void);

int joyport_resources_init(void);
void joyport_resources_shutdown(void);
int joyport_cmdline_options_init(void);

int joyport_port_register(int port, joyport_port_props_t *props);

joyport_desc_t *joyport_get_valid_devices(int port, int sort);

void joyport_display_joyport(int port, int id, uint16_t status);

char *joyport_get_port_name(int port);

void joyport_clear_devices(void);

int joyport_port_is_active(int port);

void joyport_handle_joystick_hook(int port, uint16_t state);

char *joystick_adapter_get_name(void);
uint8_t joystick_adapter_get_id(void);
uint8_t joystick_adapter_activate(uint8_t id, char *name);
void joystick_adapter_deactivate(void);
int joystick_adapter_is_snes(void);

void joystick_adapter_set_ports(int ports, int has_5vdc);
int joystick_adapter_get_ports(void);
void joystick_adapter_set_add_ports(int ports);
void joystick_adapter_set_output_check_function(int (*function)(int port, uint8_t bits));

void joystick_set_hook(int port, int val, uint16_t mask);

int joyport_snapshot_write_module(struct snapshot_s *s, int port);
int joyport_snapshot_read_module(struct snapshot_s *s, int port);

#endif
