/*
 * main.c - VICE main startup entry.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Vesa-Matti Puro <vmp@lut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

/* #define DEBUG_MAIN */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ENABLE_NLS
#include <locale.h>
#endif

#include "archdep.h"
#include "cmdline.h"
#include "console.h"
#include "debug.h"
#include "drive.h"
#include "fullscreen.h"
#include "gfxoutput.h"
#include "info.h"
#include "init.h"
#include "initcmdline.h"
#ifdef HAS_TRANSLATION
#include "intl.h"
#endif
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "main.h"
#include "platform.h"
#include "resources.h"
#include "sysfile.h"
#ifdef HAS_TRANSLATION
#include "translate.h"
#endif
#include "types.h"
#include "uiapi.h"
#include "version.h"
#include "video.h"

#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif

#ifdef DEBUG_MAIN
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#ifdef __OS2__
const
#endif
int console_mode = 0;
int video_disabled_mode = 0;
static int init_done = 0;

/* ------------------------------------------------------------------------- */

/* This is the main program entry point.  Call this from `main()'.  */
int main_program(int argc, char **argv)
{
    int i, n;
    char *program_name;
    char *tmp;
    int ishelp = 0;

    lib_init_rand();

    /* Check for -config and -console before initializing the user interface.
       -config  => use specified configuration file
       -console => no user interface
    */
    DBG(("main:early cmdline(argc:%d)\n", argc));
    for (i = 0; i < argc; i++) {
#ifndef __OS2__
        if ((!strcmp(argv[i], "-console")) || (!strcmp(argv[i], "--console"))) {
            console_mode = 1;
            video_disabled_mode = 1;
        } else
#endif
        if ((!strcmp(argv[i], "-config")) || (!strcmp(argv[i], "--config"))) {
            if ((i + 1) < argc) {
                vice_config_file = lib_stralloc(argv[++i]);
            }
        } else if ((!strcmp(argv[i], "-help")) ||
                   (!strcmp(argv[i], "--help")) ||
                   (!strcmp(argv[i], "-h")) ||
                   (!strcmp(argv[i], "-?"))) {
            ishelp = 1;
        }
    }

#ifdef ENABLE_NLS
    /* gettext stuff, not needed in Gnome, but here I can
       overrule the default locale path */
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, NLS_LOCALEDIR);
    textdomain(PACKAGE);
#endif

    DBG(("main:archdep_init(argc:%d)\n", argc));
    if (archdep_init(&argc, argv) != 0) {
        archdep_startup_log_error("archdep_init failed.\n");
        return -1;
    }

    if (atexit(main_exit) < 0) {
        archdep_startup_log_error("atexit failed.\n");
        return -1;
    }

    maincpu_early_init();
    machine_setup_context();
    drive_setup_context();
    machine_early_init();

    /* Initialize system file locator.  */
    sysfile_init(machine_name);

    gfxoutput_early_init(ishelp);
    if ((init_resources() < 0) || (init_cmdline_options() < 0)) {
        return -1;
    }

    /* Set factory defaults.  */
    if (resources_set_defaults() < 0) {
        archdep_startup_log_error("Cannot set defaults.\n");
        return -1;
    }

    /* Initialize the user interface.  `ui_init()' might need to handle the
       command line somehow, so we call it before parsing the options.
       (e.g. under X11, the `-display' option is handled independently).  */
    DBG(("main:ui_init(argc:%d)\n", argc));
    if (!console_mode && ui_init(&argc, argv) < 0) {
        archdep_startup_log_error("Cannot initialize the UI.\n");
        return -1;
    }

#ifdef HAS_TRANSLATION
    /* set the default arch language */
    translate_arch_language_init();
#endif

    if (!ishelp) {
        /* Load the user's default configuration file.  */
        if (resources_load(NULL) < 0) {
            /* The resource file might contain errors, and thus certain
            resources might have been initialized anyway.  */
            if (resources_set_defaults() < 0) {
                archdep_startup_log_error("Cannot set defaults.\n");
                return -1;
            }
        }
    }

    if (log_init() < 0) {
        archdep_startup_log_error("Cannot startup logging system.\n");
    }

    DBG(("main:initcmdline_check_args(argc:%d)\n", argc));
    if (initcmdline_check_args(argc, argv) < 0) {
        return -1;
    }

    program_name = archdep_program_name();

    /* VICE boot sequence.  */
    log_message(LOG_DEFAULT, " ");
#ifdef USE_SVN_REVISION
    log_message(LOG_DEFAULT, "*** VICE Version %s, rev %s ***", VERSION, VICE_SVN_REV_STRING);
#else
    log_message(LOG_DEFAULT, "*** VICE Version %s ***", VERSION);
#endif
    log_message(LOG_DEFAULT, "OS compiled for: %s", platform_get_compile_time_os());
    log_message(LOG_DEFAULT, "GUI compiled for: %s", platform_get_ui());
    log_message(LOG_DEFAULT, "CPU compiled for: %s", platform_get_compile_time_cpu());
    log_message(LOG_DEFAULT, "Compiler used: %s", platform_get_compile_time_compiler());
    log_message(LOG_DEFAULT, "Current OS: %s", platform_get_runtime_os());
    log_message(LOG_DEFAULT, "Current CPU: %s", platform_get_runtime_cpu());
    log_message(LOG_DEFAULT, " ");
    if (machine_class == VICE_MACHINE_VSID) {
        log_message(LOG_DEFAULT, "Welcome to %s, the free portable SID Player.",
                    program_name);
    } else {
        log_message(LOG_DEFAULT, "Welcome to %s, the free portable %s Emulator.",
                    program_name, machine_name);
    }
    log_message(LOG_DEFAULT, " ");

    log_message(LOG_DEFAULT, "Current VICE team members:");
    tmp = lib_malloc(80);
    n = 0; *tmp = 0;
    for (i = 0; core_team[i].name; i++) {
        n += strlen(core_team[i].name);
        if (n > 74) {
            log_message(LOG_DEFAULT, "%s", tmp);
            n = 0; *tmp = 0;
        }
        strcat(tmp, core_team[i].name);
        if (core_team[i + 1].name) {
            strcat(tmp, ", ");
        } else {
            strcat(tmp, ".");
            log_message(LOG_DEFAULT, "%s", tmp);
        }
    }
    lib_free(tmp);

    log_message(LOG_DEFAULT, " ");
    log_message(LOG_DEFAULT, "This is free software with ABSOLUTELY NO WARRANTY.");
    log_message(LOG_DEFAULT, "See the \"About VICE\" command for more info.");
    log_message(LOG_DEFAULT, " ");

    lib_free(program_name);

    /* Complete the GUI initialization (after loading the resources and
       parsing the command-line) if necessary.  */
    if (!console_mode && ui_init_finish() < 0) {
        return -1;
    }

    if (!console_mode && video_init() < 0) {
        return -1;
    }

    if (initcmdline_check_psid() < 0) {
        return -1;
    }

    if (init_main() < 0) {
        return -1;
    }

    initcmdline_check_attach();

    init_done = 1;

    /* Let's go...  */
    log_message(LOG_DEFAULT, "Main CPU: starting at ($FFFC).");
    maincpu_mainloop();

    log_error(LOG_DEFAULT, "perkele!");

    return 0;
}
