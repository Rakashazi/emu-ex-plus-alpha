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
#include <stdbool.h>

#include "alarm.h"
#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "cmdline.h"
#include "console.h"
#include "diskimage.h"
#include "drive.h"
#include "vice-event.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "initcmdline.h"
#include "interrupt.h"
#include "joystick.h"
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
#include "monitor_binary.h"
#include "network.h"
#include "palette.h"
#include "printer.h"
#include "profiler.h"
#include "resources.h"
#include "romset.h"
#include "screenshot.h"
#include "sound.h"
#include "sysfile.h"
#include "tape.h"
#include "traps.h"
#include "types.h"
#include "uiapi.h"
#include "uiactions.h"
#include "uihotkeys.h"
#include "util.h"
#include "video.h"
#include "vsync.h"
#include "zfile.h"

/* #define DEBUGMACHINE */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifdef DEBUGMACHINE
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int machine_init_was_called = 0;
static int mem_initialized = 0;
static bool is_jammed = false;
static char *jam_reason = NULL;
static int jam_action = MACHINE_JAM_ACTION_DIALOG;
int machine_keymap_index;
static char *ExitScreenshotName = NULL;
static char *ExitScreenshotName1 = NULL;
static bool is_first_reset = true;

/* NOTE: this function is very similar to drive_jam - in case the behavior
         changes, change drive_jam too */
unsigned int machine_jam(const char *format, ...)
{
    va_list ap;
    ui_jam_action_t ret = JAM_NONE;

    /* always ignore subsequent JAMs. reset would clear the flag again, not
     * setting it when going to the monitor would just repeatedly pop up the
     * jam dialog (until reset)
     */
    if (is_jammed) {
        return JAM_NONE;
    }

    is_jammed = true;

    va_start(ap, format);
    if (jam_reason) {
        lib_free(jam_reason);
        jam_reason = NULL;
    }
    jam_reason = lib_mvsprintf(format, ap);
    va_end(ap);

    log_message(LOG_DEFAULT, "*** %s", jam_reason);

    vsync_suspend_speed_eval();
    sound_suspend();

    if (jam_action == MACHINE_JAM_ACTION_DIALOG) {
        if (monitor_is_remote() || monitor_is_binary()) {
            if (monitor_is_remote()) {
                ret = monitor_network_ui_jam_dialog("%s", jam_reason);
            }

            if (monitor_is_binary()) {
                ret = monitor_binary_ui_jam_dialog("%s", jam_reason);
            }
        } else if (!console_mode) {
            ret = ui_jam_dialog("%s", jam_reason);
        }
    } else if (jam_action == MACHINE_JAM_ACTION_QUIT) {
        archdep_vice_exit(EXIT_SUCCESS);
    } else {
        int actions[4] = {
            -1, UI_JAM_MONITOR, UI_JAM_RESET_CPU, UI_JAM_POWER_CYCLE
        };
        ret = actions[jam_action - 1];
    }

    switch (ret) {
        case UI_JAM_RESET_CPU:
            return JAM_RESET_CPU;
        case UI_JAM_POWER_CYCLE:
            return JAM_POWER_CYCLE;
        case UI_JAM_MONITOR:
            return JAM_MONITOR;
        default:
            break;
    }
    return JAM_NONE;
}

bool machine_is_jammed(void)
{
    return is_jammed;
}

char *machine_jam_reason(void)
{
    return jam_reason;
}

static void machine_trigger_reset_internal(const unsigned int mode)
{
    is_jammed = false;

    if (jam_reason) {
        lib_free(jam_reason);
        jam_reason = NULL;
    }

    switch (mode) {
        case MACHINE_RESET_MODE_POWER_CYCLE:
            mem_initialized = 0; /* force memory initialization */
            machine_specific_powerup();
        /* Fall through.  */
        case MACHINE_RESET_MODE_RESET_CPU:
            maincpu_trigger_reset();
            break;
    }

    ui_display_reset(0, mode);
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

    is_jammed = false;

    if (jam_reason) {
        lib_free(jam_reason);
        jam_reason = NULL;
    }

    /* Do machine-specific initialization.  */
    if (!mem_initialized) {
        mem_powerup();
        mem_initialized = 1;
    }

    machine_specific_reset();

    autostart_reset();

    mem_initialize_memory();

    event_reset_ack();

    /* Give the monitor a chance to break immediately */
    monitor_reset_hook();

    vsync_reset_hook();

    /* If this is the first machine reset, kick off any requested autostart */
    if (is_first_reset) {
        is_first_reset = false;
        initcmdline_check_attach();
    }
}

void machine_maincpu_init(void)
{
    maincpu_init();
    maincpu_monitor_interface = lib_calloc(1, sizeof(monitor_interface_t));
}

void machine_early_init(void)
{
    maincpu_alarm_context = alarm_context_new("MainCPU");
}

int machine_init(void)
{
    machine_init_was_called = 1;

    fsdevice_init();
    file_system_init();
    mem_initialize_memory();

    return machine_specific_init();
}

void machine_maincpu_shutdown(void)
{
    if (maincpu_alarm_context != NULL) {
        alarm_context_destroy(maincpu_alarm_context);
    }

    lib_free(maincpu_monitor_interface);
    maincpu_shutdown();

    if (jam_reason) {
        lib_free(jam_reason);
        jam_reason = NULL;
    }
}

static void screenshot_at_exit(void)
{
    struct video_canvas_s *canvas;

    if ((ExitScreenshotName != NULL) && (ExitScreenshotName[0] != 0)) {
        /* FIXME: this always uses the first canvas, for x128 this is the VDC */
        canvas = machine_video_canvas_get(0);
        /* FIXME: perhaps select driver based on the extension of the given name. for now PNG is good enough :) */
        screenshot_save("PNG", ExitScreenshotName, canvas);
    }
    if (machine_class == VICE_MACHINE_C128) {
        if ((ExitScreenshotName1 != NULL) && (ExitScreenshotName1[0] != 0)) {
            /* FIXME: this always uses the second canvas, for x128 this is the VICII */
            canvas = machine_video_canvas_get(1);
            /* FIXME: perhaps select driver based on the extension of the given name. for now PNG is good enough :) */
            screenshot_save("PNG", ExitScreenshotName1, canvas);
        }
    }
}

void machine_shutdown(void)
{
    int save_on_exit;

    if (!machine_init_was_called) {
        /* happens at the -help command line command*/
        return;
    }

    /*
     * Avoid SoundRecordDeviceName being written to vicerc when save-on-exit
     * is enabled. If recording is/was active vicerc will contain some setting
     * for this resource and display an error.
     */
    sound_stop_recording();

    resources_get_int("SaveResourcesOnExit", &save_on_exit);
    if (save_on_exit) {
        /*
         * FIXME: I tried moving this to resources_shutdown, but if you try to save
         * resources after machine_specific_shutdown() is called then it crashes.
         * That's a bit of a code smell to me. --dqh 2020-08-01
         */
        resources_save(NULL);
    }

    screenshot_at_exit();
    screenshot_shutdown();

    file_system_detach_disk_shutdown();

    machine_specific_shutdown();

    autostart_shutdown();

    joystick_close();
#ifdef MAC_JOYSTICK
    joy_hidlib_exit();
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
    initcmdline_shutdown();

    resources_shutdown();

    drive_shutdown();

    machine_maincpu_shutdown();

    profile_shutdown();

    video_shutdown();

    if (!console_mode) {
        ui_shutdown();
#ifndef USE_HEADLESSUI
        ui_actions_shutdown();
        ui_hotkeys_shutdown();
#endif
    }

    palette_shutdown();

    sysfile_shutdown();

    log_close_all();

    event_shutdown();

    network_shutdown();

    autostart_resources_shutdown();
    sound_resources_shutdown();
    video_resources_shutdown();
    machine_resources_shutdown();
    machine_common_resources_shutdown();

    vsync_shutdown();

    joystick_resources_shutdown();
    sysfile_resources_shutdown();
    zfile_shutdown();
    ui_resources_shutdown();
    log_resources_shutdown();
    fliplist_resources_shutdown();
    romset_resources_shutdown();
#ifdef HAVE_NETWORK
    monitor_network_resources_shutdown();
    monitor_binary_resources_shutdown();
#endif
    monitor_resources_shutdown();

    archdep_shutdown();
}

/* --------------------------------------------------------- */
/* Resources & cmdline */

static int set_jam_action(int val, void *param)
{
    switch (val) {
        case MACHINE_JAM_ACTION_DIALOG:
        case MACHINE_JAM_ACTION_CONTINUE:
        case MACHINE_JAM_ACTION_MONITOR:
        case MACHINE_JAM_ACTION_RESET_CPU:
        case MACHINE_JAM_ACTION_POWER_CYCLE:
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

static int set_exit_screenshot_name1(const char *val, void *param)
{
    if (util_string_set(&ExitScreenshotName1, val)) {
        return 0;
    }

    return 0;
}

static resource_string_t resources_string[] = {
    { "ExitScreenshotName", "", RES_EVENT_NO, NULL,
      &ExitScreenshotName, set_exit_screenshot_name, NULL },
    RESOURCE_STRING_LIST_END
};

static resource_string_t resources_string_c128[] = {
    { "ExitScreenshotName1", "", RES_EVENT_NO, NULL,
      &ExitScreenshotName1, set_exit_screenshot_name1, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "JAMAction", MACHINE_JAM_ACTION_CONTINUE, RES_EVENT_SAME, NULL,
      &jam_action, set_jam_action, NULL },
    RESOURCE_INT_LIST_END
};

int machine_common_resources_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        if (resources_register_string(resources_string) < 0) {
           return -1;
        }
        if (machine_class == VICE_MACHINE_C128) {
            if (resources_register_string(resources_string_c128) < 0) {
            return -1;
            }
        }
    }
    return resources_register_int(resources_int);
}

void machine_common_resources_shutdown(void)
{
    lib_free(ExitScreenshotName);
    lib_free(ExitScreenshotName1);
}

static const cmdline_option_t cmdline_options_c128[] =
{
    { "-jamaction", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JAMAction", NULL,
      "<Type>", "Set action on CPU JAM: (0: Ask, 1: continue, 2: Monitor, 3: Reset, 4: Power cycle, 5: Quit Emulator)" },
    { "-exitscreenshot", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ExitScreenshotName", NULL,
      "<Name>", "Set name of screenshot to save when emulator exits." },
    { "-exitscreenshotvicii", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ExitScreenshotName1", NULL,
      "<Name>", "Set name of screenshot to save when emulator exits." },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options[] =
{
    { "-jamaction", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JAMAction", NULL,
      "<Type>", "Set action on CPU JAM: (0: Ask, 1: continue, 2: Monitor, 3: Reset, 4: Power cycle, 5: Quit Emulator)" },
    { "-exitscreenshot", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ExitScreenshotName", NULL,
      "<Name>", "Set name of screenshot to save when emulator exits." },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_vsid[] =
{
    { "-jamaction", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JAMAction", NULL,
      "<Type>", "Set action on CPU JAM: (0: Ask, 1: continue, 2: Monitor, 3: Reset, 4: Power cycle, 5: Quit Emulator)" },
    CMDLINE_LIST_END
};

int machine_common_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_C128) {
        return cmdline_register_options(cmdline_options_c128);
    } else if (machine_class == VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options_vsid);
    } else {
        return cmdline_register_options(cmdline_options);
    }
}
