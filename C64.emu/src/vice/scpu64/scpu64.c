/*
 * scpu64.c
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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
#include "c64cart.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cia.h"
#include "c64export.h"
#include "c64fastiec.h"
#include "scpu64gluelogic.h"
#include "c64iec.h"
#include "c64keyboard.h"
#include "scpu64mem.h"
#include "c64rsuser.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "clkguard.h"
#include "debug.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "imagecontents.h"
#include "init.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "lightpen.h"
#include "log.h"
#include "machine-drive.h"
#include "machine-printer.h"
#include "machine-video.h"
#include "machine.h"
#include "main65816cpu.h"
#include "mem.h"
#include "monitor.h"
#include "network.h"
#include "parallel.h"
#include "patchrom.h"
#include "printer.h"
#include "resources.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "scpu64-cmdline-options.h"
#include "scpu64-resources.h"
#include "scpu64-snapshot.h"
#include "scpu64.h"
#include "scpu64cpu.h"
#include "scpu64ui.h"
#include "screenshot.h"
#include "serial.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "sound.h"
#include "traps.h"
#include "types.h"
#include "userport_joystick.h"
#include "userport_rtc.h"
#include "vice-event.h"
#include "vicii.h"
#include "vicii-mem.h"
#include "video.h"
#include "video-sound.h"
#include "vsync.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif

machine_context_t machine_context;

const char machine_name[] = "SCPU64";

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

static const trap_t scpu64_serial_traps[] = {
    { "SerialListen", 0xED24, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_attention, scpu64_trap_read, scpu64_trap_store },
    { "SerialSaListen", 0xED37, 0xEDAB, { 0x20, 0x8E, 0xEE }, serial_trap_attention, scpu64_trap_read, scpu64_trap_store },
    { "SerialSendByte", 0xED41, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_send, scpu64_trap_read, scpu64_trap_store },
    { "SerialReceiveByte", 0xEE14, 0xEDAB, { 0xA9, 0x00, 0x85 }, serial_trap_receive, scpu64_trap_read, scpu64_trap_store },
    { "SerialReady", 0xEEA9, 0xEDAB, { 0xAD, 0x00, 0xDD }, serial_trap_ready, scpu64_trap_read, scpu64_trap_store },
    { NULL, 0, 0, { 0, 0, 0 }, NULL, NULL, NULL }
};

static log_t scpu64_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

static io_source_t vicii_d000_device = {
    "VIC-II",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd000, 0xd0ff, 0x3f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d100_device = {
    "VIC-II $D100-$D1FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd100, 0xd1ff, 0x3f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t sid_d400_device = {
    "SID",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd400, 0xd41f, 0x1f,
    1, /* read is always valid */
    sid_store,
    sid_read,
    sid_peek,
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t sid_d420_device = {
    "SID $D420-$D4FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd420, 0xd4ff, 0x1f,
    1, /* read is always valid */
    sid_store,
    sid_read,
    sid_peek,
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_LOW, /* low priority, device and mirrors never involved in collisions */
    0
};

static io_source_t sid_d500_device = {
    "SID $D500-$D5FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd500, 0xd5ff, 0x1f,
    1, /* read is always valid */
    sid_store,
    sid_read,
    sid_peek,
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_LOW, /* low priority, device and mirrors never involved in collisions */
    0
};

static io_source_t sid_d600_device = {
    "SID $D600-$D6FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd600, 0xd6ff, 0x1f,
    1, /* read is always valid */
    sid_store,
    sid_read,
    sid_peek,
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_LOW, /* low priority, device and mirrors never involved in collisions */
    0
};

static io_source_t sid_d700_device = {
    "SID $D700-$D7FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd700, 0xd7ff, 0x1f,
    1, /* read is always valid */
    sid_store,
    sid_read,
    sid_peek,
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_LOW, /* low priority, device and mirrors never involved in collisions */
    0
};

static io_source_list_t *vicii_d000_list_item = NULL;
static io_source_list_t *vicii_d100_list_item = NULL;
static io_source_list_t *sid_d400_list_item = NULL;
static io_source_list_t *sid_d420_list_item = NULL;
static io_source_list_t *sid_d500_list_item = NULL;
static io_source_list_t *sid_d600_list_item = NULL;
static io_source_list_t *sid_d700_list_item = NULL;

void c64io_vicii_init(void)
{
    vicii_d000_list_item = io_source_register(&vicii_d000_device);
    vicii_d100_list_item = io_source_register(&vicii_d100_device);
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
    if (scpu64_resources_init() < 0) {
        init_resource_fail("scpu64");
        return -1;
    }
    if (c64export_resources_init() < 0) {
        init_resource_fail("c64export");
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
    if (joystick_resources_init() < 0) {
        init_resource_fail("joystick");
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
    if (lightpen_resources_init() < 0) {
        init_resource_fail("lightpen");
        return -1;
    }
#endif
#ifndef COMMON_KBD
    if (kbd_resources_init() < 0) {
        init_resource_fail("kbd");
        return -1;
    }
#endif
    if (drive_resources_init() < 0) {
        init_resource_fail("drive");
        return -1;
    }
    if (scpu64_glue_resources_init() < 0) {
        init_resource_fail("scpu64 glue");
        return -1;
    }
    if (userport_joystick_resources_init() < 0) {
        init_resource_fail("userport joystick");
        return -1;
    }
    if (userport_rtc_resources_init() < 0) {
        init_resource_fail("userport rtc");
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
    scpu64_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    cartridge_resources_shutdown();
    rombanks_resources_shutdown();
    userport_rtc_resources_shutdown();
    cartio_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
}

/* C64-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (scpu64_cmdline_options_init() < 0) {
        init_cmdline_options_fail("scpu64");
        return -1;
    }
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
        return -1;
    }
    if (sid_cmdline_options_init() < 0) {
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
    if (joystick_cmdline_options_init() < 0) {
        init_cmdline_options_fail("joystick");
        return -1;
    }
    if (gfxoutput_cmdline_options_init() < 0) {
        init_cmdline_options_fail("gfxoutput");
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
    if (lightpen_cmdline_options_init() < 0) {
        init_cmdline_options_fail("lightpen");
        return -1;
    }
#endif
#ifndef COMMON_KBD
    if (kbd_cmdline_options_init() < 0) {
        init_cmdline_options_fail("kbd");
        return -1;
    }
#endif
    if (drive_cmdline_options_init() < 0) {
        init_cmdline_options_fail("drive");
        return -1;
    }
    if (scpu64_glue_cmdline_options_init() < 0) {
        init_cmdline_options_fail("scpu64 glue");
        return -1;
    }
    if (userport_joystick_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport joystick");
        return -1;
    }
    if (userport_rtc_cmdline_options_init() < 0) {
        init_cmdline_options_fail("userport rtc");
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

static void scpu64_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502, asmR65C02, asm65816;
    monitor_interface_t *drive_interface_init[DRIVE_NUM];
    monitor_cpu_type_t *asmarray[4];

    asmarray[0] = &asm65816;
    asmarray[1] = &asmR65C02;
    asmarray[2] = &asm6502;
    asmarray[3] = NULL;

    asm6502_init(&asm65816);
    asmR65C02_init(&asmR65C02);
    asm65816_init(&asm6502);

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

    scpu64_log = log_open("SCPU64");

    if (mem_load() < 0) {
        return -1;
    }

    event_init();

    /* Setup trap handling.  */
    traps_init();

    if (!video_disabled_mode) {
        joystick_init();
    }

    gfxoutput_init();

    /* Initialize serial traps.  */
    if (serial_init(scpu64_serial_traps) < 0) {
        return -1;
    }

    serial_trap_init(0xa4);
    serial_iec_bus_init();

    /* Initialize RS232 handler.  */
    rs232drv_init();
    c64_rsuser_init();

    /* Initialize print devices.  */
    printer_init();

    /* Fire up the hardware-level drive emulation.  */
    drive_init();

    disk_image_init();

    resources_get_int("AutostartDelay", &delay);
    if (delay == 0) {
        delay = 3; /* default */
    }

    /* Initialize autostart.  */
    autostart_init((CLOCK)(delay * SCPU64_PAL_RFSH_PER_SEC * SCPU64_PAL_CYCLES_PER_RFSH), 1, 0xcc, 0xd1, 0xd3, 0xd5);

    if (vicii_init(VICII_STANDARD) == NULL && !video_disabled_mode) {
        return -1;
    }

    scpu64_mem_init();

    cia1_init(machine_context.cia1);
    cia2_init(machine_context.cia2);

#ifndef COMMON_KBD
    /* Initialize the keyboard.  */
    if (c64_kbd_init() < 0) {
        return -1;
    }
#endif
    c64keyboard_init();

    scpu64_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);

    /* Initialize native sound chip */
    sid_sound_chip_init();

    /* Initialize cartridge based sound chips */
    cartridge_sound_chip_init();

    drive_sound_init();
    video_sound_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);

    /* Initialize keyboard buffer.  */
    kbdbuf_init(631, 198, 10, (CLOCK)(machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh));

    /* Initialize the C64-specific I/O */
    c64io_init();

    /* Initialize the C64-specific part of the UI.  */
    if (!console_mode) {
        scpu64ui_init();
    }

    /* Initialize glue logic.  */
    scpu64_glue_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();

    /* Initialize lightpen support and register VICII callbacks */
    lightpen_init();
    lightpen_register_timing_callback(vicii_lightpen_timing, 0);
    lightpen_register_trigger_callback(vicii_trigger_light_pen);
#endif
    c64iec_init();
    c64fastiec_init();

    cartridge_init();

    machine_drive_stub();
#if defined (USE_XF86_EXTENSIONS) && (defined(USE_XF86_VIDMODE_EXT) || defined (HAVE_XRANDR))
    {
        /* set fullscreen if user used `-fullscreen' on cmdline */
        int fs;
        resources_get_int("UseFullscreen", &fs);
        if (fs) {
            resources_set_int("VICIIFullscreen", 1);
        }
    }
#endif

    return 0;
}

/* SCPU64-specific reset sequence.  */
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
    scpu64_hardware_reset();

    reset_poweron = 0;
}

void machine_specific_powerup(void)
{
    vicii_reset_registers();
    reset_poweron = 1;
}

void machine_specific_shutdown(void)
{
    /* and cartridge */
    cartridge_detach_image(-1);

    ciacore_shutdown(machine_context.cia1);
    ciacore_shutdown(machine_context.cia2);

    scpu64_mem_shutdown();

    /* close the video chip(s) */
    vicii_shutdown();

    cartridge_shutdown();

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    sid_cmdline_options_shutdown();

    scpu64ui_shutdown();
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
    *half_cycle = (int)scpu64_get_half_cycle();
}

void machine_change_timing(int timeval)
{
    int border_mode;

    switch (timeval) {
        default:
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
        case MACHINE_SYNC_NTSCOLD ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
        case MACHINE_SYNC_PALN ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_NORMAL_BORDERS);
            border_mode = VICII_NORMAL_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
        case MACHINE_SYNC_NTSCOLD ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
        case MACHINE_SYNC_PALN ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_FULL_BORDERS);
            border_mode = VICII_FULL_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
        case MACHINE_SYNC_NTSCOLD ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
        case MACHINE_SYNC_PALN ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_DEBUG_BORDERS);
            border_mode = VICII_DEBUG_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
        case MACHINE_SYNC_NTSCOLD ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
        case MACHINE_SYNC_PALN ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_NO_BORDERS);
            border_mode = VICII_NO_BORDERS;
            break;
    }

    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = SCPU64_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = SCPU64_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = SCPU64_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = SCPU64_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = SCPU64_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = SCPU64_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = SCPU64_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = SCPU64_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = SCPU64_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = SCPU64_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        case MACHINE_SYNC_NTSCOLD:
            machine_timing.cycles_per_sec = SCPU64_NTSCOLD_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = SCPU64_NTSCOLD_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = SCPU64_NTSCOLD_RFSH_PER_SEC;
            machine_timing.cycles_per_line = SCPU64_NTSCOLD_CYCLES_PER_LINE;
            machine_timing.screen_lines = SCPU64_NTSCOLD_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        case MACHINE_SYNC_PALN:
            machine_timing.cycles_per_sec = SCPU64_PALN_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = SCPU64_PALN_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = SCPU64_PALN_RFSH_PER_SEC;
            machine_timing.cycles_per_line = SCPU64_PALN_CYCLES_PER_LINE;
            machine_timing.screen_lines = SCPU64_PALN_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        default:
            log_error(scpu64_log, "Unknown machine timing.");
    }

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);
    debug_set_machine_parameter(machine_timing.cycles_per_line, machine_timing.screen_lines);
    drive_set_machine_parameter(machine_timing.cycles_per_sec);
    serial_iec_device_set_machine_parameter(machine_timing.cycles_per_sec);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);
    clk_guard_set_clk_base(maincpu_clk_guard, machine_timing.cycles_per_rfsh);

    vicii_change_timing(&machine_timing, border_mode);

    cia1_set_timing(machine_context.cia1, machine_timing.cycles_per_sec, machine_timing.power_freq);
    cia2_set_timing(machine_context.cia2, machine_timing.cycles_per_sec, machine_timing.power_freq);

    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks, int event_mode)
{
    return scpu64_snapshot_write(name, save_roms, save_disks, event_mode);
}

int machine_read_snapshot(const char *name, int event_mode)
{
    return scpu64_snapshot_read(name, event_mode);
}

/* ------------------------------------------------------------------------- */

int machine_autodetect_psid(const char *name)
{
    return -1;
}

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

BYTE machine_tape_type_default(void)
{
    return 0;
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
    return machine_name;
}
