/*
 * machine.c - Interface to machine-specific implementations.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"
#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "clkguard.h"
#include "cmdline.h"
#include "console.h"
#include "diskimage.h"
#include "drive.h"
#include "vice-event.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "interrupt.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "monitor_network.h"
#include "network.h"
#include "printer.h"
#include "resources.h"
#include "romset.h"
#include "screenshot.h"
#include "sound.h"
#include "sysfile.h"
#include "tape.h"
#include "traps.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "video.h"
#include "vsync.h"
#include "zfile.h"

#ifdef HAS_JOYSTICK
#include "joy.h"
#endif

static int machine_init_was_called = 0;
static int mem_initialized = 0;
static int ignore_jam = 0;
static int jam_action = MACHINE_JAM_ACTION_DIALOG;
int machine_keymap_index;
static char *ExitScreenshotName = NULL;

unsigned int machine_jam(const char *format, ...)
{
    char *str;
    va_list ap;
    ui_jam_action_t ret;

    if (ignore_jam > 0) {
        return JAM_NONE;
    }

    va_start(ap, format);
    str = lib_mvsprintf(format, ap);
    va_end(ap);

    log_message(LOG_DEFAULT, "*** %s", str);

    if (jam_action == MACHINE_JAM_ACTION_DIALOG) {
        if (monitor_is_remote()) {
            ret = monitor_network_ui_jam_dialog(str);
        } else {
            ret = ui_jam_dialog(str);
        }
    } else if (jam_action == MACHINE_JAM_ACTION_QUIT) {
        exit(EXIT_SUCCESS);
    } else {
        int actions[4] = {
            -1, UI_JAM_MONITOR, UI_JAM_RESET, UI_JAM_HARD_RESET
        };
        ret = actions[jam_action - 1];
    }
    lib_free(str);

    /* always ignore subsequent JAMs. reset would clear the flag again, not
     * setting it when going to the monitor would just repeatedly pop up the
     * jam dialog (until reset)
     */
    ignore_jam = 1;

    switch (ret) {
        case UI_JAM_RESET:
            return JAM_RESET;
        case UI_JAM_HARD_RESET:
            return JAM_HARD_RESET;
        case UI_JAM_MONITOR:
            return JAM_MONITOR;
        default:
            break;
    }
    return JAM_NONE;
}

static void machine_trigger_reset_internal(const unsigned int mode)
{
    ignore_jam = 0;

    switch (mode) {
        case MACHINE_RESET_MODE_HARD:
            vsync_frame_counter = 0;
            mem_initialized = 0; /* force memory initialization */
            machine_specific_powerup();
        /* Fall through.  */
        case MACHINE_RESET_MODE_SOFT:
            maincpu_trigger_reset();
            break;
    }
}

void machine_trigger_reset(const unsigned int mode)
{
    if (event_playback_active()) {
        return;
    }

    if (network_connected()) {
        network_event_record(EVENT_RESETCPU, (void *)&mode, sizeof(mode));
    } else {
        event_record(EVENT_RESETCPU, (void *)&mode, sizeof(mode));
        machine_trigger_reset_internal(mode);
    }
}

void machine_reset_event_playback(CLOCK offset, void *data)
{
    machine_trigger_reset_internal(((unsigned int*)data)[0]);
}

void machine_reset(void)
{
    log_message(LOG_DEFAULT, "Main CPU: RESET.");

    /* Do machine-specific initialization.  */
    if (!mem_initialized) {
        mem_powerup();
        mem_initialized = 1;
    }

    machine_specific_reset();

    autostart_reset();

    mem_initialize_memory();

    event_reset_ack();

    vsync_suspend_speed_eval();
}

static void machine_maincpu_clk_overflow_callback(CLOCK sub, void *data)
{
    alarm_context_time_warp(maincpu_alarm_context, sub, -1);
    interrupt_cpu_status_time_warp(maincpu_int_status, sub, -1);
}

void machine_maincpu_init(void)
{
    maincpu_init();
    maincpu_monitor_interface = lib_calloc(1, sizeof(monitor_interface_t));
}

void machine_early_init(void)
{
    maincpu_alarm_context = alarm_context_new("MainCPU");

    maincpu_clk_guard = clk_guard_new(&maincpu_clk, CLOCK_MAX
                                      - CLKGUARD_SUB_MIN);

    clk_guard_add_callback(maincpu_clk_guard,
                           machine_maincpu_clk_overflow_callback, NULL);
}

int machine_init(void)
{
    machine_init_was_called = 1;

    machine_video_init();

    fsdevice_init();
    file_system_init();
    mem_initialize_memory();

    return machine_specific_init();
}

static void machine_maincpu_shutdown(void)
{
    if (maincpu_alarm_context != NULL) {
        alarm_context_destroy(maincpu_alarm_context);
    }
    if (maincpu_clk_guard != NULL) {
        clk_guard_destroy(maincpu_clk_guard);
    }

    lib_free(maincpu_monitor_interface);
    maincpu_shutdown();
}

static void screenshot_at_exit(void)
{
    struct video_canvas_s *canvas;

    if ((ExitScreenshotName == NULL) || (ExitScreenshotName[0] == 0)) {
        return;
    }
    /* FIXME: this always uses the first canvas, for x128/VDC we will need extra handling */
    canvas = machine_video_canvas_get(0);
    /* FIXME: perhaps select driver based on the extension of the given name. for now PNG is good enough :) */
    screenshot_save("PNG", ExitScreenshotName, canvas);
}

void machine_shutdown(void)
{
    if (!machine_init_was_called) {
        /* happens at the -help command line command*/
        return;
    }

    screenshot_at_exit();

    file_system_detach_disk_shutdown();

    machine_specific_shutdown();

    autostart_shutdown();

#ifdef HAS_JOYSTICK
    joystick_close();
#endif

    sound_close();

    printer_shutdown();
    gfxoutput_shutdown();

    fliplist_shutdown();
    file_system_shutdown();
    fsdevice_shutdown();

    tape_shutdown();

    traps_shutdown();

    kbdbuf_shutdown();
    keyboard_shutdown();

    monitor_shutdown();

    console_close_all();

    cmdline_shutdown();

    resources_shutdown();

    drive_shutdown();

    machine_maincpu_shutdown();

    video_shutdown();

    ui_shutdown();

    sysfile_shutdown();

    log_close_all();

    event_shutdown();

    network_shutdown();

    autostart_resources_shutdown();
    sound_resources_shutdown();
    video_resources_shutdown();
    machine_resources_shutdown();
    sysfile_resources_shutdown();
    zfile_shutdown();
    ui_resources_shutdown();
    log_resources_shutdown();
    fliplist_resources_shutdown();
    romset_resources_shutdown();
#ifdef HAVE_NETWORK
    monitor_network_resources_shutdown();
#endif
    archdep_shutdown();

    lib_debug_check();
}

/* --------------------------------------------------------- */
/* Resources & cmdline */

static int set_jam_action(int val, void *param)
{
    switch (val) {
        case MACHINE_JAM_ACTION_DIALOG:
        case MACHINE_JAM_ACTION_CONTINUE:
        case MACHINE_JAM_ACTION_MONITOR:
        case MACHINE_JAM_ACTION_RESET:
        case MACHINE_JAM_ACTION_HARD_RESET:
        case MACHINE_JAM_ACTION_QUIT:
            break;
        default:
            return -1;
    }

    jam_action = val;

    return 0;
}

static int set_exit_screenshot_name(const char *val, void *param)
{
    if (util_string_set(&ExitScreenshotName, val)) {
        return 0;
    }

    return 0;
}

static resource_string_t resources_string[] = {
    { "ExitScreenshotName", "", RES_EVENT_NO, NULL,
      &ExitScreenshotName, set_exit_screenshot_name, NULL },
    { NULL }
};
static const resource_int_t resources_int[] = {
    { "JAMAction", MACHINE_JAM_ACTION_DIALOG, RES_EVENT_SAME, NULL,
      &jam_action, set_jam_action, NULL },
    { NULL }
};

int machine_common_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void machine_common_resources_shutdown(void)
{
    lib_free(ExitScreenshotName);
}

static const cmdline_option_t cmdline_options[] = {
    { "-jamaction", SET_RESOURCE, 1, NULL, NULL, "JAMAction", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID, IDCLS_P_TYPE, IDCLS_SET_MACHINE_JAM_ACTION,
      NULL, NULL },
    { "-exitscreenshot", SET_RESOURCE, 1, NULL, NULL, "ExitScreenshotName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID, IDCLS_P_NAME, IDCLS_SET_EXIT_SCREENSHOT,
      NULL, NULL },
    { NULL }
};

int machine_common_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

