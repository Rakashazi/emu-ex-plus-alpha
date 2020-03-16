/*
 * c64.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>

#include "attach.h"
#include "autostart.h"
#include "bbrtc.h"
#include "c64-cmdline-options.h"
#include "c64-memory-hacks.h"
#include "c64-resources.h"
#include "c64-snapshot.h"
#include "c64.h"
#include "c64_256k.h"
#include "c64cart.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cia.h"
#include "c64fastiec.h"
#include "c64gluelogic.h"
#include "c64iec.h"
#include "c64keyboard.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rsuser.h"
#include "cardkey.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "clkguard.h"
#include "clockport-mp3at64.h"
#include "coplin_keypad.h"
#include "cx21.h"
#include "cx85.h"
#include "datasette.h"
#include "debug.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "export.h"
#include "fliplist.h"
#include "fmopl.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "imagecontents.h"
#include "init.h"
#include "joyport.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "log.h"
#include "machine-drive.h"
#include "machine-printer.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "network.h"
#include "paperclip64.h"
#include "parallel.h"
#include "patchrom.h"
#include "plus256k.h"
#include "plus60k.h"
#include "printer.h"
#include "psid.h"
#include "resources.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "rushware_keypad.h"
#include "sampler.h"
#include "sampler2bit.h"
#include "sampler4bit.h"
#include "screenshot.h"
#include "script64_dongle.h"
#include "serial.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "snespad.h"
#include "sound.h"
#include "tape.h"
#include "tape_diag_586220_harness.h"
#include "tapeport.h"
#include "tapecart.h"
#include "traps.h"
#include "types.h"
#include "userport.h"
#include "userport_4bit_sampler.h"
#include "userport_8bss.h"
#include "userport_dac.h"
#include "userport_diag_586220_harness.h"
#include "userport_digimax.h"
#include "userport_joystick.h"
#include "userport_rtc_58321a.h"
#include "userport_rtc_ds1307.h"
#include "vice-event.h"
#include "vicii.h"
#include "vicii-mem.h"
#include "video.h"
#include "video-sound.h"
#include "vizawrite64_dongle.h"
#include "vsync.h"
#include "waasoft_dongle.h"

#ifdef HAVE_MOUSE
#include "lightpen.h"
#include "mouse.h"
#endif


/** \brief  Delay in seconds before pasting -keybuf argument into the buffer
 */
#define KBDBUF_ALARM_DELAY   1


machine_context_t machine_context;

const char machine_name[] = "C64";
/* Moved to c64mem.c/c64memsc.c
int machine_class = VICE_MACHINE_C64;
*/
static void machine_vsync_hook(void);

int machine_get_keyboard_type(void)
{
#if 0
    int type;
    if (resources_get_int("KeyboardType", &type) < 0) {
        return 0;
    }
    return type;
#endif
    return 0;
}

char *machine_get_keyboard_type_name(int type)
{
    return NULL; /* return 0 if no different types exist */
}

/* return number of available keyboard types for this machine */
int machine_get_num_keyboard_types(void)
{
    return 1;
}

kbdtype_info_t *machine_get_keyboard_info_list(void)
{
    return NULL; /* return 0 if no different types exist */
}

/* ------------------------------------------------------------------------- */

static const trap_t c64_serial_traps[] = {
    { "SerialListen", 0xED24, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_attention, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialSaListen", 0xED37, 0xEDAB, { 0x20, 0x8E, 0xEE }, serial_trap_attention, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialSendByte", 0xED41, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_send, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialReceiveByte", 0xEE14, 0xEDAB, { 0xA9, 0x00, 0x85 }, serial_trap_receive, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialReady", 0xEEA9, 0xEDAB, { 0xAD, 0x00, 0xDD }, serial_trap_ready, c64memrom_trap_read, c64memrom_trap_store },
    { NULL, 0, 0, { 0, 0, 0 }, NULL, NULL, NULL }
};

/* Tape traps.  */
static const trap_t c64_tape_traps[] = {
    { "TapeFindHeader", 0xF72F, 0xF732, { 0x20, 0x41, 0xF8 }, tape_find_header_trap, c64memrom_trap_read, c64memrom_trap_store },
    { "TapeReceive", 0xF8A1, 0xFC93, { 0x20, 0xBD, 0xFC }, tape_receive_trap, c64memrom_trap_read, c64memrom_trap_store },
    { NULL, 0, 0, { 0, 0, 0 }, NULL, NULL, NULL }
};

static const tape_init_t tapeinit = {
    0xb2,
    0x90,
    0x93,
    0x29f,
    0,
    0xc1,
    0xae,
    0x277,
    0xc6,
    c64_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static log_t c64_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

/* The following I/O range is only used when +60K or +256K memory hacks are not active.
   The +60K or +256K memory hacks unregister this range and use their own replacement.
 */
static io_source_t vicii_d000_device = {
    "VIC-II",              /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xd000, 0xd0ff, 0x3f,  /* regs: $d000-d03f, mirrors: $d040-$d0ff */
    1,                     /* read is always valid */
    vicii_store,           /* store function */
    NULL,                  /* NO poke function */
    vicii_read,            /* read function */
    vicii_peek,            /* peek function */
    vicii_dump,            /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip and mirrors never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

/* The following I/O range is only used when +60K or +256K memory hacks are not active.
   The +60K or +256K memory hacks unregister this range and use their own replacement.
 */
static io_source_t vicii_d100_device = {
    "VIC-II $D100-$D1FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,              /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xd100, 0xd1ff, 0x3f,         /* mirrors of $d000-$d03f */
    1,                            /* read is always valid */
    vicii_store,                  /* store function */
    NULL,                         /* NO poke function */
    vicii_read,                   /* read function */
    vicii_peek,                   /* peek function */
    vicii_dump,                   /* chip state information dump function */ 
    IO_CART_ID_NONE,              /* not a cartridge */
    IO_PRIO_HIGH,                 /* high priority, mirrors never involved in collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

/* The following I/O range is only used when +60K or +256K memory hacks are not active.
   The +60K or +256K memory hacks unregister this range and leave the I/O range 'unconnected'.
 */
static io_source_t vicii_d200_device = {
    "VIC-II $D200-$D2FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,              /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xd200, 0xd2ff, 0x3f,         /* mirrors of $d000-$d03f */
    1,                            /* read is always valid */
    vicii_store,                  /* store function */
    NULL,                         /* NO poke function */
    vicii_read,                   /* read function */
    vicii_peek,                   /* peek function */
    vicii_dump,                   /* chip state information dump function */ 
    IO_CART_ID_NONE,              /* not a cartridge */
    IO_PRIO_HIGH,                 /* high priority, mirrors never involved in collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

/* The following I/O range is only used when +60K or +256K memory hacks are not active.
   The +60K or +256K memory hacks unregister this range and leave the I/O range 'unconnected'.
 */
static io_source_t vicii_d300_device = {
    "VIC-II $D300-$D3FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,              /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xd300, 0xd3ff, 0x3f,         /* mirrors of $d000-$d03f */
    1,                            /* read is always valid */
    vicii_store,                  /* store function */
    NULL,                         /* NO poke function */
    vicii_read,                   /* read function */
    vicii_peek,                   /* peek function */
    vicii_dump,                   /* chip state information dump function */ 
    IO_CART_ID_NONE,              /* not a cartridge */
    IO_PRIO_HIGH,                 /* high priority, mirrors never involved in collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_d400_device = {
    "SID",                 /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xd400, 0xd41f, 0x1f,  /* main registers */
    1,                     /* read is always valid */
    sid_store,             /* store function */
    NULL,                  /* NO poke function */
    sid_read,              /* read function */
    sid_peek,              /* peek function */
    sid_dump,              /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_d420_device = {
    "SID $D420-$D4FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,           /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xd420, 0xd4ff, 0x1f,      /* mirrors of $d400-$d41f */
    1,                         /* read is always valid */
    sid_store,                 /* store function */
    NULL,                      /* NO poke function */
    sid_read,                  /* read function */
    sid_peek,                  /* peek function */
    sid_dump,                  /* chip state information dump function */
    IO_CART_ID_NONE,           /* not a cartridge */
    IO_PRIO_LOW,               /* low priority, chip never involved in collisions, this is to allow additional SID chips in the same range */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_d500_device = {
    "SID $D500-$D5FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,           /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xd500, 0xd5ff, 0x1f,      /* mirrors of $d400-$d41f */
    1,                         /* read is always valid */
    sid_store,                 /* store function */
    NULL,                      /* NO poke function */
    sid_read,                  /* read function */
    sid_peek,                  /* peek function */
    sid_dump,                  /* chip state information dump function */
    IO_CART_ID_NONE,           /* not a cartridge */
    IO_PRIO_LOW,               /* low priority, chip never involved in collisions, this is to allow additional SID chips in the same range */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_d600_device = {
    "SID $D600-$D6FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,           /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xd600, 0xd6ff, 0x1f,      /* mirrors of $d400-$d41f */
    1,                         /* read is always valid */
    sid_store,                 /* store function */
    NULL,                      /* NO poke function */
    sid_read,                  /* read function */
    sid_peek,                  /* peek function */
    sid_dump,                  /* chip state information dump function */
    IO_CART_ID_NONE,           /* not a cartridge */
    IO_PRIO_LOW,               /* low priority, chip never involved in collisions, this is to allow additional SID chips in the same range */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_d700_device = {
    "SID $D700-$D7FF mirrors", /* name of the chip */
    IO_DETACH_NEVER,           /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xd700, 0xd7ff, 0x1f,      /* mirrors of $d400-$d41f */
    1,                         /* read is always valid */
    sid_store,                 /* store function */
    NULL,                      /* NO poke function */
    sid_read,                  /* read function */
    sid_peek,                  /* peek function */
    sid_dump,                  /* chip state information dump function */
    IO_CART_ID_NONE,           /* not a cartridge */
    IO_PRIO_LOW,               /* low priority, chip never involved in collisions, this is to allow additional SID chips in the same range */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *vicii_d000_list_item = NULL;
static io_source_list_t *vicii_d100_list_item = NULL;
static io_source_list_t *vicii_d200_list_item = NULL;
static io_source_list_t *vicii_d300_list_item = NULL;
static io_source_list_t *sid_d400_list_item = NULL;
static io_source_list_t *sid_d420_list_item = NULL;
static io_source_list_t *sid_d500_list_item = NULL;
static io_source_list_t *sid_d600_list_item = NULL;
static io_source_list_t *sid_d700_list_item = NULL;

void c64io_vicii_reinit(void)
{
    vicii_d000_list_item = io_source_register(&vicii_d000_device);
    vicii_d100_list_item = io_source_register(&vicii_d100_device);
    vicii_d200_list_item = io_source_register(&vicii_d200_device);
    vicii_d300_list_item = io_source_register(&vicii_d300_device);
}

void c64io_vicii_init(void)
{
    int memhack = 0;

    resources_get_int("MemoryHack", &memhack);

    if (memhack != MEMORY_HACK_PLUS60K && memhack != MEMORY_HACK_PLUS256K) {
        c64io_vicii_reinit();
    }
}

void c64io_vicii_deinit(void)
{
    if (vicii_d000_list_item != NULL) {
        io_source_unregister(vicii_d000_list_item);
        vicii_d000_list_item = NULL;
    }

    if (vicii_d100_list_item != NULL) {
        io_source_unregister(vicii_d100_list_item);
        vicii_d100_list_item = NULL;
    }

    if (vicii_d200_list_item != NULL) {
        io_source_unregister(vicii_d200_list_item);
        vicii_d200_list_item = NULL;
    }

    if (vicii_d300_list_item != NULL) {
        io_source_unregister(vicii_d300_list_item);
        vicii_d300_list_item = NULL;
    }
}

/* C64-specific I/O initialization. */
static void c64io_init(void)
{
    c64io_vicii_init();
    sid_d400_list_item = io_source_register(&sid_d400_device);
    sid_d420_list_item = io_source_register(&sid_d420_device);
    sid_d500_list_item = io_source_register(&sid_d500_device);
    sid_d600_list_item = io_source_register(&sid_d600_device);
    sid_d700_list_item = io_source_register(&sid_d700_device);
}

/* ------------------------------------------------------------------------ */

static joyport_port_props_t control_port_1 =
{
    "Control port 1",
    1,                  /* has a potentiometer connected to this port */
    1,                  /* has lightpen support on this port */
    1                   /* port is always active */
};

static joyport_port_props_t control_port_2 =
{
    "Control port 2",
    1,                  /* has a potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    1                   /* port is always active */
};

static joyport_port_props_t userport_joy_control_port_1 =
{
    "Userport joystick adapter port 1",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t userport_joy_control_port_2 =
{
    "Userport joystick adapter port 2",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0                   /* port can be switched on/off */
};

static int init_joyport_ports(void)
{
    if (joyport_port_register(JOYPORT_1, &control_port_1) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_2, &control_port_2) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_3, &userport_joy_control_port_1) < 0) {
        return -1;
    }
    return joyport_port_register(JOYPORT_4, &userport_joy_control_port_2);
}

/* C64-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0) {
        init_resource_fail("traps");
        return -1;
    }
    if (rombanks_resources_init() < 0) {
        init_resource_fail("rombanks");
        return -1;
    }
    if (c64_resources_init() < 0) {
        init_resource_fail("c64");
        return -1;
    }
    if (export_resources_init() < 0) {
        init_resource_fail("c64export");
        return -1;
    }
    if (memory_hacks_resources_init() < 0) {
        init_resource_fail("memory hacks");
        return -1;
    }
    if (plus60k_resources_init() < 0) {
        init_resource_fail("plus60k");
        return -1;
    }
    if (plus256k_resources_init() < 0) {
        init_resource_fail("plus256k");
        return -1;
    }
    if (c64_256k_resources_init() < 0) {
        init_resource_fail("c64 256k");
        return -1;
    }
    if (vicii_resources_init() < 0) {
        init_resource_fail("vicii");
        return -1;
    }
    if (sid_resources_init() < 0) {
        init_resource_fail("sid");
        return -1;
    }
    if (rs232drv_resources_init() < 0) {
        init_resource_fail("rs232drv");
        return -1;
    }
    if (rsuser_resources_init() < 0) {
        init_resource_fail("rsuser");
        return -1;
    }
    if (serial_resources_init() < 0) {
        init_resource_fail("serial");
        return -1;
    }
    if (printer_resources_init() < 0) {
        init_resource_fail("printer");
        return -1;
    }
    if (printer_userport_resources_init() < 0) {
        init_resource_fail("userport printer");
        return -1;
    }
    if (init_joyport_ports() < 0) {
        init_resource_fail("joyport ports");
        return -1;
    }
    if (joyport_resources_init() < 0) {
        init_resource_fail("joyport devices");
        return -1;
    }
    if (joyport_sampler2bit_resources_init() < 0) {
        init_resource_fail("joyport 2bit sampler");
        return -1;
    }
    if (joyport_sampler4bit_resources_init() < 0) {
        init_resource_fail("joyport 4bit sampler");
        return -1;
    }
    if (joyport_bbrtc_resources_init() < 0) {
        init_resource_fail("joyport bbrtc");
        return -1;
    }
    if (joyport_paperclip64_resources_init() < 0) {
        init_resource_fail("joyport paperclip64 dongle");
        return -1;
    }
    if (joyport_coplin_keypad_resources_init() < 0) {
        init_resource_fail("joyport coplin keypad");
        return -1;
    }
    if (joyport_cx21_resources_init() < 0) {
        init_resource_fail("joyport cx21 keypad");
        return -1;
    }
    if (joyport_script64_dongle_resources_init() < 0) {
        init_resource_fail("joyport script64 dongle");
        return -1;
    }
    if (joyport_vizawrite64_dongle_resources_init() < 0) {
        init_resource_fail("joyport vizawrite64 dongle");
        return -1;
    }
    if (joyport_waasoft_dongle_resources_init() <0) {
        init_resource_fail("joyport waasoft dongle");
        return -1;
    }
    if (joyport_cx85_resources_init() < 0) {
        init_resource_fail("joyport cx85 keypad");
        return -1;
    }
    if (joyport_rushware_keypad_resources_init() < 0) {
        init_resource_fail("joyport rushware keypad");
        return -1;
    }
    if (joyport_cardkey_resources_init() < 0) {
        init_resource_fail("joyport cardkey keypad");
        return -1;
    }
    if (joyport_snespad_resources_init() < 0) {
        init_resource_fail("joyport snespad");
        return -1;
    }
    if (joystick_resources_init() < 0) {
        init_resource_fail("joystick");
        return -1;
    }
    if (userport_resources_init() < 0) {
        init_resource_fail("userport devices");
        return -1;
    }
    if (gfxoutput_resources_init() < 0) {
        init_resource_fail("gfxoutput");
        return -1;
    }
    if (sampler_resources_init() < 0) {
        init_resource_fail("samplerdrv");
        return -1;
    }
    if (fliplist_resources_init() < 0) {
        init_resource_fail("flip list");
        return -1;
    }
    if (file_system_resources_init() < 0) {
        init_resource_fail("file system");
        return -1;
    }
    /* Initialize file system device-specific resources.  */
    if (fsdevice_resources_init() < 0) {
        init_resource_fail("file system device");
        return -1;
    }
    if (disk_image_resources_init() < 0) {
        init_resource_fail("disk image");
        return -1;
    }
    if (event_resources_init() < 0) {
        init_resource_fail("event");
        return -1;
    }
    if (kbdbuf_resources_init() < 0) {
        init_resource_fail("Keyboard");
        return -1;
    }
    if (autostart_resources_init() < 0) {
        init_resource_fail("autostart");
        return -1;
    }
#ifdef HAVE_NETWORK
    if (network_resources_init() < 0) {
        init_resource_fail("network");
        return -1;
    }
#endif
#ifdef DEBUG
    if (debug_resources_init() < 0) {
        init_resource_fail("debug");
        return -1;
    }
#endif
#ifdef HAVE_MOUSE
    if (mouse_resources_init() < 0) {
        init_resource_fail("mouse");
        return -1;
    }
#ifdef HAVE_LIGHTPEN
    if (lightpen_resources_init() < 0) {
        init_resource_fail("lightpen");
        return -1;
    }
#endif
#endif
    if (drive_resources_init() < 0) {
        init_resource_fail("drive");
        return -1;
    }
    /*
     * This needs to be called before tapeport_resources_init(), otherwise
     * the tapecart will fail to initialize due to the Datasette resource
     * appearing after the Tapecart resources
     */
    if (datasette_resources_init() < 0) {
        init_resource_fail("datasette");
        return -1;
    }
    if (tapeport_resources_init() < 0) {
        init_resource_fail("tapeport");
        return -1;
    }
    if (tape_diag_586220_harness_resources_init() < 0) {
        init_resource_fail("tape diag 586220 harness");
        return -1;
    }
    if (c64_glue_resources_init() < 0) {
        init_resource_fail("c64 glue");
        return -1;
    }
    if (userport_joystick_resources_init() < 0) {
        init_resource_fail("userport joystick");
        return -1;
    }
    if (userport_dac_resources_init() < 0) {
        init_resource_fail("userport dac");
        return -1;
    }
    if (userport_digimax_resources_init() < 0) {
        init_resource_fail("userport dac");
        return -1;
    }
    if (userport_rtc_58321a_resources_init() < 0) {
        init_resource_fail("userport rtc (58321a)");
        return -1;
    }
    if (userport_rtc_ds1307_resources_init() < 0) {
        init_resource_fail("userport rtc (ds1307)");
        return -1;
    }
    if (userport_4bit_sampler_resources_init() < 0) {
        init_resource_fail("userport 4bit sampler");
        return -1;
    }
    if (userport_8bss_resources_init() < 0) {
        init_resource_fail("userport 8bit stereo sampler");
        return -1;
    }
    if (userport_diag_586220_harness_resources_init() < 0) {
        init_resource_fail("userport diag 586220 harness");
        return -1;
    }
    if (cartio_resources_init() < 0) {
        init_resource_fail("cartio");
        return -1;
    }
    if (cartridge_resources_init() < 0) {
        init_resource_fail("cartridge");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    serial_shutdown();
    c64_resources_shutdown();
    plus60k_resources_shutdown();
    plus256k_resources_shutdown();
    c64_256k_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    cartridge_resources_shutdown();
    rombanks_resources_shutdown();
    userport_rtc_58321a_resources_shutdown();
    userport_rtc_ds1307_resources_shutdown();
    cartio_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    userport_resources_shutdown();
    joyport_bbrtc_resources_shutdown();
    tapeport_resources_shutdown();
    tapecart_exit();
}

/* C64-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (c64_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64");
        return -1;
    }
    if (memory_hacks_cmdline_options_init() < 0) {
        init_cmdline_options_fail("memory hacks");
        return -1;
    }
    if (plus60k_cmdline_options_init() < 0) {
        init_cmdline_options_fail("plus60k");
        return -1;
    }
    if (plus256k_cmdline_options_init() < 0) {
        init_cmdline_options_fail("plus256k");
        return -1;
    }
    if (c64_256k_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64 256k");
        return -1;
    }
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
        return -1;
    }
    if (sid_cmdline_options_init(SIDTYPE_SID) < 0) {
        init_cmdline_options_fail("sid");
        return -1;
    }
    if (rs232drv_cmdline_options_init() < 0) {
        init_cmdline_options_fail("rs232drv");
        return -1;
    }
    if (rsuser_cmdline_options_init() < 0) {
        init_cmdline_options_fail("rsuser");
        return -1;
    }
    if (serial_cmdline_options_init() < 0) {
        init_cmdline_options_fail("serial");
        return -1;
    }
    if (printer_cmdline_options_init() < 0) {
        init_cmdline_options_fail("printer");
        return -1;
    }
    if (printer_userport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport printer");
        return -1;
    }
    if (joyport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("joyport");
        return -1;
    }
    if (joyport_bbrtc_cmdline_options_init() < 0) {
        init_cmdline_options_fail("bbrtc");
        return -1;
    }
    if (joystick_cmdline_options_init() < 0) {
        init_cmdline_options_fail("joystick");
        return -1;
    }
    if (userport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport");
        return -1;
    }
    if (gfxoutput_cmdline_options_init() < 0) {
        init_cmdline_options_fail("gfxoutput");
        return -1;
    }
    if (sampler_cmdline_options_init() < 0) {
        init_cmdline_options_fail("samplerdrv");
        return -1;
    }
    if (fliplist_cmdline_options_init() < 0) {
        init_cmdline_options_fail("flip list");
        return -1;
    }
    if (file_system_cmdline_options_init() < 0) {
        init_cmdline_options_fail("attach");
        return -1;
    }
    if (fsdevice_cmdline_options_init() < 0) {
        init_cmdline_options_fail("file system");
        return -1;
    }
    if (disk_image_cmdline_options_init() < 0) {
        init_cmdline_options_fail("disk image");
        return -1;
    }
    if (event_cmdline_options_init() < 0) {
        init_cmdline_options_fail("event");
        return -1;
    }
    if (kbdbuf_cmdline_options_init() < 0) {
        init_cmdline_options_fail("keyboard");
        return -1;
    }
    if (autostart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("autostart");
        return -1;
    }
#ifdef HAVE_NETWORK
    if (network_cmdline_options_init() < 0) {
        init_cmdline_options_fail("network");
        return -1;
    }
#endif
#ifdef DEBUG
    if (debug_cmdline_options_init() < 0) {
        init_cmdline_options_fail("debug");
        return -1;
    }
#endif
#ifdef HAVE_MOUSE
    if (mouse_cmdline_options_init() < 0) {
        init_cmdline_options_fail("mouse");
        return -1;
    }
#endif
    if (drive_cmdline_options_init() < 0) {
        init_cmdline_options_fail("drive");
        return -1;
    }
    if (tapeport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("tapeport");
        return -1;
    }
    if (tape_diag_586220_harness_cmdline_options_init() < 0) {
        init_cmdline_options_fail("tape diag 586220 harness");
        return -1;
    }
    if (datasette_cmdline_options_init() < 0) {
        init_cmdline_options_fail("datasette");
        return -1;
    }
    if (c64_glue_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64 glue");
        return -1;
    }
    if (userport_joystick_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport joystick");
        return -1;
    }
    if (userport_dac_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport dac");
        return -1;
    }
    if (userport_digimax_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport digimax");
        return -1;
    }
    if (userport_rtc_58321a_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport rtc (58321a)");
        return -1;
    }
    if (userport_rtc_ds1307_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport rtc (ds1307)");
        return -1;
    }
    if (userport_4bit_sampler_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport 4bit sampler");
        return -1;
    }
    if (userport_8bss_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport 8bit stereo sampler");
        return -1;
    }
    if (userport_diag_586220_harness_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport diag 586220 harness");
        return -1;
    }
    if (cartio_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartio");
        return -1;
    }
    if (cartridge_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartridge");
        return -1;
    }
    return 0;
}

static void c64_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502, asmR65C02, asmz80;
    monitor_interface_t *drive_interface_init[DRIVE_NUM];
    monitor_cpu_type_t *asmarray[4];
    int i = 0;

    asmarray[i++] = &asm6502;
    asmarray[i++] = &asmz80;
    asmarray[i++] = &asmR65C02;
    asmarray[i] = NULL;

    asm6502_init(&asm6502);
    asmR65C02_init(&asmR65C02);
    asmz80_init(&asmz80);

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive_interface_init[dnr] = drive_cpu_monitor_interface_get(dnr);
    }

    /* Initialize the monitor.  */
    monitor_init(maincpu_monitor_interface_get(), drive_interface_init, asmarray);
}

void machine_setup_context(void)
{
    cia1_setup_context(&machine_context);
    cia2_setup_context(&machine_context);
    cartridge_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

/* C64-specific initialization.  */
int machine_specific_init(void)
{
    int delay;

    c64_log = log_open("C64");

    if (mem_load() < 0) {
        return -1;
    }

    event_init();

    /* Setup trap handling.  */
    traps_init();

    /* Initialize serial traps.  */
    if (serial_init(c64_serial_traps) < 0) {
        return -1;
    }

    serial_trap_init(0xa4);
    serial_iec_bus_init();

    /* Initialize RS232 handler.  */
    rs232drv_init();
    c64_rsuser_init();

    /* Initialize print devices.  */
    printer_init();

    /* Initialize the tape emulation.  */
    tape_init(&tapeinit);

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level drive emulation.  */
    drive_init();

    disk_image_init();

    resources_get_int("AutostartDelay", &delay);
    if (delay == 0) {
        delay = 3; /* default */
    }

    /* Initialize autostart.  */
    autostart_init((CLOCK)(delay * C64_PAL_RFSH_PER_SEC * C64_PAL_CYCLES_PER_RFSH), 1);

    /* Pre-init C64-specific parts of the menus before vicii_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        c64_mem_ui_init_early();
    }

    if (vicii_init(VICII_STANDARD) == NULL && !video_disabled_mode) {
        return -1;
    }

    c64_mem_init();

    cia1_init(machine_context.cia1);
    cia2_init(machine_context.cia2);

    /* Initialize the keyboard.  */
    c64keyboard_init();

    c64_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);

    /* Initialize native sound chip */
    sid_sound_chip_init();
    fmopl_set_machine_parameter(machine_timing.cycles_per_sec);

    /* Initialize cartridge based sound chips */
    cartridge_sound_chip_init();

    /* Initialize userport based sound chips */
    userport_dac_sound_chip_init();
    userport_digimax_sound_chip_init();

    /* Initialize mp3@64 */
#ifdef USE_MPG123
    clockport_mp3at64_sound_chip_init();
#endif

    drive_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);

    /* Initialize keyboard buffer.  */
    kbdbuf_init(631, 198, 10,
            (CLOCK)(machine_timing.rfsh_per_sec *
                machine_timing.cycles_per_rfsh * KBDBUF_ALARM_DELAY));

    /* Initialize the C64-specific I/O */
    c64io_init();

    gfxoutput_init();

    /* Initialize the C64-specific part of the UI.  */
    if (!console_mode) {
        c64_mem_ui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    /* Initialize glue logic.  */
    c64_glue_init();

    /* Initialize the +60K.  */
    plus60k_init();

    /* Initialize the +256K.  */
    plus256k_init();

    /* Initialize the C64 256K.  */
    c64_256k_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();

#ifdef HAVE_LIGHTPEN
    /* Initialize lightpen support and register VICII callbacks */
    lightpen_init();
    lightpen_register_timing_callback(vicii_lightpen_timing, 0);
    lightpen_register_trigger_callback(vicii_trigger_light_pen);
#endif
#endif
    c64iec_init();
    c64fastiec_init();

    cartridge_init();

    machine_drive_stub();

    return 0;
}

/* C64-specific reset sequence.  */
static int reset_poweron = 1;

void machine_specific_reset(void)
{
    int iecreset = 1;
    resources_get_int("IECReset", &iecreset);

    serial_traps_reset();

    ciacore_reset(machine_context.cia1);
    ciacore_reset(machine_context.cia2);
    sid_reset();

    rs232drv_reset(); /* driver is used by both user- and expansion port ? */
    rsuser_reset();

    printer_reset();

    /* FIXME: whats actually broken here? */
    /* reset_reu(); */

    /* The VIC-II must be the *last* to be reset.  */
    vicii_reset();

    cartridge_reset();
    if (reset_poweron || iecreset) {
        drive_reset();
    }
    datasette_reset();
    plus60k_reset();
    plus256k_reset();
    c64_256k_reset();

    sampler_reset();

    reset_poweron = 0;
}

void machine_specific_powerup(void)
{
    vicii_reset_registers();
    reset_poweron = 1;
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(1);

    /* and cartridge */
    cartridge_detach_image(-1);

    ciacore_shutdown(machine_context.cia1);
    ciacore_shutdown(machine_context.cia2);

    /* close the video chip(s) */
    vicii_shutdown();

    plus60k_shutdown();
    plus256k_shutdown();
    c64_256k_shutdown();

    cartridge_shutdown();

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    sid_cmdline_options_shutdown();

    if (!console_mode) {
        c64_mem_ui_shutdown();
    }
}

void machine_handle_pending_alarms(int num_write_cycles)
{
    vicii_handle_pending_alarms_external(num_write_cycles);
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    CLOCK sub;

    network_hook();

    drive_vsync_hook();

    autostart_advance();

    screenshot_record();

    sub = clk_guard_prevent_overflow(maincpu_clk_guard);

    /* The drive has to deal both with our overflowing and its own one, so
       it is called even when there is no overflowing in the main CPU.  */
    drive_cpu_prevent_clk_overflow_all(sub);
}

void machine_set_restore_key(int v)
{
    c64keyboard_restore_key(v);
}

int machine_has_restore_key(void)
{
    return 1;
}

/* ------------------------------------------------------------------------- */

long machine_get_cycles_per_second(void)
{
    return machine_timing.cycles_per_sec;
}

long machine_get_cycles_per_frame(void)
{
    return machine_timing.cycles_per_rfsh;
}

void machine_get_line_cycle(unsigned int *line, unsigned int *cycle, int *half_cycle)
{
    *line = (unsigned int)((maincpu_clk) / machine_timing.cycles_per_line % machine_timing.screen_lines);
    *cycle = (unsigned int)((maincpu_clk) % machine_timing.cycles_per_line);
    *half_cycle = (int)-1;
}

void machine_change_timing(int timeval, int border_mode)
{
    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = C64_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C64_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        case MACHINE_SYNC_NTSCOLD:
            machine_timing.cycles_per_sec = C64_NTSCOLD_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_NTSCOLD_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_NTSCOLD_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_NTSCOLD_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_NTSCOLD_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        case MACHINE_SYNC_PALN:
            machine_timing.cycles_per_sec = C64_PALN_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_PALN_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_PALN_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_PALN_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_PALN_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        default:
            log_error(c64_log, "Unknown machine timing.");
    }

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);
    debug_set_machine_parameter(machine_timing.cycles_per_line, machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);
    serial_iec_device_set_machine_parameter(machine_timing.cycles_per_sec);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);
#ifdef HAVE_MOUSE
    neos_mouse_set_machine_parameter(machine_timing.cycles_per_sec);
#endif
    clk_guard_set_clk_base(maincpu_clk_guard, machine_timing.cycles_per_rfsh);

    vicii_change_timing(&machine_timing, border_mode);

    cia1_set_timing(machine_context.cia1, machine_timing.cycles_per_sec, machine_timing.power_freq);
    cia2_set_timing(machine_context.cia2, machine_timing.cycles_per_sec, machine_timing.power_freq);

    fmopl_set_machine_parameter(machine_timing.cycles_per_sec);

    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks, int event_mode)
{
    int err = c64_snapshot_write(name, save_roms, save_disks, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_WRITE_SNAPSHOT);
    }
    return err;
}

int machine_read_snapshot(const char *name, int event_mode)
{
    int err = c64_snapshot_read(name, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_READ_SNAPSHOT);
    }
    return err;
}

/* ------------------------------------------------------------------------- */
/* FIXME: those two shouldnt be here anymore */
int machine_autodetect_psid(const char *name)
{
/*
    if (name == NULL) {
        return -1;
    }

    return psid_load_file(name);
*/
    return -1;
}

void machine_play_psid(int tune)
{
    /* psid_set_tune(tune); */
}

/* ------------------------------------------------------------------------- */

int machine_screenshot(screenshot_t *screenshot, struct video_canvas_s *canvas)
{
    if (canvas != vicii_get_canvas()) {
        return -1;
    }

    vicii_screenshot(screenshot);
    return 0;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh, struct video_canvas_s *canvas)
{
    if (canvas != vicii_get_canvas()) {
        return -1;
    }

    vicii_async_refresh(refresh);
    return 0;
}

void machine_update_memory_ptrs(void)
{
    vicii_update_memory_ptrs_external();
}

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return diskcontents_iec_read(unit);
}

uint8_t machine_tape_type_default(void)
{
    return TAPE_CAS_TYPE_PRG;
}

uint8_t machine_tape_behaviour(void)
{
    return TAPE_BEHAVIOUR_NORMAL;
}

static int get_cart_emulation_state(void)
{
    int value;

    if (resources_get_int("CartridgeType", &value) < 0) {
        return CARTRIDGE_NONE;
    }

    return value;
}

static int check_cart_range(unsigned int addr)
{
    if (get_cart_emulation_state() == CARTRIDGE_NONE) {
        return 1;
    }

    return (!(addr >= 0x8000 && addr < 0xa000));
}

int machine_addr_in_ram(unsigned int addr)
{
    return ((addr < 0xe000 && !(addr >= 0xa000 && addr < 0xc000)) && check_cart_range(addr));
}

const char *machine_get_name(void)
{
    if (machine_class == VICE_MACHINE_C64SC) {
        return "C64SC";
    }

    return machine_name;
}

/* ------------------------------------------------------------------------- */

static void c64_userport_set_flag(uint8_t b)
{
    if (b != 0) {
        ciacore_set_flag(machine_context.cia2);
    }
}

static userport_port_props_t userport_props = {
    1,                     /* port has the pa2 pin */
    1,                     /* port has the pa3 pin */
    c64_userport_set_flag, /* port has the flag pin, set flag function */
    1,                     /* port has the pc pin */
    1                      /* port has the cnt1, cnt2 and sp pins */
};

int machine_register_userport(void)
{
    userport_port_register(&userport_props);

    return 0;
}

/* ------------------------------------------------------------------------- */

/** \brief  List of drive type names and ID's supported by C64
 *
 * Convenience function for UI's. This list should be updated whenever drive
 * types are added or removed.
 *
 * XXX: This is here because c64drive.c is compiled into x64dtv, which supports
 *      fewer drive types.
 */
static drive_type_info_t drive_type_info_list[] = {
    { DRIVE_NAME_NONE, DRIVE_TYPE_NONE },
    { DRIVE_NAME_1540, DRIVE_TYPE_1540 },
    { DRIVE_NAME_1541, DRIVE_TYPE_1541 },
    { DRIVE_NAME_1541II, DRIVE_TYPE_1541II },
    { DRIVE_NAME_1570, DRIVE_TYPE_1570 },
    { DRIVE_NAME_1571, DRIVE_TYPE_1571 },
    { DRIVE_NAME_1581, DRIVE_TYPE_1581 },
    { DRIVE_NAME_2000, DRIVE_TYPE_2000 },
    { DRIVE_NAME_4000, DRIVE_TYPE_4000 },
    { DRIVE_NAME_2031, DRIVE_TYPE_2031 },
    { DRIVE_NAME_2040, DRIVE_TYPE_2040 },
    { DRIVE_NAME_3040, DRIVE_TYPE_3040 },
    { DRIVE_NAME_4040, DRIVE_TYPE_4040 },
    { DRIVE_NAME_1001, DRIVE_TYPE_1001 },
    { DRIVE_NAME_8050, DRIVE_TYPE_8050 },
    { DRIVE_NAME_8250, DRIVE_TYPE_8250 },
    { NULL, -1 }
};

/** \brief  Get a list of (name, id) tuples for the drives handles by C64
 *
 * Usefull for UI's, get a list of currently supported drive types with a name
 * to display and and ID to use in callbacks.
 *
 * \return  list of drive types, NULL terminated
 *
 * \note    'supported' in this context means the drives C64 can support, not
 *          what actually is supported due to ROMs and other settings
 */
drive_type_info_t *machine_drive_get_type_info_list(void)
{
    return drive_type_info_list;
}
