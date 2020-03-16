/*
 * cbm5x0.c
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
#include "cbm2mem.h"
#include "cbm2tpi.h"
#include "cbm2ui.h"
#include "cia.h"
#include "clkguard.h"
#include "datasette.h"
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
#include "paperclip64.h"
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
#include "vice-event.h"
#include "vicii.h"
#include "vicii-resources.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif


/** \brief  Delay in seconds before pasting -keybuf argument into the buffer
 */
#define KBDBUF_ALARM_DELAY   8


machine_context_t machine_context;

const char machine_name[] = "CBM-II";
int machine_class = VICE_MACHINE_CBM5x0;

static void machine_vsync_hook(void);

#define C500_POWERLINE_CYCLES_PER_IRQ (C500_PAL_CYCLES_PER_RFSH)

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

static joyport_port_props_t control_port_1 =
{
    "Control port 1",
    1,                      /* has a potentiometer connected to this port */
    0,                      /* officially has lightpen support on this port,
                               but no lightpen support is in the cbm5x0 code */
    1                       /* port is always active */
};

static joyport_port_props_t control_port_2 =
{
    "Control port 2",
    1,                      /* has a potentiometer connected to this port */
    0,                      /* has NO lightpen support on this port */
    1                       /* port is always active */
};

static int init_joyport_ports(void)
{
    if (joyport_port_register(JOYPORT_1, &control_port_1) < 0) {
        return -1;
    }
    return joyport_port_register(JOYPORT_2, &control_port_2);
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
    if (vicii_resources_init() < 0) {
        init_resource_fail("vicii");
        return -1;
    }
    if (sid_resources_init() < 0) {
        init_resource_fail("sid");
        return -1;
    }
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
    if (acia1_resources_init() < 0) {
        init_resource_fail("acia1");
        return -1;
    }
    if (rs232drv_resources_init() < 0) {
        init_resource_fail("rs232drv");
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
/* FIXME: add lightpen support for xcbm5x0 */
#if 0
    if (lightpen_resources_init() < 0) {
        init_resource_fail("lightpen");
        return -1;
    }
#endif
#endif
    if (debugcart_resources_init() < 0) {
        init_resource_fail("debug cart");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    cbm2_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
    sampler_resources_shutdown();
    cartio_shutdown();
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
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
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
    if (datasette_cmdline_options_init() < 0) {
        init_cmdline_options_fail("datasette");
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
        init_resource_fail("autostart");
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
/* for the C500 there is a powerline IRQ... */

static alarm_t *c500_powerline_clk_alarm = NULL;
static CLOCK c500_powerline_clk = 0;

static void c500_powerline_clk_alarm_handler(CLOCK offset, void *data)
{
    c500_powerline_clk += C500_POWERLINE_CYCLES_PER_IRQ;

    SIGNAL_VERT_BLANK_OFF

    alarm_set(c500_powerline_clk_alarm, c500_powerline_clk);

    SIGNAL_VERT_BLANK_ON
}

static void c500_powerline_clk_overflow_callback(CLOCK sub, void *data)
{
    c500_powerline_clk -= sub;
}


/*
 * C500 extra data (state of 50Hz clk)
 */
#define C500DATA_DUMP_VER_MAJOR   0
#define C500DATA_DUMP_VER_MINOR   0

/*
 * DWORD        IRQCLK          CPU clock ticks until next 50 Hz IRQ
 *
 */

static const char module_name[] = "C500DATA";

int cbm2_c500_snapshot_write_module(snapshot_t *p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(p, module_name, C500DATA_DUMP_VER_MAJOR,
                               C500DATA_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    SMW_DW(m, c500_powerline_clk - maincpu_clk);

    snapshot_module_close(m);

    return 0;
}

int cbm2_c500_snapshot_read_module(snapshot_t *p)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    uint32_t dword;

    m = snapshot_module_open(p, module_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != C500DATA_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_DW(m, &dword);
    c500_powerline_clk = maincpu_clk + dword;
    alarm_set(c500_powerline_clk_alarm, c500_powerline_clk);

    snapshot_module_close(m);

    return 0;
}

/* ------------------------------------------------------------------------- */

static void cbm2_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502;
    monitor_interface_t *drive_interface_init[DRIVE_NUM];
    monitor_cpu_type_t *asmarray[2];

    asmarray[0] = &asm6502;
    asmarray[1] = NULL;

    asm6502_init(&asm6502);

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
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

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();

#if 0
    /* Initialize lightpen support and register VICII callbacks */
    lightpen_init();
    lightpen_register_timing_callback(vicii_lightpen_timing, 0);
    lightpen_register_trigger_callback(vicii_trigger_light_pen);
#endif
#endif

    rs232drv_init();

    /* initialize print devices */
    printer_init();

    /* Pre-init CBM-II-specific parts of the menus before vicii_init()
       creates a canvas window with a menubar at the top. */
    if (!console_mode) {
        cbm5x0ui_init_early();
    }

    if (vicii_init(VICII_STANDARD) == NULL) {
        return -1;
    }
    /*
    c500_set_phi1_bank(15);
    c500_set_phi2_bank(15);
    */

    c500_powerline_clk_alarm = alarm_new(maincpu_alarm_context,
                                         "C500PowerlineClk",
                                         c500_powerline_clk_alarm_handler,
                                         NULL);
    clk_guard_add_callback(maincpu_clk_guard,
                           c500_powerline_clk_overflow_callback, NULL);
    machine_timing.cycles_per_sec = C500_PAL_CYCLES_PER_SEC;
    machine_timing.rfsh_per_sec = C500_PAL_RFSH_PER_SEC;
    machine_timing.cycles_per_rfsh = C500_PAL_CYCLES_PER_RFSH;

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

    drive_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);

    /* Initialize the CBM-II-specific part of the UI.  */
    if (!console_mode) {
        cbm5x0ui_init();
    }

    if (!video_disabled_mode) {
        joystick_init();
    }

    cbm2iec_init();

    machine_drive_stub();

    /* Initialize the CBM5x0-specific I/O */
    cbm5x0io_init();

    return 0;
}

/* CBM-II-specific initialization.  */
void machine_specific_reset(void)
{
    int delay;      /* delay in seconds for the kbduf_init() call */

    ciacore_reset(machine_context.cia1);
    tpicore_reset(machine_context.tpi1);
    tpicore_reset(machine_context.tpi2);
    acia1_reset();

    sid_reset();

    c500_powerline_clk = maincpu_clk + C500_POWERLINE_CYCLES_PER_IRQ;
    alarm_set(c500_powerline_clk_alarm, c500_powerline_clk);
    vicii_reset();

    printer_reset();

    rs232drv_reset();

    drive_reset();
    datasette_reset();

    mem_reset();

    /* Initialize keyboard buffer.
       This appears to work but doesn't account for banking
     */
    switch (ramsize) {
        case 64:
            delay = 8;
            break;
        case 128:
            delay = 12;
            break;
        case 256:
            delay = 20;
            break;
        case 512:
            delay = 31;
            break;
        case 1024:
            delay = 58;
            break;
        default:
            /* invalid ramsize */
            delay = 1;
            break;
    }
    kbdbuf_init(0x03ab, 0x00d1, 10,
            (CLOCK)(machine_timing.rfsh_per_sec *
                machine_timing.cycles_per_rfsh * delay));


    sampler_reset();
}

void machine_specific_powerup(void)
{
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(1);

    ciacore_shutdown(machine_context.cia1);
    tpicore_shutdown(machine_context.tpi1);
    tpicore_shutdown(machine_context.tpi2);

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    /* close the video chip(s) */
    vicii_shutdown();

    if (!console_mode) {
        cbm5x0ui_shutdown();
    }
}

void machine_handle_pending_alarms(int num_write_cycles)
{
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    CLOCK sub;

    drive_vsync_hook();

    autostart_advance();

    screenshot_record();

    sub = clk_guard_prevent_overflow(maincpu_clk_guard);

    /* The drive has to deal both with our overflowing and its own one, so
       it is called even when there is no overflowing in the main CPU.  */
    drive_cpu_prevent_clk_overflow_all(sub);
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
    *line = (unsigned int)((maincpu_clk) / machine_timing.cycles_per_line
                           % machine_timing.screen_lines);

    *cycle = (unsigned int)((maincpu_clk) % machine_timing.cycles_per_line);

    *half_cycle = (int)-1;
}

void machine_change_timing(int timeval, int border_mode)
{
    /* log_message(LOG_DEFAULT, "machine_change_timing_c500 %d", timeval); */

    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = C500_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C500_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C500_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C500_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = C500_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C500_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C500_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C500_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C500_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C500_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        default:
            log_error(LOG_DEFAULT, "Unknown machine timing.");
    }

    debug_set_machine_parameter(machine_timing.cycles_per_line,
                                machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);
#ifdef HAVE_MOUSE
    neos_mouse_set_machine_parameter(machine_timing.cycles_per_sec);
#endif
    clk_guard_set_clk_base(maincpu_clk_guard, machine_timing.cycles_per_rfsh);

    vicii_change_timing(&machine_timing, border_mode);
    cia1_set_timing(machine_context.cia1, machine_timing.cycles_per_sec, machine_timing.power_freq);
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
    if (canvas == vicii_get_canvas()) {
        vicii_screenshot(screenshot);
        return 0;
    }
    return -1;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh,
                                 struct video_canvas_s *canvas)
{
    if (canvas == vicii_get_canvas()) {
        vicii_async_refresh(refresh);
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
    return (addr < 0xe000 && !(addr >= 0x8000 && addr < 0xc000)) ? 1 : 0;
}

const char *machine_get_name(void)
{
    return "CBM-II-5x0";
}
