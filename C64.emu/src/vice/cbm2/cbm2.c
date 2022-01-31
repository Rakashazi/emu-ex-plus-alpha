/*
 * cbm2.c
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include <math.h>   /* modf */
#include <stdio.h>

#include "alarm.h"
#include "attach.h"
#include "autostart.h"
#include "bbrtc.h"
#include "cartio.h"
#include "cartridge.h"
#include "cbm2-cmdline-options.h"
#include "cbm2-resources.h"
#include "cbm2-snapshot.h"
#include "cbm2.h"
#include "cbm2acia.h"
#include "cbm2cia.h"
#include "cbm2iec.h"
#include "cbm2model.h"
#include "cbm2mem.h"
#include "cbm2tpi.h"
#include "cbm2ui.h"
#include "cia.h"
#include "crtc.h"
#include "datasette.h"
#include "datasette-sound.h"
#include "debug.h"
#include "debugcart.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "iecdrive.h"
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
#include "parallel.h"
#include "printer.h"
#include "resources.h"
#include "rs232drv.h"
#include "sampler.h"
#include "sampler2bit.h"
#include "sampler4bit.h"
#include "screenshot.h"
#include "serial.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "snapshot.h"
#include "sound.h"
#include "tape.h"
#include "tapeport.h"
#include "tpi.h"
#include "traps.h"
#include "types.h"
#include "userport.h"
#include "userport_4bit_sampler.h"
#include "userport_8bss.h"
#include "userport_dac.h"
#include "userport_digimax.h"
#include "userport_io_sim.h"
#include "userport_joystick.h"
#include "userport_petscii_snespad.h"
#include "userport_rtc_58321a.h"
#include "userport_rtc_ds1307.h"
#include "userport_superpad64.h"
#include "vice-event.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif

/** \brief  Delay in seconds before pasting -keybuf argument into the buffer
 */
#define KBDBUF_ALARM_DELAY   5


machine_context_t machine_context;

const char machine_name[] = "CBM-II";
int machine_class = VICE_MACHINE_CBM6x0;

static void machine_vsync_hook(void);

/*
static long cbm2_cycles_per_sec = C610_PAL_CYCLES_PER_SEC;
static double cbm2_rfsh_per_sec = C610_PAL_RFSH_PER_SEC;
static long cbm2_cycles_per_rfsh = C610_PAL_CYCLES_PER_RFSH;
*/

static log_t cbm2_log = LOG_ERR;
static machine_timing_t machine_timing;

/* FIXME: add different keyboard types */
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

static joyport_port_props_t joy_adapter_control_port_1 =
{
    "Joystick adapter port 1",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_2 =
{
    "Joystick adapter port 2",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_3 =
{
    "Joystick adapter port 3",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_4 =
{
    "Joystick adapter port 4",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_5 =
{
    "Joystick adapter port 5",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_6 =
{
    "Joystick adapter port 6",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_7 =
{
    "Joystick adapter port 7",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_8 =
{
    "Joystick adapter port 8",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0                       /* port can be switched on/off */
};

static int init_joyport_ports(void)
{
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

/* CBM-II-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0) {
        init_resource_fail("traps");
        return -1;
    }
    if (cbm2_resources_init() < 0) {
        init_resource_fail("cbm2");
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
    if (crtc_resources_init() < 0) {
        init_resource_fail("crtc");
        return -1;
    }
    if (sid_resources_init() < 0) {
        init_resource_fail("sid");
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
    if (tapeport_resources_init(1) < 0) {
        init_resource_fail("tapeport");
        return -1;
    }
    if (acia1_resources_init() < 0) {
        init_resource_fail("acia1");
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
    if (joystick_resources_init() < 0) {
        init_resource_fail("joystick");
        return -1;
    }
    if (sampler_resources_init() < 0) {
        init_resource_fail("samplerdrv");
        return -1;
    }
    if (gfxoutput_resources_init() < 0) {
        init_resource_fail("gfxoutput");
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
#endif
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
    if (userport_digimax_resources_init() < 0) {
        init_resource_fail("userport digimax");
        return -1;
    }
    if (userport_petscii_snespad_resources_init() < 0) {
        init_resource_fail("userport petscii snes pad");
        return -1;
    }
    if (userport_superpad64_resources_init() < 0) {
        init_resource_fail("userport userpad64");
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
    if (userport_io_sim_resources_init() < 0) {
        init_resource_fail("userport I/O simulation");
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
    cbm2_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    cartio_shutdown();
    userport_rtc_58321a_resources_shutdown();
    userport_rtc_ds1307_resources_shutdown();
    joyport_bbrtc_resources_shutdown();
    tapeport_resources_shutdown();
    debugcart_resources_shutdown();
    cartridge_resources_shutdown();
}

/* CBM-II-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (cbm2_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cbm2");
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
    if (crtc_cmdline_options_init() < 0) {
        init_cmdline_options_fail("crtc");
        return -1;
    }
    if (sid_cmdline_options_init(SIDTYPE_SID) < 0) {
        init_cmdline_options_fail("sid");
        return -1;
    }
    if (drive_cmdline_options_init() < 0) {
        init_cmdline_options_fail("drive");
        return -1;
    }
    if (tapeport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("tapeport");
        return -1;
    }
    if (acia1_cmdline_options_init() < 0) {
        init_cmdline_options_fail("acia1");
        return -1;
    }
    if (rs232drv_cmdline_options_init() < 0) {
        init_cmdline_options_fail("rs232drv");
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
    if (userport_rtc_58321a_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport rtc (58321a)");
        return -1;
    }
    if (userport_rtc_ds1307_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport rtc (ds1307)");
        return -1;
    }
    if (debugcart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("debug cart");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* provide the 50(?)Hz IRQ signal for the standard IRQ */

#define SIGNAL_VERT_BLANK_OFF tpicore_set_int(machine_context.tpi1, 0, 1);

#define SIGNAL_VERT_BLANK_ON  tpicore_set_int(machine_context.tpi1, 0, 0);

/* ------------------------------------------------------------------------- */
/* ... while the other CBM-II use the CRTC retrace signal. */

static void cbm2_crtc_signal(unsigned int signal)
{
    if (signal) {
        SIGNAL_VERT_BLANK_ON
    } else {
        SIGNAL_VERT_BLANK_OFF
    }
}

/* ------------------------------------------------------------------------- */

static void cbm2_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502;
    monitor_interface_t *drive_interface_init[NUM_DISK_UNITS];
    monitor_cpu_type_t *asmarray[2];

    asmarray[0] = &asm6502;
    asmarray[1] = NULL;

    asm6502_init(&asm6502);

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
    tpi1_setup_context(&machine_context);
    tpi2_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

/* CBM-II-specific initialization.  */
int machine_specific_init(void)
{
    cbm2_log = log_open("CBM2");

    cbm2_init_ok = 1;

    event_init();

    /* Setup trap handling - must be before mem_load() */
    traps_init();

    if (mem_load() < 0) {
        return -1;
    }

    gfxoutput_init();

    rs232drv_init();

    /* initialize print devices */
    printer_init();

    /* Pre-init CBM-II-specific parts of the menus before crtc_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        cbm2ui_init_early();
    }

    if (crtc_init() == NULL) {
        return -1;
    }
    crtc_set_retrace_callback(cbm2_crtc_signal);
    crtc_set_retrace_type(0);
    crtc_set_hw_options(1, 0x7ff, 0x1000, 512, -0x2000);

    cia1_init(machine_context.cia1);
    acia1_init();
    tpi1_init(machine_context.tpi1);
    tpi2_init(machine_context.tpi2);

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level 1541 emulation.  */
    drive_init();

    disk_image_init();

    cbm2_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* Initialize native sound chip */
    sid_sound_chip_init();

    /* Initialize userport based sound chips */
    userport_dac_sound_chip_init();
    userport_digimax_sound_chip_init();

    drive_sound_init();
    datasette_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init((unsigned int)machine_timing.cycles_per_sec,
               (unsigned int)machine_timing.cycles_per_rfsh);

    /* Initialize the CBM-II-specific part of the UI.  */
    if (!console_mode) {
        cbm2ui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    cbm2iec_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();
#endif

    machine_drive_stub();

    /* Initialize the CBM2-specific I/O */
    cbm2io_init();

    return 0;
}

/* CBM-II-specific initialization.  */
void machine_specific_reset(void)
{
    int delay = KBDBUF_ALARM_DELAY;
    int model = cbm2model_get();

    ciacore_reset(machine_context.cia1);
    tpicore_reset(machine_context.tpi1);
    tpicore_reset(machine_context.tpi2);
    acia1_reset();

    sid_reset();

    crtc_reset();

    printer_reset();

    rs232drv_reset();

    drive_reset();
    datasette_reset();

    mem_reset();

    /* delays figured out by running each model */
    /* printf("cbm2model: %d\n", model); */
    switch (model) {
        /* Most likely unneeded, since cbm5x0 should handle these. But should
         * someone be clever enough to turn the #define's in cmb2model.h into
         * an enum, a compiler could catch missing cases.
         */
        case CBM2MODEL_510_PAL:     /* fallthrough */
        case CBM2MODEL_510_NTSC:
            delay = 7;  /* only valid for the default 64KB RAM */
            break;

        /* 610: default RAM: 128KB */
        case CBM2MODEL_610_PAL:     /* fallthrough */
        case CBM2MODEL_610_NTSC:
            delay = 4;
            break;

        /* 620: default RAM: 256KB */
        case CBM2MODEL_620_PAL:     /* fallthrough */
        case CBM2MODEL_620_NTSC:
            delay = 8;
            break;

        case CBM2MODEL_620PLUS_PAL: /* fallthrough */
        case CBM2MODEL_620PLUS_NTSC:
            delay = 25;
            break;
        case CBM2MODEL_710_NTSC:
            delay = 4;
            break;
        case CBM2MODEL_720_NTSC:
            delay = 25;
            break;
        case CBM2MODEL_720PLUS_NTSC:
            delay = 25;
            break;

        /* When the RAM set via -ramsize doesn't match the RAM size for the
         * selected model in the table in cbm2model.h, the model is set
         * to CBM2MODEL_UNKNOWN (99) */
        default:
           switch (ramsize) {
                case 128:
                    delay = 4;
                    break;
                case 256:
                    delay = 8;
                    break;
                case 512:
                    delay = 13;
                    break;
                case 1024:
                    delay = 30;
                    break;
                default:
                    delay = 30;     /* shouldn't get here */
            }
            break;
    }

    /* Initialize keyboard buffer.
       This appears to work but doesn't account for banking. */
    /* printf("init kbdbuf with %d seconds delay\n", delay); */
    kbdbuf_init(0x03ab, 0x00d1, 10,
            (CLOCK)(machine_timing.rfsh_per_sec *
                machine_timing.cycles_per_rfsh * delay));

    sampler_reset();
}

void machine_specific_powerup(void)
{
    userport_powerup();
    tapeport_powerup();
    joyport_powerup();
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(TAPEPORT_PORT_1 + 1);

    ciacore_shutdown(machine_context.cia1);
    tpicore_shutdown(machine_context.tpi1);
    tpicore_shutdown(machine_context.tpi2);

    /* close the video chip(s) */
    crtc_shutdown();

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    if (!console_mode) {
        cbm2ui_shutdown();
    }
}

void machine_handle_pending_alarms(CLOCK num_write_cycles)
{
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    drive_vsync_hook();

    screenshot_record();
}

/* Dummy - no restore key.  */
void machine_set_restore_key(int v)
{
}

int machine_has_restore_key(void)
{
    return 0;
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

    *cycle = ((maincpu_clk) % machine_timing.cycles_per_line);

    *half_cycle = (int)-1;
}

void machine_change_timing(int timeval, int border_mode)
{
    /* log_message(LOG_DEFAULT, "machine_change_timing_c610 %d", timeval); */

    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = C610_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C610_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C610_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C610_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = C610_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C610_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C610_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C610_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C610_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C610_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        default:
            log_error(LOG_DEFAULT, "Unknown machine timing.");
    }

    debug_set_machine_parameter(machine_timing.cycles_per_line,
                                machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);

    cia1_set_timing(machine_context.cia1,
                    (int)machine_timing.cycles_per_sec,
                    machine_timing.power_freq);
}

/* Set the screen refresh rate, as this is variable in the CRTC */
void machine_set_cycles_per_frame(long cpf)
{
    double i, f;

    machine_timing.cycles_per_rfsh = cpf;
    machine_timing.rfsh_per_sec = ((double)machine_timing.cycles_per_sec)
                                  / ((double)cpf);

    f = modf(machine_timing.rfsh_per_sec, &i) * 1000;
    log_message(cbm2_log, "cycles per frame set to %ld, refresh to %d.%03dHz",
                cpf, (int)i, (int)f);

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks,
                           int event_mode)
{
    int err = cbm2_snapshot_write(name, save_roms, save_disks, event_mode);
    if ((err < 0) && (snapshot_get_error() == SNAPSHOT_NO_ERROR)) {
        snapshot_set_error(SNAPSHOT_CANNOT_WRITE_SNAPSHOT);
    }
    return err;
}

int machine_read_snapshot(const char *name, int event_mode)
{
    int err = cbm2_snapshot_read(name, event_mode);
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
    if (canvas == crtc_get_canvas()) {
        crtc_screenshot(screenshot);
        return 0;
    }

    return -1;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
{
    if (canvas == crtc_get_canvas()) {
        crtc_async_refresh(refresh);
        return 0;
    }
    return -1;
}

/*-----------------------------------------------------------------------*/

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return NULL;
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
    /* FIXME: handle the banking */
    
    if (addr >= 0x25a && addr <= 0x25d) {
        /* 'Pickup subroutine' */
        return 0;
    }
    
    if (addr >= 0x8000 && addr < 0xc000) {
        return 0;
    }
    
    if (addr > 0xe000) {
        return 0;
    }
    
    return 1;
}

const char *machine_get_name(void)
{
    return machine_name;
}

/* ------------------------------------------------------------------------- */

/* native screenshot support */

uint8_t *crtc_get_active_bitmap(void)
{
    return NULL;
}

/* ------------------------------------------------------------------------- */

static void cbm2_userport_set_flag(uint8_t b)
{
    if (b != 0) {
        ciacore_set_flag(machine_context.cia1);
    }
}

static userport_port_props_t userport_props = {
    1,                      /* port has the pa2 pin */
    1,                      /* port has the pa3 pin */
    cbm2_userport_set_flag, /* port has the flag pin, set flag function */
    1,                      /* port has the pc pin */
    0,                      /* port does NOT have the cnt1, cnt2 or sp pins */
    0                       /* port does NOT have the reset pin */
};

int machine_register_userport(void)
{
    userport_port_register(&userport_props);

    return 0;
}
