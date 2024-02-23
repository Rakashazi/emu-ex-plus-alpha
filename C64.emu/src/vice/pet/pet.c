/*
 * pet.c
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <stdlib.h>

#include "attach.h"
#include "autostart.h"
#include "bbrtc.h"
#include "cartio.h"
#include "crtc-mem.h"
#include "crtc.h"
#include "datasette.h"
#include "datasette-sound.h"
#include "debug.h"
#include "debugcart.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-snapshot.h"
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
#include "pet-cmdline-options.h"
#include "pet-resources.h"
#include "pet-snapshot.h"
#include "pet.h"
#include "petcolour.h"
#include "petiec.h"
#include "petmem.h"
#include "petreu.h"
#include "petdww.h"
#include "pethre.h"
#include "pets.h"
#include "petsound.h"
#include "petui.h"
#include "petvia.h"
#include "petacia.h"
#include "petpia.h"
#include "printer.h"
#include "resources.h"
#include "rs232drv.h"
#include "sampler.h"
#include "sampler2bit.h"
#include "sampler4bit.h"
#include "screenshot.h"
#include "sid.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sidcart.h"
#include "sound.h"
#include "tape.h"
#include "tapeport.h"
#include "traps.h"
#include "types.h"
#include "userport.h"
#include "userport_dac.h"
#include "userport_hummer_joystick.h"
#include "userport_io_sim.h"
#include "userport_joystick.h"
#include "userport_petscii_snespad.h"
#include "userport_rtc_58321a.h"
#include "userport_rtc_ds1307.h"
#include "userport_spt_joystick.h"
#include "userport_synergy_joystick.h"
#include "userport_woj_joystick.h"
#include "util.h"
#include "via.h"
#include "vice-event.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif

machine_context_t machine_context;

const char machine_name[] = "PET";
int machine_class = VICE_MACHINE_PET;

static void machine_vsync_hook(void);

/*
static long     pet_cycles_per_rfsh     = PET_PAL_CYCLES_PER_RFSH;
static double   pet_rfsh_per_sec        = PET_PAL_RFSH_PER_SEC;
*/

static log_t pet_log = LOG_ERR;
static machine_timing_t machine_timing;

int machine_get_keyboard_type(void)
{
    int type;
    if (resources_get_int("KeyboardType", &type) < 0) {
        return 0;
    }
    return type;
}

char *machine_get_keyboard_type_name(int type)
{
    static char names[KBD_TYPE_NUM][5] = {
        KBD_TYPE_STR_BUSINESS_UK,
        KBD_TYPE_STR_BUSINESS_US,
        KBD_TYPE_STR_BUSINESS_DE,
        KBD_TYPE_STR_BUSINESS_JP,
        KBD_TYPE_STR_GRAPHICS_US
    };
    return names[type]; /* return 0 if no different types exist */
}

/* return number of available keyboard types for this machine */
int machine_get_num_keyboard_types(void)
{
    return KBD_TYPE_NUM;
}

/* FIXME: adjust this to reality :) */
static kbdtype_info_t kbdinfo[KBD_TYPE_NUM + 1] = {
    { "Business (uk)", KBD_TYPE_BUSINESS_UK, 0 },
    { "Business (us)", KBD_TYPE_BUSINESS_US, 0 },
    { "Business (de)", KBD_TYPE_BUSINESS_DE, 0 },
    { "Business (jp)", KBD_TYPE_BUSINESS_JP, 0 },
    { "Graphics (us)", KBD_TYPE_GRAPHICS_US, 0 },
    { NULL, 0, 0 }
};

kbdtype_info_t *machine_get_keyboard_info_list(void)
{
    return kbdinfo;
}

/* ------------------------------------------------------------------------ */

static joyport_port_props_t joy_adapter_control_port_1 =
{
    "Joystick adapter port 1",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_2 =
{
    "Joystick adapter port 2",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_3 =
{
    "Joystick adapter port 3",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_4 =
{
    "Joystick adapter port 4",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_5 =
{
    "Joystick adapter port 5",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_6 =
{
    "Joystick adapter port 6",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_7 =
{
    "Joystick adapter port 7",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
    0                       /* port can be switched on/off */
};

static joyport_port_props_t joy_adapter_control_port_8 =
{
    "Joystick adapter port 8",
    0,                      /* has NO potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    0,                      /* has NO joystick adapter on this port */
    1,                      /* has output support on this port */
    0,                      /* default for joystick adapter ports is NO +5vdc line on this port, can be changed by the joystick adapter when activated */
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

/* PET-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (traps_resources_init() < 0) {
        init_resource_fail("traps");
        return -1;
    }
    if (pet_resources_init() < 0) {
        init_resource_fail("pet");
        return -1;
    }
    if (cartio_resources_init() < 0) {
        init_resource_fail("cartio");
        return -1;
    }
    if (petreu_resources_init() < 0) {
        init_resource_fail("petreu");
        return -1;
    }
    if (pia1_resources_init() < 0) {
        init_resource_fail("pia1");
        return -1;
    }
    if (crtc_resources_init() < 0) {
        init_resource_fail("crtc");
        return -1;
    }
    if (petdww_resources_init() < 0) {
        init_resource_fail("petdww");
        return -1;
    }
    if (pethre_resources_init() < 0) {
        init_resource_fail("pethre");
        return -1;
    }
    if (sidcart_resources_init() < 0) {
        init_resource_fail("sidcart");
        return -1;
    }
    if (drive_resources_init() < 0) {
        init_resource_fail("drive");
        return -1;
    }
    if (tapeport_resources_init(2) < 0) {
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
    if (joystick_resources_init() < 0) {
        init_resource_fail("joystick");
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
    if (userport_joystick_woj_resources_init() < 0) {
        init_resource_fail("userport woj joystick");
        return -1;
    }
    if (userport_spt_joystick_resources_init() < 0) {
        init_resource_fail("userport stupid pet tricks joystick");
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
    if (debugcart_resources_init() < 0) {
        init_resource_fail("debug cart");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    petdww_resources_shutdown();
    pethre_resources_shutdown();
    pet_resources_shutdown();
    petreu_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    cartio_shutdown();
    userport_rtc_58321a_resources_shutdown();
    userport_rtc_ds1307_resources_shutdown();
    tapeport_resources_shutdown();
    debugcart_resources_shutdown();
    joyport_resources_shutdown();
}

/* PET-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (pet_cmdline_options_init() < 0) {
        init_cmdline_options_fail("pet");
        return -1;
    }
    if (cartio_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartio");
        return -1;
    }
    if (petreu_cmdline_options_init() < 0) {
        init_cmdline_options_fail("petreu");
        return -1;
    }
    if (crtc_cmdline_options_init() < 0) {
        init_cmdline_options_fail("crtc");
        return -1;
    }
    if (petdww_cmdline_options_init() < 0) {
        init_cmdline_options_fail("petdww");
        return -1;
    }
    if (pethre_cmdline_options_init() < 0) {
        init_cmdline_options_fail("pethre");
        return -1;
    }
    if (pia1_cmdline_options_init() < 0) {
        init_cmdline_options_fail("pia1");
        return -1;
    }
    if (sidcart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("sidcart");
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
    if (joystick_cmdline_options_init() < 0) {
        init_cmdline_options_fail("joystick");
        return -1;
    }
    if (userport_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport");
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

#define SIGNAL_VERT_BLANK_OFF   pia1_signal(PIA_SIG_CB1, PIA_SIG_RISE, offset);

#define SIGNAL_VERT_BLANK_ON    pia1_signal(PIA_SIG_CB1, PIA_SIG_FALL, offset);

static void pet_crtc_signal(unsigned int signal, CLOCK offset)
{
    if (signal) {
        SIGNAL_VERT_BLANK_ON
    } else {
        SIGNAL_VERT_BLANK_OFF
    }
}

/* ------------------------------------------------------------------------- */

void machine_handle_pending_alarms(CLOCK num_write_cycles)
{
}

static void pet_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502;
    monitor_cpu_type_t asm6809;
    monitor_interface_t *drive_interface_init[NUM_DISK_UNITS];
    monitor_cpu_type_t *asmarray[3];

    asmarray[0] = &asm6502;
    asmarray[1] = &asm6809;
    asmarray[2] = NULL;

    asm6502_init(&asm6502);
    asm6809_init(&asm6809);

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        drive_interface_init[dnr] = drive_cpu_monitor_interface_get(dnr);
    }

    /* Initialize the monitor.  */
    monitor_init(maincpu_monitor_interface_get(), drive_interface_init,
                 asmarray);
}

void machine_setup_context(void)
{
    petvia_setup_context(&machine_context);
    machine_printer_setup_context(&machine_context);
}

/* PET-specific initialization.  */
int machine_specific_init(void)
{
    pet_log = log_open("PET");

    pet_init_ok = 1;    /* used in petmodel_set() */

    event_init();

    /* Setup trap handling - must be before mem_load() */
    traps_init();

    if (mem_load() < 0) {
        return -1;
    }

    gfxoutput_init();

    log_message(pet_log, "Initializing IEEE488 bus...");

    rs232drv_init();

    /* initialize print devices */
    printer_init();

    /* Pre-init PET-specific parts of the menus before crtc_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        petui_init_early();
    }

    /* Initialize the CRTC emulation.  */
    if (crtc_init() == NULL) {
        return -1;
    }

    crtc_set_retrace_type(petres.model.crtc);
    crtc_set_retrace_callback(pet_crtc_signal);
    pet_crtc_set_screen();
    petcolour_init();

    via_init(machine_context.via);
    pia1_init();
    pia2_init();
    acia1_init();

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level 1541 emulation.  */
    drive_init();

    disk_image_init();

    pet_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* Initialize the sidcart first */
    sidcart_sound_chip_init();

    /* Initialize native sound chip */
    pet_sound_chip_init();

    /* Initialize userport based sound chips */
    userport_dac_sound_chip_init();

    drive_sound_init();
    datasette_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init((unsigned int)machine_timing.cycles_per_sec,
               (unsigned int)machine_timing.cycles_per_rfsh);

    /* Initialize keyboard buffer.  FIXME: Is this correct?  */
    /* moved to mem_load() because it's model specific... AF 30jun1998
    kbdbuf_init(631, 198, 10, PET_PAL_CYCLES_PER_RFSH * PET_PAL_RFSH_PER_SEC);
    */

    /* Initialize the PET-specific part of the UI.  */
    if (!console_mode) {
        petui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    /* Initialize the PET Ram and Expansion Unit. */
    petreu_init();

    /* Initialize the PET Double-W Hi-Res graphics card. */
    petdww_init();

    /* Initialize the PET Hi-Res Emulator graphics card. */
    pethre_init();

    petiec_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();
#endif

    machine_drive_stub();

    return 0;
}

void machine_specific_powerup(void)
{
    petdww_powerup();
    pethre_powerup();
    userport_powerup();
    tapeport_powerup();
    joyport_powerup();
}

/* PET-specific initialization.  */
void machine_specific_reset(void)
{
    pia1_reset();
    pia2_reset();
    viacore_reset(machine_context.via);
    acia1_reset();
    crtc_reset();
    sid_reset();
    petmem_reset();
    rs232drv_reset();
    printer_reset();
    drive_reset();
    datasette_reset();
    petreu_reset();
    petdww_reset();
    pethre_reset();
    sampler_reset();
}

void machine_specific_shutdown(void)
{
    int i;

    /* and the tape */
    for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
        tape_image_detach_internal(i + 1);
    }

    viacore_shutdown(machine_context.via);

    /* close the video chip(s) */
    petdww_shutdown();
    pethre_shutdown();
    crtc_shutdown();

    petreu_shutdown();

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    sidcart_cmdline_options_shutdown();

    if (!console_mode) {
        petui_shutdown();
    }
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

    *cycle = (unsigned int)((maincpu_clk) % machine_timing.cycles_per_line);

    *half_cycle = (int)-1;
}

void machine_change_timing(int timeval, int powerfreq, int border_mode)
{
    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = PET_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = PET_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = PET_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = PET_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = PET_PAL_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = PET_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = PET_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = PET_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = PET_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = PET_NTSC_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        default:
            log_error(pet_log, "Unknown machine timing.");
    }

    debug_set_machine_parameter(machine_timing.cycles_per_line, machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);

    /* Should these be called also? */
#if 0
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);
#endif

    machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
}

/* Set the screen refresh rate, as this is variable in the CRTC */
void machine_set_cycles_per_frame(long cpf)
{
    double i, f;

    machine_timing.cycles_per_rfsh = cpf;
    machine_timing.rfsh_per_sec = ((double)machine_timing.cycles_per_sec)
                                  / ((double)cpf);

    f = modf(machine_timing.rfsh_per_sec, &i) * 1000;

    log_message(pet_log, "cycles per frame set to %ld, refresh to %d.%03dHz",
                cpf, (int)i, (int)f);

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec,
                                machine_timing.cycles_per_sec);

    /* sound_set_cycles_per_rfsh(machine_timing.cycles_per_rfsh); */
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks,
                           int event_mode)
{
    return pet_snapshot_write(name, save_roms, save_disks, event_mode);
}

int machine_read_snapshot(const char *name, int event_mode)
{
    return pet_snapshot_read(name, event_mode);
}


/* ------------------------------------------------------------------------- */

int machine_autodetect_psid(const char *name)
{
    return -1;
}


/* ------------------------------------------------------------------------- */

void pet_crtc_set_screen(void)
{
    int cols, vmask;

    cols = petres.model.video;
    vmask = petres.vmask;

    /* mem_initialize_memory(); */

    if (!cols) {
        cols = petres.rom_video;
        if (!cols) {
            cols = PET_COLS;
        }
        vmask = (cols == 40) ? 0x3ff : 0x7ff;
    }

    /* when switching 8296 to 40 columns, CRTC ends up at $9000 otherwise...*/
    if (cols == 40) {
        vmask = 0x3ff;
    }
/*
    log_message(pet_mem_log, "set_screen(vmask=%04x, cols=%d, crtc=%d)",
                vmask, cols, petres.model.crtc);
*/
    int hwflag = (cols == 80) ? CRTC_HW_DOUBLE_CHARS : 0;
    /* Note: see bug #1954.
     * The real cause for the timing difference is unknown so far.
     * If found, the condition below will probably change. */
    if (cols == 80 && petres.map != PET_MAP_8296) {
        hwflag |= CRTC_HW_LATE_BEAM;
    }
    crtc_set_screen_options(cols, 25 * 10);
    crtc_set_screen_addr(mem_ram + 0x8000);
    crtc_set_hw_options(hwflag,
                        vmask,
                        0x2000,         /* vchar: MA13 */
                        512,            /* vcoffset */
                        0x1000);        /* vrevmask: MA12 */
    crtc_set_retrace_type(petres.model.crtc ? CRTC_RETRACE_TYPE_CRTC
                                            : CRTC_RETRACE_TYPE_DISCRETE);

    /* No CRTC -> assume 40 columns and 60 Hz */
    if (!petres.model.crtc) {
        static uint8_t crtc_values[14] = {
            /*
             * Set the CRTC to display 60 frames per second.
             *
             * Tuned specifically for 64 clocks (= chars) per scanline,
             * for the Cursor #18 Hi-Res program.
             * Unless CRTC_BEAM_RACING is enabled, the exact time of the IRQ is
             * not 100% right, but close enough to get a visual effect.
             *
             * 15625 Hz horizontal
             * PET: cycles per frame set to 16640, refresh to 60.096Hz
             *
             * Additional note: new ROMs 99/9A count to 623:
             * 0099-009A        Jiffy clock correction: 623rd 1/60 sec
             *                  does not increment time
             *
             * Presumably this should correct the frequency which is
             * slightly over 60 Hz: 60.096 * 622 / 623 = 59.99954.
             *
             * However, 60.096 * 625 / 626 = 60.0000...
             * so it could have done even better.
             *
             * Since we use the CRTC, a raster line is considered to start at
             * the left edge of the visible characters. So the right border,
             * horizontal retrace, and left border are at the end of the line.
             * Likewise, scan line 0 is the first text scan line.
             * There are 200 text lines, 20 bottom border, 20 vertical retrace
             * and 20 top border lines.
             * The "off-screen" signal, which triggers the IRQ, goes on just
             * after the last text character, and comes on 3*20 lines later, at
             * the same place in line -1 (259).
             */
              63, /* R0 total horizontal characters - 1 */
              40, /* R1 displayed horizontal characters */
              50, /* R2 horizontal sync position */
            (0 << 4)|8, /* R3 vertical / horizontal sync width */
              31, /* R4 total vertical characters - 1 */
               4, /* R5 total vertical lines adjustment */
              25, /* R6 displayed vertical characters */
              29, /* R7 vertical sync position */
               0, /* R8 MODECTRL */
               7, /* R9 scanlines per character row - 1, including spacing */
               0, /* R10 CURSORSTART */
               0, /* R11 CURSOREND */
            0x10, /* R12 DISPSTARTH */
            0x00, /* R13 DISPSTARTL */
        };
        int r;

        for (r = 13; r >= 0; r--) {
            crtc_store(0, r);
            crtc_store(1, crtc_values[r]);
        }
    }
}

int machine_screenshot(screenshot_t *screenshot, struct video_canvas_s *canvas)
{
    if (canvas != crtc_get_canvas()) {
        return -1;
    }

    crtc_screenshot(screenshot);
    return 0;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
{
    if (canvas != crtc_get_canvas()) {
        return -1;
    }

    crtc_async_refresh(refresh);
    return 0;
}

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
    /* Exclude both possible locations of the CHRGET routine */
    if (addr >= 0x0070 && addr < 0x0088) {
        return 0;
    }

    /* the old ROM location */
    if (addr >= 0x00c2 && addr < 0x00da) {
        return 0;
    }

    return addr < 0xb000;
}

const char *machine_get_name(void)
{
    return machine_name;
}

/* ------------------------------------------------------------------------- */
/* native screenshot support */

uint8_t *crtc_get_active_bitmap(void)
{
    uint8_t *retval = NULL;

    retval = petdww_crtc_get_active_bitmap();

    /* left open for future expansion of boards with their own ram */

    return retval;
}

/* ------------------------------------------------------------------------- */

static void pet_userport_set_flag(uint8_t b)
{
    viacore_signal(machine_context.via, VIA_SIG_CA1, b ? VIA_SIG_RISE : VIA_SIG_FALL);
}

static userport_port_props_t userport_props = {
    1,                     /* port has the pa2 pin */
    0,                     /* port does NOT have the pa3 pin */
    pet_userport_set_flag, /* port has the flag pin, set flag function */
    0,                     /* port does NOT have the pc pin */
    0,                     /* port does NOT have the cnt1, cnt2 or sp pins */
    0                      /* port does NOT have the reset pin */
};

int machine_register_userport(void)
{
    userport_port_register(&userport_props);

    return 0;
}
