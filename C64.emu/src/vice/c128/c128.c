/*
 * c128.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * Based on the original work in VICE 0.11.0 by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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
 *  along with this program; if not, wrie to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>

#include "attach.h"
#include "autostart.h"
#include "c128-cmdline-options.h"
#include "c128-resources.h"
#include "c128-snapshot.h"
#include "c128.h"
#include "c128fastiec.h"
#include "c128mem.h"
#include "c128memrom.h"
#include "c128mmu.h"
#include "c128ui.h"
#include "c64-midi.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cia.h"
#include "c64export.h"
#include "c64iec.h"
#include "c64keyboard.h"
#include "c64memrom.h"
#include "c64rsuser.h"
#include "cartridge.h"
#include "cia.h"
#include "clkguard.h"
#include "datasette.h"
#include "debug.h"
#include "diskimage.h"
#include "drive-cmdline-options.h"
#include "drive-resources.h"
#include "drive-sound.h"
#include "drive.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "functionrom.h"
#include "gfxoutput.h"
#include "imagecontents.h"
#include "init.h"
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
#include "parallel.h"
#include "patchrom.h"
#include "printer.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "screenshot.h"
#include "serial.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "sound.h"
#include "tape.h"
#include "tpi.h"
#include "traps.h"
#include "types.h"
#include "userport_joystick.h"
#include "userport_rtc.h"
#include "vice-event.h"
#include "vicii.h"
#include "vicii-mem.h"
#include "video.h"
#include "video-sound.h"
#include "vdc.h"
#include "vdc-mem.h"
#include "vsync.h"
#include "z80.h"
#include "z80mem.h"

#ifdef HAVE_MOUSE
#include "lightpen.h"
#include "mouse.h"
#endif

/* dummy functions until the C128 version of the
   256K expansion can be made */

int c64_256k_enabled = 0;
int c64_256k_start = 0xdf80;

void c64_256k_store(WORD addr, BYTE byte)
{
}

BYTE c64_256k_read(WORD addr)
{
    return 0xff;
}

BYTE c64_256k_ram_segment2_read(WORD addr)
{
    return mem_ram[addr];
}

void c64_256k_ram_segment2_store(WORD addr, BYTE byte)
{
    mem_ram[addr] = byte;
}

void c64_256k_cia_set_vbank(int ciabank)
{
}

/* dummy functions until the C128 version of the
   +60K expansion can be made */

int plus60k_enabled = 0;

BYTE plus60k_ram_read(WORD addr)
{
    return mem_ram[addr];
}

void plus60k_ram_store(WORD addr, BYTE value)
{
    mem_ram[addr] = value;
}

/* dummy functions until the C128 version of the
   +256K expansion can be made */

int plus256k_enabled = 0;

BYTE plus256k_ram_high_read(WORD addr)
{
    return mem_ram[addr];
}

void plus256k_ram_high_store(WORD addr, BYTE byte)
{
    mem_ram[addr] = byte;
}

/* Lightpen trigger function; needs to trigger both VICII and VDC */
void c128_trigger_light_pen(CLOCK mclk)
{
    vicii_trigger_light_pen(mclk);
    vdc_trigger_light_pen(mclk);
}

machine_context_t machine_context;

const char machine_name[] = "C128";
int machine_class = VICE_MACHINE_C128;

static void machine_vsync_hook(void);

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

static const trap_t c128_serial_traps[] = {
    { "SerialListen", 0xE355, 0xE5BA, { 0x20, 0x73, 0xE5 }, serial_trap_attention, c128memrom_trap_read, c128memrom_trap_store },
    { "SerialSaListen", 0xE37C, 0xE5BA, { 0x20, 0x73, 0xE5 }, serial_trap_attention, c128memrom_trap_read, c128memrom_trap_store },
    { "SerialSendByte", 0xE38C, 0xE5BA, { 0x20, 0x73, 0xE5 }, serial_trap_send, c128memrom_trap_read, c128memrom_trap_store },
    { "SerialReceiveByte", 0xE43E, 0xE5BA, { 0x20, 0x73, 0xE5 }, serial_trap_receive, c128memrom_trap_read, c128memrom_trap_store },
    { "Serial ready", 0xE569, 0xE572, { 0xAD, 0x00, 0xDD }, serial_trap_ready, c128memrom_trap_read, c128memrom_trap_store },
    { "Serial ready", 0xE4F5, 0xE572, { 0xAD, 0x00, 0xDD }, serial_trap_ready, c128memrom_trap_read, c128memrom_trap_store },
    { "SerialListen", 0xED24, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_attention, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialSaListen", 0xED37, 0xEDAB, { 0x20, 0x8E, 0xEE }, serial_trap_attention, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialSendByte", 0xED41, 0xEDAB, { 0x20, 0x97, 0xEE }, serial_trap_send, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialReceiveByte", 0xEE14, 0xEDAB, { 0xA9, 0x00, 0x85 }, serial_trap_receive, c64memrom_trap_read, c64memrom_trap_store },
    { "SerialReady", 0xEEA9, 0xEDAB, { 0xAD, 0x00, 0xDD }, serial_trap_ready, c64memrom_trap_read, c64memrom_trap_store },
    { NULL, 0, 0, { 0, 0, 0 }, NULL, NULL, NULL }
};

/* Tape traps.  */
static const trap_t c128_tape_traps[] = {
    { "TapeFindHeader", 0xE8D3, 0xE8D6, { 0x20, 0xF2, 0xE9 }, tape_find_header_trap, c128memrom_trap_read, c128memrom_trap_store },
    { "TapeReceive", 0xEA60, 0xEE57, { 0x20, 0x9B, 0xEE }, tape_receive_trap, c128memrom_trap_read, c128memrom_trap_store },
    { "TapeFindHeader", 0xF72F, 0xF732, { 0x20, 0x41, 0xF8 }, tape_find_header_trap, c64memrom_trap_read, c64memrom_trap_store },
    { "TapeReceive", 0xF8A1, 0xFC93, { 0x20, 0xBD, 0xFC }, tape_receive_trap, c64memrom_trap_read, c64memrom_trap_store },
    { NULL, 0, 0, { 0, 0, 0 }, NULL, NULL, NULL
    }
};

static const tape_init_t tapeinit_c128_mode = {
    0xb2,
    0x90,
    0x93,
    0xa09,
    0,
    0xc1,
    0xae,
    0x34a,
    0xd0,
    c128_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static const tape_init_t tapeinit_c64_mode = {
    0xb2,
    0x90,
    0x93,
    0x29f,
    0,
    0xc1,
    0xae,
    0x277,
    0xc6,
    c128_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static int tapemode = 0;

void machine_tape_init_c64(void)
{
    if (tapemode != 1) {
        if (tapemode == 0) {
            tape_init(&tapeinit_c64_mode);
        } else {
            tape_reinit(&tapeinit_c64_mode);
        }
        tapemode = 1;
    }
}

void machine_tape_init_c128(void)
{
    if (tapemode != 2) {
        if (tapemode == 0) {
            tape_init(&tapeinit_c128_mode);
        } else {
            tape_reinit(&tapeinit_c128_mode);
        }
        tapemode = 2;
    }
}

static log_t c128_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

/* C128-specific I/O initialization. */

static io_source_t vicii_d000_device = {
    "VIC-IIe",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd000, 0xd0ff, 0x7f,
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
    "VIC-IIe $D100-$D1FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd100, 0xd1ff, 0x7f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d200_device = {
    "VIC-IIe $D200-$D2FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd100, 0xd1ff, 0x7f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d300_device = {
    "VIC-IIe $D300-$D3FF mirrors",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd100, 0xd1ff, 0x7f,
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
    "SID mirrors",
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

static io_source_list_t *vicii_d000_list_item = NULL;
static io_source_list_t *vicii_d100_list_item = NULL;
static io_source_list_t *vicii_d200_list_item = NULL;
static io_source_list_t *vicii_d300_list_item = NULL;
static io_source_list_t *sid_d400_list_item = NULL;
static io_source_list_t *sid_d420_list_item = NULL;

void c64io_vicii_init(void)
{
    vicii_d000_list_item = io_source_register(&vicii_d000_device);
    vicii_d100_list_item = io_source_register(&vicii_d100_device);
    vicii_d200_list_item = io_source_register(&vicii_d200_device);
    vicii_d300_list_item = io_source_register(&vicii_d300_device);
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

static void c128io_init(void)
{
    c64io_vicii_init();
    sid_d400_list_item = io_source_register(&sid_d400_device);
    sid_d420_list_item = io_source_register(&sid_d420_device);
}

/* ------------------------------------------------------------------------ */

/* C128-specific resource initialization.  This is called before initializing
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
    if (c128_resources_init() < 0) {
        init_resource_fail("c128");
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
    if (vdc_resources_init() < 0) {
        init_resource_fail("vdc");
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
    if (lightpen_resources_init() < 0) {
        init_resource_fail("lightpen");
        return -1;
    }
    if (mouse_resources_init() < 0) {
        init_resource_fail("mouse");
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
    if (datasette_resources_init() < 0) {
        init_resource_fail("datasette");
        return -1;
    }
    if (cartridge_resources_init() < 0) {
        init_resource_fail("cartridge");
        return -1;
    }
    if (mmu_resources_init() < 0) {
        init_resource_fail("mmu");
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
    if (functionrom_resources_init() < 0) {
        init_resource_fail("functionrom");
        return -1;
    }
    return 0;
}

void machine_resources_shutdown(void)
{
    serial_shutdown();
    c128_resources_shutdown();
    rs232drv_resources_shutdown();
    printer_resources_shutdown();
    drive_resources_shutdown();
    cartridge_resources_shutdown();
    functionrom_resources_shutdown();
    rombanks_resources_shutdown();
    userport_rtc_resources_shutdown();
    cartio_shutdown();
    fsdevice_resources_shutdown();
    disk_image_resources_shutdown();
}

/* C128-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (traps_cmdline_options_init() < 0) {
        init_cmdline_options_fail("traps");
        return -1;
    }
    if (c128_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c128");
        return -1;
    }
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
        return -1;
    }
    if (vdc_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vdc");
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
    if (lightpen_cmdline_options_init() < 0) {
        init_cmdline_options_fail("lightpen");
        return -1;
    }
    if (mouse_cmdline_options_init() < 0) {
        init_cmdline_options_fail("mouse");
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
    if (datasette_cmdline_options_init() < 0) {
        init_cmdline_options_fail("datasette");
        return -1;
    }
    if (cartridge_cmdline_options_init() < 0) {
        init_cmdline_options_fail("cartridge");
        return -1;
    }
    if (mmu_cmdline_options_init() < 0) {
        init_cmdline_options_fail("mmu");
        return -1;
    }
    if (functionrom_cmdline_options_init() < 0) {
        init_cmdline_options_fail("functionrom");
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
    return 0;
}

static void c128_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502, asmz80, asmR65C02;
    monitor_interface_t *drive_interface_init[DRIVE_NUM];
    monitor_cpu_type_t *asmarray[4];

    asmarray[0] = &asm6502;
    asmarray[1] = &asmz80;
    asmarray[2] = &asmR65C02;
    asmarray[3] = NULL;

    asm6502_init(&asm6502);
    asmz80_init(&asmz80);
    asmR65C02_init(&asmR65C02);

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

/* C128-specific initialization.  */
int machine_specific_init(void)
{
    int delay;

    c128_log = log_open("C128");

    if (mem_load() < 0) {
        return -1;
    }

    event_init();

    if (z80mem_load() < 0) {
        return -1;
    }

    /* Setup trap handling.  */
    traps_init();

    /* Initialize serial traps.  */
    if (serial_init(c128_serial_traps) < 0) {
        return -1;
    }

    serial_trap_init(0xa4);
    serial_iec_bus_init();

    if (!video_disabled_mode) {
        joystick_init();
    }

    gfxoutput_init();

    /* initialize RS232 handler */
    rs232drv_init();
    c64_rsuser_init();

    /* initialize print devices */
    printer_init();

    /* Initialize the tape emulation.  */
    machine_tape_init_c128();

    /* Initialize the datasette emulation.  */
    datasette_init();

    /* Fire up the hardware-level drive emulation.  */
    drive_init();

    disk_image_init();

    /* Initialize autostart. FIXME: at least 0xa26 is only for 40 cols */
    resources_get_int("AutostartDelay", &delay);
    if (delay == 0) {
        delay = 3; /* default */
    }
    autostart_init((CLOCK)(delay * C128_PAL_RFSH_PER_SEC * C128_PAL_CYCLES_PER_RFSH), 1, 0xa27, 0xe0, 0xec, 0xee);

    if (vdc_init() == NULL) {
        return -1;
    }

    if (vicii_init(VICII_EXTENDED) == NULL) {
        return -1;
    }

    cia1_init(machine_context.cia1);
    cia2_init(machine_context.cia2);

#ifndef COMMON_KBD
    /* Initialize the keyboard.  */
    if (c128_kbd_init() < 0) {
        return -1;
    }
#endif

    c64keyboard_init();

    c128_monitor_init();

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
    kbdbuf_init(842, 208, 10, (CLOCK)(machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh));

    /* Initialize the C128-specific I/O */
    c128io_init();

    /* Initialize the C128-specific part of the UI.  */
    c128ui_init();

#ifdef HAVE_MOUSE
    /* Initialize mouse support (if present).  */
    mouse_init();

    /* Initialize lightpen support and register VICII/VDC callbacks */
    lightpen_init();
    lightpen_register_timing_callback(vicii_lightpen_timing, 1);
    lightpen_register_timing_callback(vdc_lightpen_timing, 0);
    lightpen_register_trigger_callback(c128_trigger_light_pen);
#endif

    c64iec_init();
    c128fastiec_init();

    cartridge_init();

    mmu_init();

    machine_drive_stub();

#if defined (USE_XF86_EXTENSIONS) && (defined(USE_XF86_VIDMODE_EXT) || defined (HAVE_XRANDR))
    {
        /* set fullscreen if user used `-fullscreen' on cmdline
           use VICII as default */
        int fs;

        resources_get_int("UseFullscreen", &fs);
        if (fs) {
            resources_get_int("40/80ColumnKey", &fs);
            if (fs == 1) {
                resources_set_int("VICIIFullscreen", 1);
            } else {
                resources_set_int("VDCFullscreen", 1);
            }
        }
    }
#endif

    return 0;
}

/* C128-specific reset sequence.  */
void machine_specific_reset(void)
{
    serial_traps_reset();

    ciacore_reset(machine_context.cia1);
    ciacore_reset(machine_context.cia2);
    sid_reset();

    rs232drv_reset();
    rsuser_reset();

    printer_reset();

    vdc_reset();

    /* The VIC-II must be the *last* to be reset.  */
    vicii_reset();

    cartridge_reset();
    drive_reset();
    datasette_reset();

    z80mem_initialize();
    z80_reset();
}

void machine_specific_powerup(void)
{
}

void machine_specific_shutdown(void)
{
    /* and the tape */
    tape_image_detach_internal(1);

    /* and cartridge */
    cartridge_detach_image(-1);

    ciacore_shutdown(machine_context.cia1);
    ciacore_shutdown(machine_context.cia2);

    cartridge_shutdown();

#ifdef HAVE_MOUSE
    mouse_shutdown();
#endif

    /* close the video chip(s) */
    vicii_shutdown();
    vdc_shutdown();

    sid_cmdline_options_shutdown();

    c128ui_shutdown();
}

void machine_handle_pending_alarms(int num_write_cycles)
{
    vicii_handle_pending_alarms_external(num_write_cycles);
}

/* ------------------------------------------------------------------------- */

void machine_kbdbuf_reset_c128(void)
{
    kbdbuf_reset(842, 208, 10, (CLOCK)(machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh));
}

void machine_kbdbuf_reset_c64(void)
{
    kbdbuf_reset(631, 198, 10, (CLOCK)(machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh));
}

void machine_autostart_reset_c128(void)
{
    /* FIXME: at least 0xa26 is only for 40 cols */
    autostart_reinit((CLOCK)(3 * machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh), 1, 0xa27, 0xe0, 0xec, 0xee);
}

void machine_autostart_reset_c64(void)
{
    autostart_reinit((CLOCK)(3 * machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh), 1, 0xcc, 0xd1, 0xd3, 0xd5);
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
    *half_cycle = (int)vicii_get_half_cycle();
}

void machine_change_timing(int timeval)
{
    int border_mode;

    switch (timeval) {
        default:
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_NORMAL_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_NORMAL_BORDERS);
            border_mode = VICII_NORMAL_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_FULL_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_FULL_BORDERS);
            border_mode = VICII_FULL_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_DEBUG_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_DEBUG_BORDERS);
            border_mode = VICII_DEBUG_BORDERS;
            break;
        case MACHINE_SYNC_PAL ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
        case MACHINE_SYNC_NTSC ^ VICII_BORDER_MODE(VICII_NO_BORDERS):
            timeval ^= VICII_BORDER_MODE(VICII_NO_BORDERS);
            border_mode = VICII_NO_BORDERS;
            break;
    }

    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = C128_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C128_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C128_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C128_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = C128_PAL_SCREEN_LINES;
            machine_timing.power_freq = 50;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C128_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C128_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C128_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C128_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C128_NTSC_SCREEN_LINES;
            machine_timing.power_freq = 60;
            break;
        default:
            log_error(c128_log, "Unknown machine timing.");
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
    return c128_snapshot_write(name, save_roms, save_disks, event_mode);
}

int machine_read_snapshot(const char *name, int event_mode)
{
    return c128_snapshot_read(name, event_mode);
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
    if (canvas == vdc_get_canvas()) {
        vdc_screenshot(screenshot);
        return 0;
    }

    return -1;
}

int machine_canvas_async_refresh(struct canvas_refresh_s *refresh, struct video_canvas_s *canvas)
{
    if (canvas == vicii_get_canvas()) {
        vicii_async_refresh(refresh);
        return 0;
    }
    if (canvas == vdc_get_canvas()) {
        vdc_async_refresh(refresh);
        return 0;
    }

    return -1;
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
    return TAPE_CAS_TYPE_BAS;
}

int machine_addr_in_ram(unsigned int addr)
{
    /* TODO check for carts */
    return (addr < 0xe000 && !(addr >= 0xa000 && addr < 0xc000)) ? 1 : 0;
}

const char *machine_get_name(void)
{
    return machine_name;
}
