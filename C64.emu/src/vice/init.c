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
#include "autostart.h"
#include "cmdline.h"
#include "console.h"
#include "debug.h"
#include "diskimage.h"
#include "drive.h"
#include "drivecpu.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "initcmdline.h"
#include "joy.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "log.h"
#include "machine-bus.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#ifdef HAVE_NETWORK
#include "monitor_network.h"
#endif
#include "network.h"
#include "palette.h"
#include "ram.h"
#include "resources.h"
#include "romset.h"
#include "screenshot.h"
#include "signals.h"
#include "sysfile.h"
#include "uiapi.h"
#include "vdrive.h"
#include "vice-event.h"


static void init_resource_fail(const char *module)
{
    archdep_startup_log_error("Cannot initialize %s resources.\n",
                              module);
}

int init_resources(void)
{
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
    if (autostart_resources_init() < 0) {
        init_resource_fail("autostart");
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
    if (debug_resources_init() < 0) {
        init_resource_fail("debug");
        return -1;
    }
    if (machine_common_resources_init() < 0) {
        init_resource_fail("machine common");
        return -1;
    }
    if (machine_resources_init() < 0) {
        init_resource_fail("machine");
        return -1;
    }
    if (joystick_init_resources() < 0) {
        init_resource_fail("joystick");
        return -1;
    }
    if (ram_resources_init() < 0) {
        init_resource_fail("RAM");
        return -1;
    }
    if (gfxoutput_resources_init() < 0) {
        init_resource_fail("GFXOUTPUT");
        return -1;
    }
    if (network_resources_init() < 0) {
        init_resource_fail("network");
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

static void init_cmdline_options_fail(const char *module)
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
        if (autostart_cmdline_options_init() < 0) {
            init_resource_fail("autostart");
            return -1;
        }
        if (romset_cmdline_options_init() < 0) {
            init_cmdline_options_fail("romset");
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
        if (disk_image_cmdline_options_init() < 0) {
            init_cmdline_options_fail("disk image");
            return -1;
        }
        if (event_cmdline_options_init() < 0) {
            init_cmdline_options_fail("event");
            return -1;
        }
    }
    if (monitor_cmdline_options_init() < 0) {
        init_cmdline_options_fail("monitor");
        return -1;
    }
#ifdef DEBUG
    if (debug_cmdline_options_init() < 0) {
        init_cmdline_options_fail("debug");
        return -1;
    }
#endif
    if (machine_common_cmdline_options_init() < 0) {
        init_cmdline_options_fail("machine common");
        return -1;
    }
    if (machine_cmdline_options_init() < 0) {
        init_cmdline_options_fail("machine");
        return -1;
    }

    if (machine_class != VICE_MACHINE_VSID) {
        if (fsdevice_cmdline_options_init() < 0) {
            init_cmdline_options_fail("file system");
            return -1;
        }
    }
    if (!video_disabled_mode && joystick_init_cmdline_options() < 0) {
        init_cmdline_options_fail("joystick");
        return -1;
    }
    if (machine_class != VICE_MACHINE_VSID) {
        if (kbdbuf_cmdline_options_init() < 0) {
            init_cmdline_options_fail("keyboard");
            return -1;
        }
        if (ram_cmdline_options_init() < 0) {
            init_cmdline_options_fail("RAM");
            return -1;
        }
        if (gfxoutput_cmdline_options_init() < 0) {
            init_cmdline_options_fail("GFXOUTPUT");
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
        gfxoutput_init();
        screenshot_init();

        drivecpu_early_init_all();
    }

    machine_bus_init();
    machine_maincpu_init();

    event_init();

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

    if (!video_disabled_mode) {
        joystick_init();
    }

    if (machine_class != VICE_MACHINE_VSID) {
        disk_image_init();
        vdrive_init();
    }

    ui_init_finalize();

    return 0;
}
