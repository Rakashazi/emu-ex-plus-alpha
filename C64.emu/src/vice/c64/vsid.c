/*
 * vsid.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  groepaz <groepaz@gmx.net>
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
#include <stdbool.h>

/* FIXME: remove more unneeded stuff
 *
 * all iec/drive/printer/cartridge can be removed and replaced by stubs in
 * vsidstubs.c
 */
#include "c64-resources.h"
#include "c64-snapshot.h"
#include "c64.h"
#include "c64cia.h"
#include "c64gluelogic.h"
#include "c64mem.h"
#include "cia.h"
#include "debug.h"
#include "drive.h"
#include "imagecontents.h"
#include "init.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "log.h"
#include "machine-drive.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "psid.h"
#include "resources.h"
#include "screenshot.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid.h"
#include "vicii.h"
#include "vicii-mem.h"
#include "video.h"
#include "vsid-cmdline-options.h"
#include "vsidui.h"
#include "vsid-debugcart.h"
#include "vsync.h"


machine_context_t machine_context;

const char machine_name[] = "C64"; /* FIXME: this must be c64 currently, else the roms can not be loaded */

/** \brief  PSID file loaded from commandline
 *
 * The UI needs to attempt to retrieve STIL info for a file "autostarted" from
 * the command line.
 */
char *psid_autostart_image = NULL;

/* Moved to c64mem.c/c64memsc.c/vsidmem.c
int machine_class = VICE_MACHINE_VSID;
*/
static void machine_vsync_hook(void);

static log_t c64_log = LOG_ERR;
static machine_timing_t machine_timing;

/* ------------------------------------------------------------------------ */

static int vsid_autostart_delay = 0;
static uint16_t vsid_autostart_load_addr = 0;
static uint8_t *vsid_autostart_data = NULL;
static uint16_t vsid_autostart_length = 0;

/* ------------------------------------------------------------------------ */




/* C64-specific resource initialization.  This is called before initializing
   the machine itself with `machine_init()'.  */
int machine_resources_init(void)
{
    if (c64_resources_init() < 0) {
        init_resource_fail("c64");
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
    if (psid_resources_init() < 0) {
        init_resource_fail("psid");
        return -1;
    }
    if (debugcart_resources_init() < 0) {
        init_resource_fail("debug cart");
        return -1;
    }
#ifdef DEBUG
    if (debug_resources_init() < 0) {
        init_resource_fail("debug");
        return -1;
    }
#endif
    return 0;
}

void machine_resources_shutdown(void)
{
    c64_resources_shutdown();
    debugcart_resources_shutdown();
}

/* C64-specific command-line option initialization.  */
int machine_cmdline_options_init(void)
{
    if (vsid_cmdline_options_init() < 0) {
        init_cmdline_options_fail("c64");
        return -1;
    }
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
    if (vicii_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vicii");
        return -1;
    }
#endif
    if (sid_cmdline_options_init(SIDTYPE_SID) < 0) {
        init_cmdline_options_fail("sid");
        return -1;
    }
    if (psid_cmdline_options_init() < 0) {
        init_cmdline_options_fail("psid");
        return -1;
    }
    if (debugcart_cmdline_options_init() < 0) {
        init_cmdline_options_fail("debug cart");
        return -1;
    }
    return 0;
}

static void c64_monitor_init(void)
{
    unsigned int dnr;
    monitor_cpu_type_t asm6502;
    monitor_interface_t *drive_interface_init[NUM_DISK_UNITS];
    monitor_cpu_type_t *asmarray[2];

    asmarray[0] = &asm6502;
    asmarray[1] = NULL;

    asm6502_init(&asm6502);

    /* keep the monitor happy */
    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        drive_interface_init[dnr] = maincpu_monitor_interface_get();
    }

    /* Initialize the monitor.  */
    monitor_init(maincpu_monitor_interface_get(), drive_interface_init, asmarray);
}

void machine_setup_context(void)
{
    cia1_setup_context(&machine_context);
    cia2_setup_context(&machine_context);
}

/* C64-specific initialization.  */
int machine_specific_init(void)
{
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
    if (console_mode) {
        video_disabled_mode = 1;
    }
#else
    video_disabled_mode = 1;
#endif

    c64_log = log_open("C64");

    if (mem_load() < 0) {
        return -1;
    }

    if (vicii_init(VICII_STANDARD) == NULL && !video_disabled_mode) {
        return -1;
    }

    c64_mem_init();

    cia1_init(machine_context.cia1);
    cia2_init(machine_context.cia2);

    if (!video_disabled_mode) {
        joystick_init();
    }

    c64_monitor_init();

    /* Initialize vsync and register our hook function.  */
    vsync_init(machine_vsync_hook);
    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);

    /* Initialize native sound chip */
    sid_sound_chip_init();

    /* Initialize sound.  Notice that this does not really open the audio
       device yet.  */
    sound_init((unsigned int)(machine_timing.cycles_per_sec),
               (unsigned int)(machine_timing.cycles_per_rfsh));

    /* Initialize keyboard buffer.  */
    kbdbuf_init(631, 198, 10, (CLOCK)(machine_timing.rfsh_per_sec * machine_timing.cycles_per_rfsh));

    /* Initialize the C64-specific part of the UI.  */
    if (!console_mode) {
        vsid_ui_init();
    }

    /* Initialize glue logic.  */
    c64_glue_init();

    machine_drive_stub();

    return 0;
}

/* C64-specific reset sequence.  */
void machine_specific_reset(void)
{
    ciacore_reset(machine_context.cia1);
    ciacore_reset(machine_context.cia2);
    sid_reset();

    /* The VIC-II must be the *last* to be reset.  */
    vicii_reset();

    if (psid_basic_rsid_to_autostart(&vsid_autostart_load_addr, &vsid_autostart_data, &vsid_autostart_length)) {
        vsid_autostart_delay = (int)(machine_timing.rfsh_per_sec * 23 / 10);
    } else {
        vsid_autostart_delay = 0; /* disables it */
        psid_init_driver();
        psid_init_tune(1);
    }
}

void machine_specific_powerup(void)
{
    vicii_reset_registers();
}

void machine_specific_shutdown(void)
{
    ciacore_shutdown(machine_context.cia1);
    ciacore_shutdown(machine_context.cia2);

    /* close the video chip(s) */
    vicii_shutdown();

    if (!console_mode) {
        vsid_ui_close();
    }

    sid_cmdline_options_shutdown();

    psid_shutdown();
}

void machine_handle_pending_alarms(CLOCK num_write_cycles)
{
    vicii_handle_pending_alarms_external(num_write_cycles);
}

/* ------------------------------------------------------------------------- */

/* This hook is called at the end of every frame.  */
static void machine_vsync_hook(void)
{
    int i;
    unsigned int playtime;
    static unsigned int time = 0;

    if (vsid_autostart_delay > 0) {
        if (--vsid_autostart_delay == 0) {
            log_message(c64_log, "Triggering VSID autoload");
            psid_init_tune(0);
            for (i = 0; i < vsid_autostart_length; i += 1) {
                mem_inject((uint16_t)(vsid_autostart_load_addr + i),
                        vsid_autostart_data[i]);
            }
            mem_set_basic_text(vsid_autostart_load_addr,
                    (uint16_t)(vsid_autostart_load_addr + vsid_autostart_length));
            kbdbuf_feed_runcmd("RUN\r");
        }
    }

#if 0
    playtime = (psid_increment_frames() * machine_timing.cycles_per_rfsh)
        / machine_timing.cycles_per_sec;
#else
    /* Count deciseconds */
    playtime = (double)psid_increment_frames()
        / machine_timing.rfsh_per_sec * 10.0;
#endif
    if (playtime != time) {
        time = playtime;
        vsid_ui_display_time(playtime);
    }
}

void machine_set_restore_key(int v)
{
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

void machine_change_timing(int timeval, int powerfreq, int border_mode)
{
    switch (timeval) {
        case MACHINE_SYNC_PAL:
            machine_timing.cycles_per_sec = C64_PAL_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_PAL_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_PAL_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_PAL_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_PAL_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        case MACHINE_SYNC_NTSC:
            machine_timing.cycles_per_sec = C64_NTSC_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_NTSC_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_NTSC_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_NTSC_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_NTSC_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        case MACHINE_SYNC_NTSCOLD:
            machine_timing.cycles_per_sec = C64_NTSCOLD_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_NTSCOLD_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_NTSCOLD_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_NTSCOLD_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_NTSCOLD_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        case MACHINE_SYNC_PALN:
            machine_timing.cycles_per_sec = C64_PALN_CYCLES_PER_SEC;
            machine_timing.cycles_per_rfsh = C64_PALN_CYCLES_PER_RFSH;
            machine_timing.rfsh_per_sec = C64_PALN_RFSH_PER_SEC;
            machine_timing.cycles_per_line = C64_PALN_CYCLES_PER_LINE;
            machine_timing.screen_lines = C64_PALN_SCREEN_LINES;
            machine_timing.power_freq = powerfreq;
            break;
        default:
            log_error(c64_log, "Unknown machine timing.");
    }

    vsync_set_machine_parameter(machine_timing.rfsh_per_sec, machine_timing.cycles_per_sec);
    sound_set_machine_parameter(machine_timing.cycles_per_sec, machine_timing.cycles_per_rfsh);
    debug_set_machine_parameter(machine_timing.cycles_per_line, machine_timing.screen_lines);
    sid_set_machine_parameter(machine_timing.cycles_per_sec);

    vicii_change_timing(&machine_timing, border_mode);

    cia1_set_timing(machine_context.cia1,
                    (int)machine_timing.cycles_per_sec,
                    machine_timing.power_freq);
    cia2_set_timing(machine_context.cia2,
                    (int)machine_timing.cycles_per_sec,
                    machine_timing.power_freq);

    machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
}

/* ------------------------------------------------------------------------- */

int machine_write_snapshot(const char *name, int save_roms, int save_disks, int event_mode)
{
    return c64_snapshot_write(name, save_roms, save_disks, event_mode);
}

int machine_read_snapshot(const char *name, int event_mode)
{
    return c64_snapshot_read(name, event_mode);
}

/* ------------------------------------------------------------------------- */

int machine_autodetect_psid(const char *name)
{
    if (name == NULL) {
        return -1;
    }

    if (psid_load_file(name) < 0) {
        /* FIXME: show error message box */
        return -1;
    }
    psid_autostart_image = lib_strdup(name);
    return 0;
}

void machine_play_psid(int tune)
{
    psid_set_tune(tune);
}

int machine_screenshot(screenshot_t *screenshot, struct video_canvas_s *canvas)
{
    return -1;
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
    return NULL;
}

uint8_t machine_tape_type_default(void)
{
    return 0;
}

uint8_t machine_tape_behaviour(void)
{
    return 0;
}

int machine_addr_in_ram(unsigned int addr)
{
    return ((addr < 0xe000 && !(addr >= 0xa000 && addr < 0xc000)));
}

const char *machine_get_name(void)
{
    return "VSID";
}

char *machine_get_keyboard_type_name(int type)
{
    return NULL; /* return 0 if no different types exist */
}
