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

/* #define USERPORT_EXPERIMENTAL_DEVICES */

#ifdef HAVE_EXPERIMENTAL_DEVICES
#define USERPORT_EXPERIMENTAL_DEVICES
#endif

#define USERPORT_NO_PULSE   0
#define USERPORT_PULSE      1

/* IMPORTANT: Do NOT put #ifdef's in this enum,
              Do NOT change the order of the ID's,
              Add new devices at the end, before JOYPORT_MAX_DEVICES */
enum {
    USERPORT_DEVICE_NONE = 0,
    USERPORT_DEVICE_PRINTER,
    USERPORT_DEVICE_RS232_MODEM,
    USERPORT_DEVICE_JOYSTICK_CGA,
    USERPORT_DEVICE_JOYSTICK_PET,
    USERPORT_DEVICE_JOYSTICK_HUMMER,
    USERPORT_DEVICE_JOYSTICK_OEM,
    USERPORT_DEVICE_JOYSTICK_HIT,
    USERPORT_DEVICE_JOYSTICK_KINGSOFT,
    USERPORT_DEVICE_JOYSTICK_STARBYTE,
    USERPORT_DEVICE_JOYSTICK_SYNERGY,
    USERPORT_DEVICE_JOYSTICK_WOJ,
    USERPORT_DEVICE_DAC,
    USERPORT_DEVICE_DIGIMAX,
    USERPORT_DEVICE_4BIT_SAMPLER,
    USERPORT_DEVICE_8BSS,
    USERPORT_DEVICE_RTC_58321A,
    USERPORT_DEVICE_RTC_DS1307,
    USERPORT_DEVICE_PETSCII_SNESPAD,
    USERPORT_DEVICE_SUPERPAD64,
    USERPORT_DEVICE_DIAG_586220_HARNESS,
    USERPORT_DEVICE_DRIVE_PAR_CABLE,
    USERPORT_DEVICE_IO_SIMULATION,
    USERPORT_DEVICE_WIC64,
    USERPORT_DEVICE_SPACEBALLS,
    USERPORT_DEVICE_SPT_JOYSTICK,

    /* This item always needs to be at the end */
    USERPORT_MAX_DEVICES
};

enum {
    USERPORT_DEVICE_TYPE_NONE = 0,
    USERPORT_DEVICE_TYPE_PRINTER,
    USERPORT_DEVICE_TYPE_MODEM,
    USERPORT_DEVICE_TYPE_DRIVE_PAR_CABLE,
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,
    USERPORT_DEVICE_TYPE_AUDIO_OUTPUT,
    USERPORT_DEVICE_TYPE_SAMPLER,
    USERPORT_DEVICE_TYPE_RTC,
#ifdef USERPORT_EXPERIMENTAL_DEVICES
    USERPORT_DEVICE_TYPE_HARNESS,
#endif
#ifdef HAVE_LIBCURL
    USERPORT_DEVICE_TYPE_WIFI,
#endif
    USERPORT_DEVICE_TYPE_IO_SIMULATION
};

/* 24 pin userport pin defines */

/* pin 1 is currently GND on all supported 24 pin userports */
#define USERPORT24_PIN_1   0x00000001

#define USERPORT24_PIN_2   0x00000002
#define USERPORT24_PIN_3   0x00000004
#define USERPORT24_PIN_4   0x00000008
#define USERPORT24_PIN_5   0x00000010
#define USERPORT24_PIN_6   0x00000020
#define USERPORT24_PIN_7   0x00000040
#define USERPORT24_PIN_8   0x00000080
#define USERPORT24_PIN_9   0x00000100
#define USERPORT24_PIN_10  0x00000200
#define USERPORT24_PIN_11  0x00000400

/* pin 12 is currently GND on all supported 24 pin userports */
#define USERPORT24_PIN_12  0x00000800

/* pin A is currently GND on all supported 24 pin userports */
#define USERPORT24_PIN_A   0x00001000

#define USERPORT24_PIN_B   0x00002000
#define USERPORT24_PIN_C   0x00004000
#define USERPORT24_PIN_D   0x00008000
#define USERPORT24_PIN_E   0x00010000
#define USERPORT24_PIN_F   0x00020000
#define USERPORT24_PIN_H   0x00040000
#define USERPORT24_PIN_J   0x00080000
#define USERPORT24_PIN_K   0x00100000
#define USERPORT24_PIN_L   0x00200000
#define USERPORT24_PIN_M   0x00400000

/* pin N is currently GND on all supported 24 pin userports */
#define USERPORT24_PIN_N   0x00800000


/* xcbm2 pins */

/* pin 1 is GND on cbm2 */
#define USERPORT26_PIN_1   0x00000001

#define USERPORT26_PIN_2   0x00000002

/* pin 3 is GND on cbm2 */
#define USERPORT26_PIN_3   0x00000004

#define USERPORT26_PIN_4   0x00000008
#define USERPORT26_PIN_5   0x00000010
#define USERPORT26_PIN_6   0x00000020
#define USERPORT26_PIN_7   0x00000040
#define USERPORT26_PIN_8   0x00000080
#define USERPORT26_PIN_9   0x00000100
#define USERPORT26_PIN_10  0x00000200
#define USERPORT26_PIN_11  0x00000400
#define USERPORT26_PIN_12  0x00000800
#define USERPORT26_PIN_13  0x00001000
#define USERPORT26_PIN_14  0x00002000
#define USERPORT26_PIN_15  0x00004000
#define USERPORT26_PIN_16  0x00008000
#define USERPORT26_PIN_17  0x00010000
#define USERPORT26_PIN_18  0x00020000
#define USERPORT26_PIN_19  0x00040000
#define USERPORT26_PIN_20  0x00080000
#define USERPORT26_PIN_21  0x00100000
#define USERPORT26_PIN_22  0x00200000
#define USERPORT26_PIN_23  0x00400000

/* pin 24 is +5VDC on cbm2 */
#define USERPORT26_PIN_24  0x00800000

#define USERPORT26_PIN_25  0x01000000
#define USERPORT26_PIN_26  0x02000000

/* x64/x64sc/x128/xscpu64/xvic/xpet/xpet data pins */
#define USERPORT24_PIN_D0   USERPORT24_PIN_C
#define USERPORT24_PIN_D1   USERPORT24_PIN_D
#define USERPORT24_PIN_D2   USERPORT24_PIN_E
#define USERPORT24_PIN_D3   USERPORT24_PIN_F
#define USERPORT24_PIN_D4   USERPORT24_PIN_H
#define USERPORT24_PIN_D5   USERPORT24_PIN_J
#define USERPORT24_PIN_D6   USERPORT24_PIN_K
#define USERPORT24_PIN_D7   USERPORT24_PIN_L

/* xplus4 data pins */
#define USERPORT24_PLUS4_PIN_P0   USERPORT24_PIN_B
#define USERPORT24_PLUS4_PIN_P1   USERPORT24_PIN_K
#define USERPORT24_PLUS4_PIN_P2   USERPORT24_PIN_4
#define USERPORT24_PLUS4_PIN_P3   USERPORT24_PIN_5
#define USERPORT24_PLUS4_PIN_P4   USERPORT24_PIN_6
#define USERPORT24_PLUS4_PIN_P5   USERPORT24_PIN_7
#define USERPORT24_PLUS4_PIN_P6   USERPORT24_PIN_J
#define USERPORT24_PLUS4_PIN_P7   USERPORT24_PIN_F

/* x64/x64sc/xscpu64/x128 specific pins, you can use these pin names when a device only works on the machines in question. */
#define USERPORT24_C64_PIN_CNT1         USERPORT24_PIN_4
#define USERPORT24_C64_PIN_SP1          USERPORT24_PIN_5
#define USERPORT24_C64_PIN_CNT2         USERPORT24_PIN_6
#define USERPORT24_C64_PIN_SP2          USERPORT24_PIN_7
#define USERPORT24_C64_PIN_PC2          USERPORT24_PIN_8
#define USERPORT24_C64_PIN_IEC_ATN_IN   USERPORT24_PIN_9
#define USERPORT24_C64_PIN_NFLAG2       USERPORT24_PIN_B
#define USERPORT24_C64_PIN_PA2          USERPORT24_PIN_M

/* x64dtv specific pins */
#define USERPORT24_C64DTV_PIN_PA2   USERPORT24_PIN_M

/* xvic specific pins */
#define USERPORT24_VIC20_PIN_JOY0             USERPORT24_PIN_4
#define USERPORT24_VIC20_PIN_JOY1             USERPORT24_PIN_5
#define USERPORT24_VIC20_PIN_JOY2             USERPORT24_PIN_6
#define USERPORT24_VIC20_PIN_LP_FIRE          USERPORT24_PIN_7
#define USERPORT24_VIC20_PIN_PA6_TAPE_SENSE   USERPORT24_PIN_8
#define USERPORT24_VIC20_PIN_IEC_ATN_IN       USERPORT24_PIN_9
#define USERPORT24_VIC20_PIN_CB1              USERPORT24_PIN_B
#define USERPORT24_VIC20_PIN_CB2              USERPORT24_PIN_M

/* xplus4 specific pins */
#define USERPORT24_PLUS4_PIN_RS232_CLOCK   USERPORT24_PIN_8
#define USERPORT24_PLUS4_PIN_ATN           USERPORT24_PIN_9
#define USERPORT24_PLUS4_PIN_ACIA_RXD      USERPORT24_PIN_C
#define USERPORT24_PLUS4_PIN_ACIA_RTS      USERPORT24_PIN_D
#define USERPORT24_PLUS4_PIN_ACIA_DTR      USERPORT24_PIN_E
#define USERPORT24_PLUS4_PIN_ACIA_DCD      USERPORT24_PIN_H
#define USERPORT24_PLUS4_PIN_ACIA_DSR      USERPORT24_PIN_L
#define USERPORT24_PLUS4_PIN_ACIA_TXD      USERPORT24_PIN_M

/* xpet specific pins */
#define USERPORT24_PET_PIN_TV_VIDEO          USERPORT24_PIN_2
#define USERPORT24_PET_PIN_IEEE_SRQ          USERPORT24_PIN_3
#define USERPORT24_PET_PIN_IEEE_EOI          USERPORT24_PIN_4
#define USERPORT24_PET_PIN_IEEE_DIAG_SENSE   USERPORT24_PIN_5
#define USERPORT24_PET_PIN_TAPE2_READ        USERPORT24_PIN_6
#define USERPORT24_PET_PIN_TAPE_WRITE        USERPORT24_PIN_7
#define USERPORT24_PET_PIN_TAPE1_READ        USERPORT24_PIN_8
#define USERPORT24_PET_PIN_TV_VERTICAL       USERPORT24_PIN_9
#define USERPORT24_PET_PIN_TV_HORIZONTAL     USERPORT24_PIN_10
#define USERPORT24_PET_PIN_GRAPHIC           USERPORT24_PIN_11
#define USERPORT24_PET_PIN_CA1               USERPORT24_PIN_B
#define USERPORT24_PET_PIN_CB2               USERPORT24_PIN_M

/* xcbm2 specific pins */
#define USERPORT26_CBM2_PIN_PB2     USERPORT26_PIN_2
#define USERPORT26_CBM2_PIN_PB3     USERPORT26_PIN_4
#define USERPORT26_CBM2_PIN_NPC     USERPORT26_PIN_5
#define USERPORT26_CBM2_PIN_NFLAG   USERPORT26_PIN_6
#define USERPORT26_CBM2_PIN_2D7     USERPORT26_PIN_7
#define USERPORT26_CBM2_PIN_2D6     USERPORT26_PIN_8
#define USERPORT26_CBM2_PIN_2D5     USERPORT26_PIN_9
#define USERPORT26_CBM2_PIN_2D4     USERPORT26_PIN_10
#define USERPORT26_CBM2_PIN_2D3     USERPORT26_PIN_11
#define USERPORT26_CBM2_PIN_2D2     USERPORT26_PIN_12
#define USERPORT26_CBM2_PIN_2D1     USERPORT26_PIN_13
#define USERPORT26_CBM2_PIN_2D0     USERPORT26_PIN_14
#define USERPORT26_CBM2_PIN_1D7     USERPORT26_PIN_15
#define USERPORT26_CBM2_PIN_1D6     USERPORT26_PIN_16
#define USERPORT26_CBM2_PIN_1D5     USERPORT26_PIN_17
#define USERPORT26_CBM2_PIN_1D4     USERPORT26_PIN_18
#define USERPORT26_CBM2_PIN_1D3     USERPORT26_PIN_19
#define USERPORT26_CBM2_PIN_1D2     USERPORT26_PIN_20
#define USERPORT26_CBM2_PIN_1D1     USERPORT26_PIN_21
#define USERPORT26_CBM2_PIN_1D0     USERPORT26_PIN_22
#define USERPORT26_CBM2_PIN_NCNT    USERPORT26_PIN_23
#define USERPORT26_CBM2_PIN_NIRQ    USERPORT26_PIN_25
#define USERPORT26_CBM2_PIN_SP      USERPORT26_PIN_26

/* this structure is used by userport devices */
typedef struct userport_device_s {
    /* Name of the device */
    char *name;

    /* flag to indicate that the device is a joystick/pad adapter */
    int joystick_adapter_id;

    /* flag to indicate the device type */
    int device_type;

    /* Device enable/disable */
    int (*enable)(int val);

    /* Read pb0-7 pins */
    uint8_t (*read_pbx)(uint8_t orig);

    /* Store pb0-7 pins */
    void (*store_pbx)(uint8_t val, int pulse);

    /* Read pa2 pin */
    uint8_t (*read_pa2)(uint8_t orig);

    /* Store pa2 pin */
    void (*store_pa2)(uint8_t val);

    /* Read pa3 pin */
    uint8_t (*read_pa3)(uint8_t orig);

    /* Store pa3 pin */
    void (*store_pa3)(uint8_t val);

    /* Device needs pc pin */
    int needs_pc;

    /* Store sp1 pin */
    void (*store_sp1)(uint8_t val);

    /* Read sp1 pin */
    uint8_t (*read_sp1)(uint8_t orig);

    /* Store sp2 pin */
    void (*store_sp2)(uint8_t val);

    /* Read sp2 pin */
    uint8_t (*read_sp2)(uint8_t orig);

    /* device reset function, reset line on the userport */
    void (*reset)(void);

    /* device powerup function, gets called when a hard reset is done */
    void (*powerup)(void);

    /* Snapshot write */
    int (*write_snapshot)(struct snapshot_s *s);

    /* Snapshot read */
    int (*read_snapshot)(struct snapshot_s *s);  /* pointer to the device snapshot read function */
} userport_device_t;

/* this structure is used by userport ports */
typedef struct userport_port_props_s {
    int has_pa2;                   /* port has the pa2 line */
    int has_pa3;                   /* port has the pa3 line */
    void (*set_flag)(uint8_t val); /* pointer to set flag function */
    int has_pc;                    /* port has the pc line */
    int has_sp12;                  /* port has the sp1 and sp2 lines */
    int has_reset;                 /* port had the reset line */
} userport_port_props_t;

typedef struct userport_desc_s {
    char *name;
    int id;
    int device_type;
} userport_desc_t;

void userport_port_register(userport_port_props_t *props);
int userport_device_register(int id, userport_device_t *device);

uint8_t read_userport_pbx(uint8_t orig);
void store_userport_pbx(uint8_t val, int pulse);
uint8_t read_userport_pa2(uint8_t orig);
void store_userport_pa2(uint8_t val);
uint8_t read_userport_pa3(uint8_t orig);
void store_userport_pa3(uint8_t val);
uint8_t read_userport_pc(uint8_t orig);
uint8_t read_userport_sp1(uint8_t orig);
void store_userport_sp1(uint8_t val);
uint8_t read_userport_sp2(uint8_t orig);
void store_userport_sp2(uint8_t val);
void userport_reset_start(void);
void userport_reset(void);
void userport_reset_end(void);
void userport_powerup(void);

/* use this function from userport device code to set the userport flag */
void set_userport_flag(uint8_t val);

int userport_resources_init(void);
int userport_cmdline_options_init(void);

userport_desc_t *userport_get_valid_devices(int sort);
const char *userport_get_device_type_desc(int type);

void userport_enable(int val);
int userport_get_active_state(void);

int userport_snapshot_write_module(snapshot_t *s);
int userport_snapshot_read_module(snapshot_t *s);

#endif
