/*
 * vic20.c
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "attach.h"
#include "autostart.h"
#include "bbrtc.h"
#include "cardkey.h"
#include "cartridge.h"
#include "cartio.h"
#include "coplin_keypad.h"
#include "cx21.h"
#include "cx85.h"
#include "datasette.h"
#include "datasette-sound.h"
#include "debug.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "fliplist.h"
#include "fmopl.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "iecdrive.h"
#include "imagecontents.h"
#include "inception.h"
#include "init.h"
#include "joyport.h"
#include "joyport_io_sim.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "log.h"
#include "resources.h"
#include "machine-drive.h"
#include "machine-printer.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "network.h"
#include "ninja_snespad.h"
#include "parallel.h"
#include "printer.h"
#include "protopad.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "rushware_keypad.h"
#include "sampler.h"
#include "sampler2bit.h"
#include "sampler4bit.h"
#include "screenshot.h"
#include "script64_dongle.h"
#include "serial.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sound.h"
#include "spaceballs.h"
#include "tape.h"
#include "tapeport.h"
#include "traps.h"
#include "trapthem_snespad.h"
#include "types.h"
#include "userport.h"
#include "userport_dac.h"
#include "userport_io_sim.h"
#include "userport_joystick.h"
#include "userport_petscii_snespad.h"
#include "userport_rtc_58321a.h"
#include "userport_rtc_ds1307.h"
#include "via.h"
#include "vic.h"
#include "vic-mem.h"
#include "vic20-cmdline-options.h"
#include "vic20-ieee488.h"
#include "vic20-midi.h"
#include "vic20-resources.h"
#include "vic20-snapshot.h"
#include "vic20.h"
#include "vic20iec.h"
#include "vic20ieeevia.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "vic20sound.h"
#include "vic20rsuser.h"
#include "vic20ui.h"
#include "vic20via.h"
#include "vice-event.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "lightpen.h"
#include "mouse.h"
#endif

/** \brief  Delay in seconds before pasting -keybuf argument into the buffer
 */
#define KBDBUF_ALARM_DELAY   1


machine_context_t machine_context;

const char machine_name[] = "VIC20";
int machine_class = VICE_MACHINE_VIC20;

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

static const trap_t vic20_serial_traps[] = {
    {
        "SerialListen",
        0xEE2E,
        0xEEB2,
        { 0x20, 0xA0, 0xE4 },
        serial_trap_attention,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        "SerialSaListen",
        0xEE40,
        0xEEB2,
        { 0x20, 0x8D, 0xEF },
        serial_trap_attention,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        "SerialSendByte",
        0xEE49,
        0xEEB2,
        { 0x78, 0x20, 0xA0 },
        serial_trap_send,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        "SerialReceiveByte",
        0xEF19,
        0xEEB2,
        { 0x78, 0xA9, 0x00 },
        serial_trap_receive,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        "SerialReady",
        0xE4B2,
        0xEEB2,
        { 0xAD, 0x1F, 0x91 },
        serial_trap_ready,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        NULL,
        0,
        0,
        {0, 0, 0},
        NULL,
        NULL,
        NULL
    }
};

/* Tape traps.  */
static const trap_t vic20_tape_traps[] = {
    {
        "TapeFindHeader",
        0xF7B2,
        0xF7B5,
        { 0x20, 0xC0, 0xF8 },
        tape_find_header_trap,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        "TapeReceive",
        0xF90B,
        0xFCCF,
        { 0x20, 0xFB, 0xFC },
        tape_receive_trap,
        vic20memrom_trap_read,
        vic20memrom_trap_store
    },
    {
        NULL,
        0,
        0,
        { 0, 0, 0 },
        NULL,
        NULL,
        NULL
    }
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
    vic20_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static log_t vic20_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

static int via2_dump(void)
{
    return viacore_dump(machine_context.via2);
}

static int via1_dump(void)
{
    return viacore_dump(machine_context.via1);
}

static void vic_via1_via2_store(uint16_t addr, uint8_t data)
{
    if (addr & 0x10) {
        via2_store(addr, data);
    }
    if (addr & 0x20) {
        via1_store(addr, data);
    }
    vic_store(addr, data);
}

static uint8_t vic_via1_via2_read(uint16_t addr)
{
    uint8_t retval = vic_read(addr);

    if (addr & 0x10) {
        retval &= via2_read(addr);
    }

    if (addr & 0x20) {
        retval &= via1_read(addr);
    }

    return retval;
}

static uint8_t vic_via1_via2_peek(uint16_t addr)
{
    uint8_t retval = vic_peek(addr);

    if (addr & 0x10) {
        retval &= via2_peek(addr);
    }

    if (addr & 0x20) {
        retval &= via1_peek(addr);
    }

    return retval;
}

static void via1_via2_store(uint16_t addr, uint8_t data)
{
    if (addr & 0x10) {
        via2_store(addr, data);
    }
    if (addr & 0x20) {
        via1_store(addr, data);
    }
}

static uint8_t via1_via2_read(uint16_t addr)
{
    uint8_t retval = 0xff;

    if (addr & 0x10) {
        retval &= via2_read(addr);
    }

    if (addr & 0x20) {
        retval &= via1_read(addr);
    }

    return retval;
}

static uint8_t via1_via2_peek(uint16_t addr)
{
    uint8_t retval = 0xff;

    if (addr & 0x10) {
        retval &= via2_peek(addr);
    }

    if (addr & 0x20) {
        retval &= via1_peek(addr);
    }

    return retval;
}

/* FIXME: the upper 4 bits of the mask are used to indicate the register size if not equal to the mask,
          this is done as a temporary HACK to keep mirrors working and still get the correct register size,
          this needs to be fixed properly after the 3.6 release */
static io_source_t vic_device = {
    "VIC",                 /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
#if 0
    0x9000, 0x90ff, 0x3f,  /* address range for the device, must include A5/A4 */
#endif
    0x9000, 0x90ff, 0xf03f,  /* address range for the device, must include A5/A4 */
    1,                     /* read is always valid */
    vic_via1_via2_store,   /* store function */
    NULL,                  /* NO poke function */
    vic_via1_via2_read,    /* read function */
    vic_via1_via2_peek,    /* peek function */
    vic_dump,              /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip and mirrors never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

/* FIXME: the upper 4 bits of the mask are used to indicate the register size if not equal to the mask,
          this is done as a temporary HACK to keep mirrors working and still get the correct register size,
          this needs to be fixed properly after the 3.6 release */
static io_source_t via2_device = {
    "VIA2",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
#if 0
    0x9110, 0x93ff, 0x3f,  /* address range for the device, must include A5/A4 */
#endif
    0x9110, 0x93ff, 0xf03f,  /* address range for the device, must include A5/A4 */
    1,                     /* read is always valid */
    via1_via2_store,       /* store function */
    NULL,                  /* NO poke function */
    via1_via2_read,        /* read function */
    via1_via2_peek,        /* peek function */
    via2_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip and mirrors never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

/* FIXME: the upper 4 bits of the mask are used to indicate the register size if not equal to the mask,
          this is done as a temporary HACK to keep mirrors working and still get the correct register size,
          this needs to be fixed properly after the 3.6 release */
static io_source_t via1_device = {
    "VIA1",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
#if 0
    0x9120, 0x93ff, 0x3f,  /* address range for the device, must include A5/A4 */
#endif
    0x9120, 0x93ff, 0xf03f,  /* address range for the device, must include A5/A4 */
    1,                     /* read is always valid */
    via1_via2_store,       /* store function */
    NULL,                  /* NO poke function */
    via1_via2_read,        /* read function */
    via1_via2_peek,        /* peek function */
    via1_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip and mirrors never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *vic_list_item = NULL;
static io_source_list_t *via1_list_item = NULL;
static io_source_list_t *via2_list_item = NULL;

static void vic20io0_init(void)
{
    vic_list_item = io_source_register(&vic_device);
    via1_list_item = io_source_register(&via1_device);
    via2_list_item = io_source_register(&via2_device);
}

/* ------------------------------------------------------------------------ */

static joyport_port_props_t control_port = {
    "Control port",
    1,  /* has a potentiometer connected to this port */
    1,  /* has lightpen support on this port */
    1,  /* has joystick adapter on this port */
    1,  /* has output support on this port */
    1   /* port is always active */
};

static joyport_port_props_t joy_adapter_control_port_1 = {
    "Joystick adapter port 1",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_2 = {
    "Joystick adapter port 2",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_3 = {
    "Joystick adapter port 3",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_4 = {
    "Joystick adapter port 4",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_5 = {
    "Joystick adapter port 5",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_6 = {
    "Joystick adapter port 6",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_7 = {
    "Joystick adapter port 7",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_8 = {
    "Joystick adapter port 8",
    0,  /* has NO potentiometer connected to this port */
    0,  /* has NO lightpen support on this port */
    0,  /* has NO joystick adapter on this port */
    1,  /* has output support on this port */
    0   /* port can be switched on/off */
};

static int init_joyport_ports(void)
{
    if (joyport_port_register(JOYPORT_1, &control_port) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_3, &joy_adapter_control_port_1) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_4, &joy_adapter_control_port_2) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_5, &joy_adapter_control_port_3) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_6, &joy_adapter_control_port_4) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_7, &joy_adapter_control_port_5) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_8, &joy_adapter_control_port_6) < 0) {
        return -1;
    }
    if (joyport_port_register(JOYPORT_9, &joy_adapter_control_port_7) < 0) {
        return -1;
    }
    return joyport_port_register(JOYPORT_10, &joy_adapter_control_port_8);
}

/* VIC20-specific resource initialization.  This is called before
   initializing the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0) {
        init_resource_fail("traps");
        return -1;
    }
    if (vic20_resources_init() < 0) {
        init_resource_fail("vic20");
        return -1;
    }
    if (vic_resources_init() < 0) {
        init_resource_fail("vic");
        return -1;
    }
    if (sidcart_resources_init() < 0) {
        init_resource_fail("sidcart");
        return -1;
    }
    if (rs232drv_resources_init() < 0) {
        init_resource_fail("rs232drv");
        return -1;
    }
    if (userport_resources_init() < 0) {
        init_resource_fail("userport devices");
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
    if (joyport_script64_dongle_resources_init() < 0) {
        init_resource_fail("joyport script64 dongle");
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
    if (joyport_trapthem_snespad_resources_init() < 0) {
        init_resource_fail("joyport trapthem snespad");
        return -1;
    }
    if (joyport_ninja_snespad_resources_init() < 0) {
        init_resource_fail("joyport ninja snespad");
        return -1;
    }
    if (joyport_protopad_resources_init() < 0) {
        init_resource_fail("joyport protopad");
        return -1;
    }
    if (joyport_spaceballs_resources_init() < 0) {
        init_resource_fail("joyport spaceballs");
        return -1;
    }
    if (joyport_inception_resources_init() < 0) {
        init_resource_fail("joyport inception");
        return -1;
    }
    if (joystick_resources_init() < 0) {
        init_resource_fail("joystick");
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
#ifdef HAVE_LIGHTPEN
    if (lightpen_resources_init() < 0) {
        init_resource_fail("lightpen");
        return -1;
    }
#endif
    if (mouse_resources_init() < 0) {
        init_resource_fail("mouse");
        return -1;
    }
#endif
    if (tapeport_resources_init(1) < 0) {
        init_resource_fail("tapeport");
        return -1;
    }
    if (cartridge_resources_init() < 0) {
        init_resource_fail("cartridge");
        return -1;
    }
#ifdef HAVE_MIDI
    if (vic20_midi_resources_init() < 0) {
        init_resource_fail("vic20 midi");
        return -1;
    }
#endif
    if (vic20_ieee488_resources_init() < 0) {
        init_resource_fail("vic20 ieee488");
        return -1;
    }
    if (userport_joystick_cga_resources_init() < 0) {
        init_resource_fail("userport cga joystick");
        return -1;
    }
    if (userport_joystick_pet_resources_init() < 0) {
        init_resource_fail("userport pet joystick");
        return -1;
    }
    if (userport_joystick_hummer_resources_init() < 0) {
        init_resource_fail("userport hummer joystick");
        return -1;
    }
    if (userport_joystick_oem_resources_init() < 0) {
        init_resource_fail("userport oem joystick");
        return -1;
    }
    if (userport_joystick_synergy_resources_init() < 0) {
        init_resource_fail("userport synergy joystick");
        return -1;
    }
    if (userport_dac_resources_init() < 0) {
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
    if (userport_petscii_snespad_resources_init() < 0) {
        init_resource_fail("userport petscii snes pad");
        return -1;
    }
    if (userport_io_sim_resources_init() < 0) {
        init_resource_fail("userport I/O simulation");
        return -1;
    }
    if (joyport_io_sim_resources_init() < 0) {
        init_resource_fail("joyport I/O simulation");
        return -1;
    }
    if (cartio_resources_init() < 0) {
        init_resource_fail("cartio");
        return -1;
    }
    /* Must be called after initializing cartridge resources. Some carts provide
     * additional busses.  The drive resources check the validity of the drive
     * type against the available busses on the system.  So if you had e.g. an
     * IEEE cart enabled and an IEEE defined, on startup the drive code would
     * reset the drive type to the default for the IEC bus. */
    if (drive_resources_init() < 0) {
        init_resource_fail("drive");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    serial_shutdown();
    vic20_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    cartridge_resources_shutdown();
#ifdef HAVE_MIDI
    midi_resources_shutdown();
#endif
    cartio_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    userport_rtc_58321a_resources_shutdown();
    userport_rtc_ds1307_resources_shutdown();
    joyport_bbrtc_resources_shutdown();
    tapeport_resources_shutdown();
}

/* VIC20-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (vic20_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vic20");
        return -1;
    }
    if (vic_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vic");
        return -1;
    }
    if (sidcart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("sidcart");
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
    if (cartridge_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartridge");
        return -1;
    }
#ifdef HAVE_MIDI
    if (vic20_midi_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vic20 midi");
        return -1;
    }
#endif
    if (vic20_ieee488_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vic20 ieee488");
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
    if (cartio_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartio");
        return -1;
    }
    return 0;
}

static void vic20_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502, asmR65C02;
    monitor_interface_t *drive_interface_init[NUM_DISK_UNITS];
    monitor_cpu_type_t *asmarray[3];

    asmarray[0] = &asm6502;
    asmarray[1] = &asmR65C02;
    asmarray[2] = NULL;

    asm6502_init(&asm6502);
    asmR65C02_init(&asmR65C02);

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        drive_interface_init[dnr] = drive_cpu_monitor_interface_get(dnr);
    }

    /* Initialize the monitor.  */
    monitor_init(maincpu_monitor_interface_get(), drive_interface_init,
                 asmarray);
}

void machine_setup_context(void)
{
    vic20via1_setup_context(&machine_context);
    vic20via2_setup_context(&machine_context);
    vic20ieeevia1_setup_context(&machine_context);
    vic20ieeevia2_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

void machine_handle_pending_alarms(CLOCK num_write_cycles)
{
}

/* VIC20-specific initialization.  */
int machine_specific_init(void)
{
    vic20_log = log_open("VIC20");

    if (mem_load() < 0) {
        return -1;
    }

    event_init();

    /* Setup trap handling.  */
    traps_init();

    gfxoutput_init();

    /* Initialize serial traps.  If user does not want them, or if the
       ``drive'' emulation is used, do not install them.  */
    if (serial_init(vic20_serial_traps) < 0) {
        return -1;
    }

    serial_trap_init(0xa4);
    serial_iec_bus_init();

    /* Initialize RS232 handler.  */
    rs232drv_init();
    vic20_rsuser_init();

    /* initialize print devices.  */
    printer_init();

    /* Initialize the tape emulation.  */
    tape_init(&tapeinit);

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level drive emulation. */
    drive_init();

    disk_image_init();

    /* Initialize autostart.  */
    autostart_init(3, 1);

    /* Pre-init VIC20-specific parts of the menus before vic_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        vic20ui_init_early();
    }

    /* Initialize the VIC-I emulation.  */
    if (vic_init() == NULL) {
        return -1;
    }

    via1_init(machine_context.via1);
    via2_init(machine_context.via2);

    ieeevia1_init(machine_context.ieeevia1);
    ieeevia2_init(machine_context.ieeevia2);

    vic20_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* Initialize native sound chip first */
    vic_sound_chip_init();

    /* Initialize the sidcart */
    sidcart_sound_chip_init();

    /* Initialize cartridge based sound chips */
    cartridge_sound_chip_init();

    /* Initialize userport based sound chips */
    userport_dac_sound_chip_init();

    drive_sound_init();
    datasette_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init((unsigned int)machine_timing.cycles_per_sec,
               (unsigned int)machine_timing.cycles_per_rfsh);
    fmopl_set_machine_parameter(machine_timing.cycles_per_sec);

    /* Initialize keyboard buffer.  */
    kbdbuf_init(631, 198, 10,
            (CLOCK)(machine_timing.cycles_per_rfsh *
                machine_timing.rfsh_per_sec * KBDBUF_ALARM_DELAY));

    /* Initialize the VIC20-specific I/O */
    vic20io0_init();

    /* Initialize the VIC20-specific part of the UI.  */
    if (!console_mode) {
        vic20ui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    vic20iec_init();

    cartridge_init();

#ifdef HAVE_MOUSE
    mouse_init();

#ifdef HAVE_LIGHTPEN
    /* Initialize lightpen support and register VIC-I callbacks */
    lightpen_init();
    lightpen_register_timing_callback(vic_lightpen_timing, 0);
    lightpen_register_trigger_callback(vic_trigger_light_pen);
#endif
#endif

    /* Register joystick callback (for lightpen triggering via fire button) */
    joystick_register_machine(via2_check_lightpen);

#ifdef HAVE_MIDI
    midi_init();
#endif

    machine_drive_stub();

    return 0;
}

/* VIC20-specific reset sequence.  */
void machine_specific_reset(void)
{
    serial_traps_reset();

    viacore_reset(machine_context.via1);
    viacore_reset(machine_context.via2);
    vic_reset();
    sid_reset();

    /* These calls must be before the VIA initialization */
    rs232drv_reset();
    userport_reset();

    viacore_reset(machine_context.ieeevia1);
    viacore_reset(machine_context.ieeevia2);

#ifdef HAVE_MIDI
    midi_reset();
#endif

    printer_reset();

    cartridge_reset();
    drive_reset();
    datasette_reset();

    sampler_reset();
}

/* VIC20-specific powerup/hardreset  */
void machine_specific_powerup(void)
{
    cartridge_powerup();
    userport_powerup();
    tapeport_powerup();
    joyport_powerup();
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(TAPEPORT_PORT_1 + 1);

    /* and cartridge */
    cartridge_detach_image(-1);

    viacore_shutdown(machine_context.via1);
    viacore_shutdown(machine_context.via2);
    viacore_shutdown(machine_context.ieeevia1);
    viacore_shutdown(machine_context.ieeevia2);

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    /* close the video chip(s) */
    vic_shutdown();

    sidcart_cmdline_options_shutdown();

    if (!console_mode) {
        vic20ui_shutdown();
    }
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    drive_vsync_hook();

    screenshot_record();
}

void machine_set_restore_key(int v)
{
    viacore_signal(machine_context.via2,
                   VIA_SIG_CA1, v ? VIA_SIG_FALL : VIA_SIG_RISE);
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
            machine_timing.cycles_per_sec = VIC20_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = VIC20_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = VIC20_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = VIC20_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = VIC20_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = VIC20_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = VIC20_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = VIC20_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = VIC20_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = VIC20_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        default:
            log_error(vic20_log, "Unknown machine timing.");
    }

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec,
                                machine_timing.cycles_per_rfsh);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);
    debug_set_machine_parameter(machine_timing.cycles_per_line,
                                machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);
    serial_iec_device_set_machine_parameter(machine_timing.cycles_per_sec);
#ifdef HAVE_MOUSE
    neos_mouse_set_machine_parameter(machine_timing.cycles_per_sec);
#endif

    vic_change_timing(&machine_timing, border_mode);

    fmopl_set_machine_parameter(machine_timing.cycles_per_sec);

    rsuser_change_timing(machine_timing.cycles_per_sec);

    mem_patch_kernal();

    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks,
                           int event_mode)
{
    int err = vic20_snapshot_write(name, save_roms, save_disks, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_WRITE_SNAPSHOT);
    }
    return err;
}

int machine_read_snapshot(const char *name, int event_mode)
{
    int err = vic20_snapshot_read(name, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_READ_SNAPSHOT);
    }
    return err;
}


/* ------------------------------------------------------------------------- */
int machine_autodetect_psid(const char *name)
{
    return -1;
}


int machine_screenshot(screenshot_t *screenshot, struct video_canvas_s *canvas)
{
    if (canvas != vic_get_canvas()) {
        return -1;
    }

    vic_screenshot(screenshot);
    return 0;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
{
    if (canvas != vic_get_canvas()) {
        return -1;
    }

    vic_async_refresh(refresh);
    return 0;
}

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return diskcontents_iec_read(unit);
}

uint8_t machine_tape_type_default(void)
{
    return TAPE_CAS_TYPE_BAS;
}

uint8_t machine_tape_behaviour(void)
{
    return TAPE_BEHAVIOUR_NORMAL;
}

int machine_addr_in_ram(unsigned int addr)
{
    if (addr >= 0x73 && addr <= 0x8a) {
        /* CHRGET zero page routine */
        return 0;
    }
    
    if (addr >= 0xc000) {
        /* ROM */
        return 0;
    }
    
    return 1;
}

const char *machine_get_name(void)
{
    return machine_name;
}

/* ------------------------------------------------------------------------- */

static void vic20_userport_set_flag(uint8_t b)
{
    viacore_signal(machine_context.via2, VIA_SIG_CB1, b ? VIA_SIG_RISE : VIA_SIG_FALL);
}

static userport_port_props_t userport_props = {
    1,                       /* port has the pa2 pin */
    0,                       /* port does NOT have the pa3 pin */
    vic20_userport_set_flag, /* port has the flag pin, set flag function */
    0,                       /* port does NOT have the pc pin */
    1,                       /* port does have the cnt1, cnt2 and sp pins */
    1                        /* port has the reset pin */
};

int machine_register_userport(void)
{
    userport_port_register(&userport_props);

    return 0;
}
