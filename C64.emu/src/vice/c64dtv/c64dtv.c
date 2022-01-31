/*
 * c64dtv.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
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
#include "c64cia.h"
#include "c64dtv-cmdline-options.h"
#include "c64dtv-resources.h"
#include "c64dtv-snapshot.h"
#include "c64dtv.h"
#include "c64dtvmem.h"
#include "c64dtvmemsnapshot.h"
#include "c64fastiec.h"
#include "c64iec.h"
#include "c64keyboard.h"
#include "c64memrom.h"
#include "c64ui.h"
#include "cia.h"
#include "coplin_keypad.h"
#include "debug.h"
#include "debugcart.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "flash-trap.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "imagecontents.h"
#include "inception.h"
#include "init.h"
#include "joyport.h"
#include "joyport_io_sim.h"
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
#include "multijoy.h"
#include "network.h"
#include "ninja_snespad.h"
#include "paperclip64.h"
#include "parallel.h"
#include "patchrom.h"
#include "printer.h"
#include "protopad.h"
#include "ps2mouse.h"
#include "resources.h"
#include "rs232drv.h"
#include "rushware_keypad.h"
#include "sampler.h"
#include "sampler2bit.h"
#include "sampler4bit.h"
#include "screenshot.h"
#include "serial.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "sound.h"
#include "tape.h"
#include "tapecart.h"
#include "tapeport.h"
#include "traps.h"
#include "trapthem_snespad.h"
#include "types.h"
#include "userport.h"
#include "userport_io_sim.h"
#include "userport_joystick.h"
#include "vice-event.h"
#include "vicii.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif

/** \brief  Delay in seconds before pasting -keybuf argument into the buffer
 */
#define KBDBUF_ALARM_DELAY   7


machine_context_t machine_context;

const char machine_name[] = "C64DTV";
int machine_class = VICE_MACHINE_C64DTV;

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
    {
        "SerialListen",
        0xED24,
        0xEDAB,
        { 0x20, 0x97, 0xEE },
        serial_trap_attention,
        c64memrom_trap_read,
        c64memrom_trap_store
    },
    {
        "SerialSaListen",
        0xED37,
        0xEDAB,
        { 0x20, 0x8E, 0xEE },
        serial_trap_attention,
        c64memrom_trap_read,
        c64memrom_trap_store
    },
    {
        "SerialSendByte",
        0xED41,
        0xEDAB,
        { 0x20, 0x97, 0xEE },
        serial_trap_send,
        c64memrom_trap_read,
        c64memrom_trap_store
    },
    {
        "SerialReceiveByte",
        0xEE14,
        0xEDAB,
        { 0xA9, 0x00, 0x85 },
        serial_trap_receive,
        c64memrom_trap_read,
        c64memrom_trap_store
    },
    {
        "SerialReady",
        0xEEA9,
        0xEDAB,
        { 0xAD, 0x00, 0xDD },
        serial_trap_ready,
        c64memrom_trap_read,
        c64memrom_trap_store
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

static log_t c64_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

static const trap_t c64dtv_flash_traps[] = {
    {
        "FlashSeekNext",
        0xF77F,
        0xF782,
        { 0x20, 0x79, 0xF8 },
        flash_trap_seek_next,
        c64memrom_trap_read,
        c64memrom_trap_store
    },
    {
        "FlashLoadBody",
        0xF7CF,
        0xF7D2,
        { 0x20, 0xE9, 0xF7 },
        flash_trap_load_body,
        c64memrom_trap_read,
        c64memrom_trap_store
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

/* ------------------------------------------------------------------------ */

static joyport_port_props_t control_port_1 =
{
    "Control port 1",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    1,                  /* has joystick adapter on this port */
    1,                  /* has output support on this port */
    1                   /* port is always active */
};

static joyport_port_props_t control_port_2 =
{
    "Control port 2",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    1,                  /* has joystick adapter on this port */
    1,                  /* has output support on this port */
    1                   /* port is always active */
};

static joyport_port_props_t joy_adapter_control_port_1 =
{
    "Joystick adapter port 1",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_2 =
{
    "Joystick adapter port 2",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_3 =
{
    "Joystick adapter port 3",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_4 =
{
    "Joystick adapter port 4",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_5 =
{
    "Joystick adapter port 5",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_6 =
{
    "Joystick adapter port 6",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_7 =
{
    "Joystick adapter port 7",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
    0                   /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_8 =
{
    "Joystick adapter port 8",
    0,                  /* has NO potentiometer connected to this port */
    0,                  /* has NO lightpen support on this port */
    0,                  /* has NO joystick adapter on this port */
    1,                  /* has output support on this port */
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

/* C64DTV-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0) {
        init_resource_fail("traps");
        return -1;
    }
    if (c64dtv_resources_init() < 0) {
        init_resource_fail("c64dtv");
        return -1;
    }
    if (c64dtvmem_resources_init() < 0) {
        init_resource_fail("c64dtvmem");
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
    if (serial_resources_init() < 0) {
        init_resource_fail("serial");
        return -1;
    }
    if (flash_trap_resources_init() < 0) {
        init_resource_fail("flash_trap");
        return -1;
    }
    if (printer_resources_init() < 0) {
        init_resource_fail("printer");
        return -1;
    }
    if (init_joyport_ports() < 0) {
        init_resource_fail("joyport ports");
        return -1;
    }
    if (userport_resources_init() < 0) {
        init_resource_fail("userport devices");
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
    if (joyport_rushware_keypad_resources_init() < 0) {
        init_resource_fail("joyport rushware keypad");
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
    if (joyport_inception_resources_init() < 0) {
        init_resource_fail("joyport inception");
        return -1;
    }
    if (joyport_multijoy_resources_init() < 0) {
        init_resource_fail("joyport multijoy");
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
    if (mouse_resources_init() < 0) {
        init_resource_fail("mouse");
        return -1;
    }
    if (mouse_ps2_resources_init() < 0) {
        init_resource_fail("ps2mouse");
        return -1;
    }
#endif
    if (drive_resources_init() < 0) {
        init_resource_fail("drive");
        return -1;
    }
    if (userport_joystick_hummer_resources_init() < 0) {
        init_resource_fail("userport hummer joystick");
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
    if (debugcart_resources_init() < 0) {
        init_resource_fail("debug cart");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    serial_shutdown();
    flash_trap_shutdown();
    flash_trap_resources_shutdown();
    c64dtv_resources_shutdown();
    c64dtvmem_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    joyport_bbrtc_resources_shutdown();
    debugcart_resources_shutdown();
}

/* C64DTV-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (c64dtv_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64dtv");
        return -1;
    }
    if (c64dtvmem_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64dtvmem");
        return -1;
    }
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
        return -1;
    }
    if (sid_cmdline_options_init(SIDTYPE_SIDDTV) < 0) {
        init_cmdline_options_fail("sid");
        return -1;
    }
    if (rs232drv_cmdline_options_init() < 0) {
        init_cmdline_options_fail("rs232drv");
        return -1;
    }
    if (serial_cmdline_options_init() < 0) {
        init_cmdline_options_fail("serial");
        return -1;
    }
    if (flash_trap_cmdline_options_init() < 0) {
        init_cmdline_options_fail("flash trap");
        return -1;
    }
    if (printer_cmdline_options_init() < 0) {
        init_cmdline_options_fail("printer");
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
    if (mouse_ps2_cmdline_options_init() < 0) {
        init_cmdline_options_fail("mouse");
        return -1;
    }
#endif
    if (drive_cmdline_options_init() < 0) {
        init_cmdline_options_fail("drive");
        return -1;
    }
    if (debugcart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("debug cart");
        return -1;
    }
    return 0;
}

static void c64dtv_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502dtv, asm6502, asmR65C02;
    monitor_interface_t *drive_interface_init[NUM_DISK_UNITS];
    monitor_cpu_type_t *asmarray[4];

    asmarray[0] = &asm6502dtv;
    asmarray[1] = &asm6502;
    asmarray[2] = &asmR65C02;
    asmarray[3] = NULL;

    asm6502dtv_init(&asm6502dtv);
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
    cia1_setup_context(&machine_context);
    cia2_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

/* C64DTV-specific initialization.  */
int machine_specific_init(void)
{
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

    gfxoutput_init();

    /* Initialize flash traps.  */
    if (flash_trap_init(c64dtv_flash_traps) < 0) {
        return -1;
    }

    /* Initialize RS232 handler.  */
    rs232drv_init();

    /* Initialize print devices.  */
    printer_init();

    /* Fire up the hardware-level drive emulation.  */
    drive_init();

    disk_image_init();

    /* Initialize autostart.  */
    autostart_init(7, 1);

    /* Pre-init C64DTV-specific parts of the menus before vicii_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        c64dtvui_init_early();
    }

    if (vicii_init(VICII_DTV) == NULL && !console_mode) {
        return -1;
    }

    cia1_init(machine_context.cia1);
    cia2_init(machine_context.cia2);

    /* Initialize the keyboard.  */
    c64keyboard_init();

    c64dtv_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* Initialize native sound chip */
    sid_sound_chip_init();

    drive_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init((unsigned int)machine_timing.cycles_per_sec,
               (unsigned int)machine_timing.cycles_per_rfsh);

    /* Initialize keyboard buffer.  */
    kbdbuf_init(631, 198, 10, (CLOCK)(machine_timing.rfsh_per_sec *                                 machine_timing.cycles_per_rfsh * KBDBUF_ALARM_DELAY));

    /* Initialize the C64DTV-specific part of the UI.  */
    if (!console_mode) {
        c64dtvui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    /* Initialize the C64DTV.  */
    c64dtv_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_ps2_init();
#endif

    c64iec_init();
    c64fastiec_init();

    machine_drive_stub();

    return 0;
}

/* C64DTV-specific reset sequence.  */
void machine_specific_reset(void)
{
    serial_traps_reset();
    flash_traps_reset();

    ciacore_reset(machine_context.cia1);
    ciacore_reset(machine_context.cia2);
    sid_reset();

    rs232drv_reset();

    printer_reset();

    /* FIXME */
    /* reset_reu(); */

    /* The VIC-II must be the *last* to be reset.  */
    vicii_reset();

    drive_reset();
    c64dtvmem_reset();

    sampler_reset();
}

void machine_specific_powerup(void)
{
    vicii_reset_registers();
    userport_powerup();
    joyport_powerup();
}

void machine_specific_shutdown(void)
{
    ciacore_shutdown(machine_context.cia1);
    ciacore_shutdown(machine_context.cia2);

#ifdef HAVE_MOUSE
    mouse_ps2_shutdown();
#endif

    /* close the video chip(s) */
    vicii_shutdown();

    c64dtvmem_shutdown();

    sid_cmdline_options_shutdown();

    if (!console_mode) {
        c64dtvui_shutdown();
    }
}

void machine_handle_pending_alarms(CLOCK num_write_cycles)
{
    vicii_handle_pending_alarms_external(num_write_cycles);
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    network_hook();

    drive_vsync_hook();

    screenshot_record();
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

    *cycle = (unsigned int)(maincpu_clk % machine_timing.cycles_per_line);

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
            machine_timing.power_freq = 0;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C64_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 0;
            break;
        default:
            log_error(c64_log, "Unknown machine timing.");
    }

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec,
                                machine_timing.cycles_per_rfsh);
    debug_set_machine_parameter(machine_timing.cycles_per_line,
                                machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);
    serial_iec_device_set_machine_parameter(machine_timing.cycles_per_sec);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);

    vicii_change_timing(&machine_timing, border_mode);
    cia1_set_timing(machine_context.cia1,
                    (int)machine_timing.cycles_per_sec,
                    (int)machine_timing.power_freq);
    cia2_set_timing(machine_context.cia2,
                    (int)machine_timing.cycles_per_sec,
                    (int)machine_timing.power_freq);

    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks,
                           int event_mode)
{
    int err = c64dtv_snapshot_write(name, save_roms, save_disks, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_WRITE_SNAPSHOT);
    }
    return err;
}

int machine_read_snapshot(const char *name, int event_mode)
{
    int err = c64dtv_snapshot_read(name, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_READ_SNAPSHOT);
    }
    return err;
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

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
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

int machine_addr_in_ram(unsigned int addr)
{
    /* Hack to make autostarting prg files work - the DTV splash screen runs from RAM */
    if (maincpu_clk <= 6817181 && addr >= 0x824 && addr <= 0x884) {
        return 0;
    }
    
#if 0
    /*
     * If autostart stops working on DTV, use this to check if the splash screen is
     * excuting stuff from RAM, in which case modify the above check.
     */
    if (
        addr < 0xe000
            && !(addr >= 0xa000 && addr < 0xc000)
            && !(addr >= 0x0073 && addr <= 0x008a)) {
        log_message(LOG_DEFAULT, "%llu RAM: %x", maincpu_clk, addr);
        return 0;
    }
#endif

    /* NOTE: while the RAM/ROM distinction is more complicated, this is
       sufficient from autostart's perspective */
    return (
        addr < 0xe000
            && !(addr >= 0xa000 && addr < 0xc000)
            && !(addr >= 0x0073 && addr <= 0x008a));
}

const char *machine_get_name(void)
{
    return machine_name;
}

/* ------------------------------------------------------------------------- */

static userport_port_props_t userport_props = {
    0,    /* port does NOT have the pa2 pin */
    0,    /* port does NOT have the pa3 pin */
    NULL, /* port does NOT have the flag pin */
    0,    /* port does NOT have the pc pin */
    0,    /* port does NOT have the cnt1, cnt2 or sp pins */
    0     /* port does NOT have the reset pin */
};

int machine_register_userport(void)
{
    userport_port_register(&userport_props);

    return 0;
}

/* ------------------------------------------------------------------------- */

/** \brief  List of drive type names and ID's supported by C64DTV
 *
 * Convenience function for UI's. This list should be updated whenever drive
 * types are added or removed.
 *
 * XXX: This is here because there is no c64dtvdrive.c.
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
    { DRIVE_NAME_CMDHD, DRIVE_TYPE_CMDHD },
    { NULL, -1 }
};

/** \brief  Get a list of (name, id) tuples for the drives handles by C64DTV
 *
 * Usefull for UI's, get a list of currently supported drive types with a name
 * to display and and ID to use in callbacks.
 *
 * \return  list of drive types, NULL terminated
 *
 * \note    'supported' in this context means the drives C64DTV can support,
 *          not what actually is supported due to ROMs and other settings
 */
drive_type_info_t *machine_drive_get_type_info_list(void)
{
    return drive_type_info_list;
}
