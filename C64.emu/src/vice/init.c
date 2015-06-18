/*
 * init.c - General initialization.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "archdep.h"
#include "attach.h"
#include "cmdline.h"
#include "console.h"
#include "drive.h"
#include "initcmdline.h"
#include "keyboard.h"
#include "log.h"
#include "machine-bus.h"
#include "machine-video.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#ifdef HAVE_NETWORK
#include "monitor_network.h"
#endif
#include "palette.h"
#include "ram.h"
#include "resources.h"
#include "romset.h"
#include "screenshot.h"
#include "signals.h"
#include "sysfile.h"
#include "uiapi.h"
#include "vdrive.h"
#include "video.h"
#include "vsync.h"

/* #define DBGINIT */

#ifdef DBGINIT
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

void init_resource_fail(const char *module)
{
    archdep_startup_log_error("Cannot initialize %s resources.\n",
                              module);
}

int init_resources(void)
{
    DBG(("init_resources\n"));
    if (resources_init(machine_get_name())) {
        archdep_startup_log_error("Cannot initialize resource handling.\n");
        return -1;
    }
    if (log_resources_init() < 0) {
        init_resource_fail("log");
        return -1;
    }
    if (sysfile_resources_init() < 0) {
        init_resource_fail("system file locator");
        return -1;
    }
    if (romset_resources_init() < 0) {
        init_resource_fail("romset");
        return -1;
    }
    if (ui_resources_init() < 0) {
        init_resource_fail("UI");
        return -1;
    }
    if (machine_common_resources_init() < 0) {
        init_resource_fail("machine common");
        return -1;
    }
    if (vsync_resources_init() < 0) {
        init_resource_fail("vsync");
        return -1;
    }
    if (sound_resources_init() < 0) {
        init_resource_fail("sound");
        return -1;
    }
    if (keyboard_resources_init() < 0) {
        init_resource_fail("keyboard");
        return -1;
    }
    if (machine_video_resources_init() < 0) {
        init_resource_fail("machine video");
        return -1;
    }
    if (machine_resources_init() < 0) {
        init_resource_fail("machine");
        return -1;
    }
    if (ram_resources_init() < 0) {
        init_resource_fail("RAM");
        return -1;
    }
    if (monitor_resources_init() < 0) {
        init_resource_fail("monitor");
        return -1;
    }
#ifdef HAVE_NETWORK
    if (monitor_network_resources_init() < 0) {
        init_resource_fail("MONITOR_NETWORK");
        return -1;
    }
#endif
    return 0;
}

void init_cmdline_options_fail(const char *module)
{
    archdep_startup_log_error("Cannot initialize %s command-line options.\n",
                              module);
}

int init_cmdline_options(void)
{
    if (cmdline_init()) {
        archdep_startup_log_error("Cannot initialize command-line handling.\n");
        return -1;
    }
    if (log_cmdline_options_init() < 0) {
        init_cmdline_options_fail("log");
        return -1;
    }
    if (initcmdline_init() < 0) {
        init_cmdline_options_fail("main");
        return -1;
    }
    if (sysfile_cmdline_options_init() < 0) {
        init_cmdline_options_fail("system file locator");
        return -1;
    }
    if (!video_disabled_mode && ui_cmdline_options_init() < 0) {
        init_cmdline_options_fail("UI");
        return -1;
    }
    if (machine_class != VICE_MACHINE_VSID) {
        if (romset_cmdline_options_init() < 0) {
            init_cmdline_options_fail("romset");
            return -1;
        }
    }
    if (monitor_cmdline_options_init() < 0) {
        init_cmdline_options_fail("monitor");
        return -1;
    }
    if (machine_common_cmdline_options_init() < 0) {
        init_cmdline_options_fail("machine common");
        return -1;
    }
    if (vsync_cmdline_options_init() < 0) {
        init_cmdline_options_fail("vsync");
        return -1;
    }
    if (sound_cmdline_options_init() < 0) {
        init_cmdline_options_fail("sound");
        return -1;
    }
#ifdef COMMON_KBD
    if (keyboard_cmdline_options_init() < 0) {
        init_cmdline_options_fail("keyboard");
        return -1;
    }
#endif
    if (video_cmdline_options_init() < 0) {
        init_cmdline_options_fail("video");
        return -1;
    }
    if (machine_cmdline_options_init() < 0) {
        init_cmdline_options_fail("machine");
        return -1;
    }

    if (machine_class != VICE_MACHINE_VSID) {
        if (ram_cmdline_options_init() < 0) {
            init_cmdline_options_fail("RAM");
            return -1;
        }
    }
#ifdef HAVE_NETWORK
    if (monitor_network_cmdline_options_init() < 0) {
        init_cmdline_options_fail("MONITOR_NETWORK");
        return -1;
    }
#endif
    return 0;
}

int init_main(void)
{
    signals_init(debug.do_core_dumps);

    romset_init();

    if (!video_disabled_mode) {
        palette_init();
    }

    if (machine_class != VICE_MACHINE_VSID) {
        screenshot_init();

        drive_cpu_early_init_all();
    }

    machine_bus_init();
    machine_maincpu_init();

    /* Machine-specific initialization.  */
    if (machine_init() < 0) {
        log_error(LOG_DEFAULT, "Machine initialization failed.");
        return -1;
    }

    /* FIXME: what's about uimon_init??? */
    /* the monitor console MUST be available, because of for example cpujam,
       or -initbreak from cmdline.
    */
    if (console_init() < 0) {
        log_error(LOG_DEFAULT, "Console initialization failed.");
        return -1;
    }

    keyboard_init();

    if (machine_class != VICE_MACHINE_VSID) {
        vdrive_init();
    }

    ui_init_finalize();

    return 0;
}
