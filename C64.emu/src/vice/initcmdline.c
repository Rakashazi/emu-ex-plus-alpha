/*
 * initcmdline.c - Initial command line options.
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

/* #define DEBUG_CMDLINE */

#include "vice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "cmdline.h"
#include "initcmdline.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "tape.h"
#include "translate.h"
#include "util.h"
#include "vicefeatures.h"

#ifdef DEBUG_CMDLINE
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

static char *autostart_string = NULL;
static char *startup_disk_images[4];
static char *startup_tape_image;
static unsigned int autostart_mode = AUTOSTART_MODE_NONE;

int cmdline_get_autostart_mode(void)
{
    return autostart_mode;
}

static void cmdline_free_autostart_string(void)
{
    lib_free(autostart_string);
    autostart_string = NULL;
}

static void cmdline_free_startup_images(void)
{
    int unit;

    for (unit = 0; unit < 4; unit++) {
        if (startup_disk_images[unit] != NULL) {
            lib_free(startup_disk_images[unit]);
        }
        startup_disk_images[unit] = NULL;
    }
    if (startup_tape_image != NULL) {
        lib_free(startup_tape_image);
    }
    startup_tape_image = NULL;
}

static int cmdline_help(const char *param, void *extra_param)
{
    cmdline_show_help(NULL);
    exit(0);

    return 0;   /* OSF1 cc complains */
}

static int cmdline_features(const char *param, void *extra_param)
{
    feature_list_t *list = vice_get_feature_list();

    printf("Compile time options:\n");
    while (list->symbol) {
        printf("%-25s %4s %s\n", list->symbol, list->isdefined ? "yes " : "no  ", list->descr);
        ++list;
    }

    exit(0);
    return 0;   /* OSF1 cc complains */
}

static int cmdline_config(const char *param, void *extra_param)
{
    /* "-config" needs to be handled before this gets called
       but it also needs to be registered as a cmdline option,
       hence this kludge. */
    return 0;
}

static int cmdline_dumpconfig(const char *param, void *extra_param)
{
    return resources_dump(param);
}

static int cmdline_default(const char *param, void *extra_param)
{
    return resources_set_defaults();
}

static int cmdline_chdir(const char *param, void *extra_param)
{
    return ioutil_chdir(param);
}

static int cmdline_limitcycles(const char *param, void *extra_param)
{
    maincpu_clk_limit = strtoul(param, NULL, 0);
    return 0;
}

static int cmdline_autostart(const char *param, void *extra_param)
{
    cmdline_free_autostart_string();
    autostart_string = lib_stralloc(param);
    autostart_mode = AUTOSTART_MODE_RUN;
    return 0;
}

static int cmdline_autoload(const char *param, void *extra_param)
{
    cmdline_free_autostart_string();
    autostart_string = lib_stralloc(param);
    autostart_mode = AUTOSTART_MODE_LOAD;
    return 0;
}

#if !defined(__OS2__) && !defined(__BEOS__)
static int cmdline_console(const char *param, void *extra_param)
{
    console_mode = 1;
    video_disabled_mode = 1;
    return 0;
}
#endif


static int cmdline_attach(const char *param, void *extra_param)
{
    int unit = vice_ptr_to_int(extra_param);

    switch (unit) {
        case 1:
            lib_free(startup_tape_image);
            startup_tape_image = lib_stralloc(param);
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            lib_free(startup_disk_images[unit - 8]);
            startup_disk_images[unit - 8] = lib_stralloc(param);
            break;
        default:
            archdep_startup_log_error("cmdline_attach(): unexpected unit number %d?!\n", unit);
    }

    return 0;
}

static const cmdline_option_t common_cmdline_options[] = {
    { "-help", CALL_FUNCTION, 0,
      cmdline_help, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SHOW_COMMAND_LINE_OPTIONS,
      NULL, NULL },
    { "-?", CALL_FUNCTION, 0,
      cmdline_help, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SHOW_COMMAND_LINE_OPTIONS,
      NULL, NULL },
    { "-h", CALL_FUNCTION, 0,
      cmdline_help, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SHOW_COMMAND_LINE_OPTIONS,
      NULL, NULL },
    { "-features", CALL_FUNCTION, 0,
      cmdline_features, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SHOW_COMPILETIME_FEATURES,
      NULL, NULL },
    { "-default", CALL_FUNCTION, 0,
      cmdline_default, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RESTORE_DEFAULT_SETTINGS,
      NULL, NULL },
    { "-config", CALL_FUNCTION, 1,
      cmdline_config, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_FILE, IDCLS_SPECIFY_CONFIG_FILE,
      NULL, NULL },
    { "-dumpconfig", CALL_FUNCTION, 1,
      cmdline_dumpconfig, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_FILE, IDCLS_SPECIFY_DUMPCONFIG_FILE,
      NULL, NULL },
    { "-chdir", CALL_FUNCTION, 1,
      cmdline_chdir, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDGS_P_DIRECTORY, IDGS_MON_CD_DESCRIPTION,
      NULL, NULL },
    { "-limitcycles", CALL_FUNCTION, 1,
      cmdline_limitcycles, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_LIMIT_CYCLES,
      NULL, NULL },
#if (!defined  __OS2__ && !defined __BEOS__)
    { "-console", CALL_FUNCTION, 0,
      cmdline_console, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_CONSOLE_MODE,
      NULL, NULL },
    { "-core", SET_RESOURCE, 0,
      NULL, NULL, "DoCoreDump", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_CORE_DUMPS,
      NULL, NULL },
    { "+core", SET_RESOURCE, 0,
      NULL, NULL, "DoCoreDump", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_ALLOW_CORE_DUMPS,
      NULL, NULL },
#else
    { "-debug", SET_RESOURCE, 0,
      NULL, NULL, "DoCoreDump", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_CALL_EXCEPTION_HANDLER,
      NULL, NULL },
    { "+debug", SET_RESOURCE, 0,
      NULL, NULL, "DoCoreDump", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_CALL_EXCEPTION_HANDLER,
      NULL, NULL },
#endif
    { NULL }
};

/* These are the command-line options for the initialization sequence.  */

static const cmdline_option_t cmdline_options[] = {
    { "-autostart", CALL_FUNCTION, 1,
      cmdline_autostart, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AND_AUTOSTART,
      NULL, NULL },
    { "-autoload", CALL_FUNCTION, 1,
      cmdline_autoload, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AND_AUTOLOAD,
      NULL, NULL },
    { "-1", CALL_FUNCTION, 1,
      cmdline_attach, (void *)1, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AS_TAPE,
      NULL, NULL },
    { "-8", CALL_FUNCTION, 1,
      cmdline_attach, (void *)8, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AS_DISK_8,
      NULL, NULL },
    { "-9", CALL_FUNCTION, 1,
      cmdline_attach, (void *)9, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AS_DISK_9,
      NULL, NULL },
    { "-10", CALL_FUNCTION, 1,
      cmdline_attach, (void *)10, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AS_DISK_10,
      NULL, NULL },
    { "-11", CALL_FUNCTION, 1,
      cmdline_attach, (void *)11, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_ATTACH_AS_DISK_11,
      NULL, NULL },
    { NULL }
};

int initcmdline_init(void)
{
    if (cmdline_register_options(common_cmdline_options) < 0) {
        return -1;
    }

    /* Disable autostart options for vsid */
    if (machine_class != VICE_MACHINE_VSID) {
        if (cmdline_register_options(cmdline_options) < 0) {
            return -1;
        }
    }

    atexit(cmdline_free_startup_images);

    return 0;
}

int initcmdline_check_psid(void)
{
    /* Check for PSID here since we don't want to allow autodetection
       in autostart.c. */
    if (machine_class == VICE_MACHINE_VSID) {
        if (autostart_string != NULL
            && machine_autodetect_psid(autostart_string) == -1) {
            log_error(LOG_DEFAULT, "`%s' is not a valid PSID file.",
                      autostart_string);
            return -1;
        }
    }

    return 0;
}

int initcmdline_check_args(int argc, char **argv)
{
    DBG(("initcmdline_check_args (argc:%d)\n", argc));
    if (cmdline_parse(&argc, argv) < 0) {
        archdep_startup_log_error("Error parsing command-line options, bailing out. For help use '-help'\n");
        return -1;
    }
    DBG(("initcmdline_check_args 1 (argc:%d)\n", argc));

    /* The last orphan option is the same as `-autostart'.  */
    if ((argc > 1) && (autostart_string == NULL)) {
        autostart_string = lib_stralloc(argv[1]);
        autostart_mode = AUTOSTART_MODE_RUN;
        argc--, argv++;
    }
    DBG(("initcmdline_check_args 2 (argc:%d)\n", argc));

    if (argc > 1) {
        int len = 0, j;

        for (j = 1; j < argc; j++) {
            len += argv[j] ? (int)strlen(argv[j]) : 0;
        }

        {
            char *txt = lib_calloc(1, len + argc + 1);
            for (j = 1; j < argc; j++) {
                if (argv[j]) {
                    strcat(strcat(txt, " "), argv[j]);
                }
            }
            archdep_startup_log_error("Extra arguments on command-line: %s\n",
                                      txt);
            lib_free(txt);
        }
        return -1;
    }

    return 0;
}

void initcmdline_check_attach(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        /* Handle general-purpose command-line options.  */

        /* `-autostart' */
        if (autostart_string != NULL) {
            autostart_autodetect_opt_prgname(autostart_string, 0, autostart_mode);
        }
        /* `-8', `-9', `-10' and `-11': Attach specified disk image.  */
        {
            int i;

            for (i = 0; i < 4; i++) {
                if (startup_disk_images[i] != NULL
                    && file_system_attach_disk(i + 8, startup_disk_images[i])
                    < 0) {
                    log_error(LOG_DEFAULT,
                              "Cannot attach disk image `%s' to unit %d.",
                              startup_disk_images[i], i + 8);
                }
            }
        }

        /* `-1': Attach specified tape image.  */
        if (startup_tape_image && tape_image_attach(1, startup_tape_image) < 0) {
            log_error(LOG_DEFAULT, "Cannot attach tape image `%s'.",
                      startup_tape_image);
        }
    }

    cmdline_free_autostart_string();
}
