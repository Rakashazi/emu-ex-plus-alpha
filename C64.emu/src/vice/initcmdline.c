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
#include "cartridge.h"
#include "cmdline.h"
#include "console.h"
#include "drive.h"
#include "fliplist.h"
#include "fsdevice.h"
#include "gfxoutput.h"
#include "initcmdline.h"
#include "ioutil.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "monitor_binary.h"
#include "monitor_network.h"
#include "network.h"
#include "printer.h"
#include "resources.h"
#include "romset.h"
#include "sysfile.h"
#include "tape.h"
#include "tapeport.h"
#include "traps.h"
#include "uiapi.h"
#include "util.h"
#include "vice-event.h"
#include "vicefeatures.h"
#include "video.h"
#include "vsync.h"
#include "zfile.h"


#ifdef DEBUG_CMDLINE
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define NUM_STARTUP_DISK_IMAGES 8
static char *autostart_string = NULL;
static char *startup_disk_images[NUM_STARTUP_DISK_IMAGES];
static char *startup_tape_image[TAPEPORT_MAX_PORTS];
static unsigned int autostart_mode = AUTOSTART_MODE_NONE;


/** \brief  Get autostart mode
 *
 * \return  autostart mode
 */
int cmdline_get_autostart_mode(void)
{
    return autostart_mode;
}


void cmdline_set_autostart_mode(int mode)
{
    autostart_mode = mode;
}


static void cmdline_free_autostart_string(void)
{
    lib_free(autostart_string);
    autostart_string = NULL;
}

void initcmdline_shutdown(void)
{
    int unit;

    for (unit = 0; unit < NUM_STARTUP_DISK_IMAGES; unit++) {
        if (startup_disk_images[unit] != NULL) {
            lib_free(startup_disk_images[unit]);
        }
        startup_disk_images[unit] = NULL;
    }
    for (unit = 0; unit < TAPEPORT_MAX_PORTS; unit++) {
        if (startup_tape_image[unit] != NULL) {
            lib_free(startup_tape_image[unit]);
        }
        startup_tape_image[unit] = NULL;
    }
}

static int cmdline_help(const char *param, void *extra_param)
{
    cmdline_show_help(NULL);

    /*
     * Clean up memory.
     *
     * (Once this works properly it should be refactored into a separate function
     *  so cmdline_features() can also use this code.
     *
     * Currently still leaks in various drive-related code, such as ieee and
     * drive CPUs, alarm/clock code and drive-related monitor interface(s).
     *
     * Looks like the drive contexts are only half initialized, because when
     * I remove the `if (!drive_init_was_called)` from drive/drive.c I get a
     * nice segfault:
     *
     * wd1770_shutdown (drv=0x0) at ../../../../vice/src/drive/iec/wd1770.c:211
     * 211          lib_free(drv->myname);
     *
     * This can be avoided by properly setting drv->myname to NULL and checking
     * for NULL before calling lib_free(), but I fear there will be a lot of
     * this in the drive code.
     *
     * --compyx
     */

/* FIXME: a hack to prevent -help crashing on the SDL ui.
          This needs to be fixed properly!! */
#if defined(USE_SDLUI) || defined(USE_SDLUI2)
    /* remove any trace of this variable once this is properly fixed! */
    sdl_help_shutdown = 1;
#endif

#if 0
    file_system_detach_disk_shutdown();
#endif
    machine_specific_shutdown();
    printer_shutdown();
    gfxoutput_shutdown();
#if 0
    fliplist_shutdown();
    file_system_shutdown();
    fsdevice_shutdown();
    tape_shutdown();
    traps_shutdown();
    kbdbuf_shutdown();
#endif
    keyboard_shutdown();

    monitor_shutdown();

    console_close_all();

    cmdline_shutdown();
    initcmdline_shutdown();

    resources_shutdown();
    drive_shutdown();

    machine_maincpu_shutdown();
#if 0
    video_shutdown();
    if (!console_mode) {
        ui_shutdown();
    }
#endif

    sysfile_shutdown();
    log_close_all();

    event_shutdown();

    network_shutdown();

    autostart_resources_shutdown();
    sound_resources_shutdown();
#if 0
    video_resources_shutdown();
#endif
    machine_resources_shutdown();
    machine_common_resources_shutdown();

    vsync_shutdown();

    sysfile_resources_shutdown();
#if 0
    zfile_shutdown();
#endif

    ui_resources_shutdown();
    log_resources_shutdown();
    fliplist_resources_shutdown();
#if 0
    romset_resources_shutdown();
#endif
#ifdef HAVE_NETWORK
    monitor_network_resources_shutdown();
    monitor_binary_resources_shutdown();
#endif
    monitor_resources_shutdown();

    archdep_shutdown();
    archdep_vice_exit(0);
    return 0;   /* OSF1 cc complains */
}

static int cmdline_features(const char *param, void *extra_param)
{
    const feature_list_t *list = vice_get_feature_list();

    printf("Compile time options:\n");
    while (list->symbol) {
        printf("%-25s %4s %s\n", list->symbol, list->isdefined ? "yes " : "no  ", list->descr);
        ++list;
    }

    archdep_vice_exit(0);
    return 0;   /* OSF1 cc complains */
}

static int cmdline_config(const char *param, void *extra_param)
{
    /* "-config" needs to be handled before this gets called
       but it also needs to be registered as a cmdline option,
       hence this kludge. */
    return 0;
}

static int cmdline_add_config(const char *param, void *extra_param)
{
    return resources_load(param);
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
    uint64_t clk_limit = strtoull(param, NULL, 0);
    if (clk_limit > CLOCK_MAX) {
        fprintf(stderr, "too many cycles, use max %"PRIu64"\n", CLOCK_MAX);
        return -1;
    }
    maincpu_clk_limit = (CLOCK)clk_limit;
    return 0;
}

static int cmdline_autostart(const char *param, void *extra_param)
{
    cmdline_free_autostart_string();
    autostart_string = lib_strdup(param);
    autostart_mode = AUTOSTART_MODE_RUN;
    return 0;
}

static int cmdline_autoload(const char *param, void *extra_param)
{
    cmdline_free_autostart_string();
    autostart_string = lib_strdup(param);
    autostart_mode = AUTOSTART_MODE_LOAD;
    return 0;
}

#if !defined(__OS2__) && !defined(__BEOS__)
static int cmdline_console(const char *param, void *extra_param)
{
    console_mode = 1;
    /* video_disabled_mode = 1; Breaks exitscreenshot */
    return 0;
}
#endif

static int cmdline_seed(const char *param, void *extra_param)
{
    lib_rand_seed(strtoul(param, NULL, 0));
    return 0;
}

static int cmdline_attach(const char *param, void *extra_param)
{
    int unit = vice_ptr_to_int(extra_param);

    switch (unit) {
        case 1:
            lib_free(startup_tape_image[TAPEPORT_PORT_1]);
            startup_tape_image[TAPEPORT_PORT_1] = lib_strdup(param);
            break;
        case 2:
            if (machine_class == VICE_MACHINE_PET) {
                lib_free(startup_tape_image[TAPEPORT_PORT_2]);
                startup_tape_image[TAPEPORT_PORT_2] = lib_strdup(param);
            } else {
                archdep_startup_log_error("cmdline_attach(): unexpected unit number %d?!\n", unit);
            }
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            lib_free(startup_disk_images[unit - 8]);
            startup_disk_images[unit - 8] = lib_strdup(param);
            break;
        case 64:
        case 65:
        case 66:
        case 67:
            lib_free(startup_disk_images[unit - 64 + 4]);
            startup_disk_images[unit - 64 + 4] = lib_strdup(param);
            break;
        default:
            archdep_startup_log_error("cmdline_attach(): unexpected unit number %d?!\n", unit);
    }

    return 0;
}

static const cmdline_option_t common_cmdline_options[] =
{
    { "-help", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_help, NULL, NULL, NULL,
      NULL, "Show a list of the available options and exit normally" },
    { "-?", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_help, NULL, NULL, NULL,
      NULL, "Show a list of the available options and exit normally" },
    { "-h", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_help, NULL, NULL, NULL,
      NULL, "Show a list of the available options and exit normally" },
    { "-features", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_features, NULL, NULL, NULL,
      NULL, "Show a list of the available compile-time options and their configuration." },
    { "-default", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_default, NULL, NULL, NULL,
      NULL, "Restore default settings" },
    { "-config", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_config, NULL, NULL, NULL,
      "<filename>", "Specify config file" },
    { "-addconfig", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_add_config, NULL, NULL, NULL,
      "<filename>", "Specify extra config file for loading additional resources." },
    { "-dumpconfig", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_dumpconfig, NULL, NULL, NULL,
      "<filename>", "Dump all resources to specified config file" },
    { "-chdir", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_chdir, NULL, NULL, NULL,
      "Directory", "Change current working directory." },
    { "-limitcycles", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_limitcycles, NULL, NULL, NULL,
      "<value>", "Specify number of cycles to run before quitting with an error." },
    { "-console", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cmdline_console, NULL, NULL, NULL,
      NULL, "Console mode (for music playback)" },
    { "-seed", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_seed, NULL, NULL, NULL,
      "<value>", "Set random seed (for debugging)" },
    { "-core", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DoCoreDump", (resource_value_t)1,
      NULL, "Allow production of core dumps" },
    { "+core", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DoCoreDump", (resource_value_t)0,
      NULL, "Do not produce core dumps" },
    CMDLINE_LIST_END
};

/* These are the command-line options for the initialization sequence.  */

static const cmdline_option_t cmdline_options[] =
{
    { "-autostart", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_autostart, NULL, NULL, NULL,
      "<Name>", "Attach and autostart tape/disk image <name>" },
    { "-autoload", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_autoload, NULL, NULL, NULL,
      "<Name>", "Attach and autoload tape/disk image <name>" },
    { "-1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)1, NULL, NULL,
      "<Name>", "Attach <name> as a tape image" },
    { "-8", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)8, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #8" },
    { "-8d1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)64, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #8 drive #1" },
    { "-9", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)9, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #9" },
    { "-9d1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)65, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #9 drive #1" },
    { "-10", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)10, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #10" },
    { "-10d1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)66, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #10 drive #1" },
    { "-11", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)11, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #11" },
    { "-11d1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)67, NULL, NULL,
      "<Name>", "Attach <name> as a disk image in unit #11 drive #1" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_pet_options[] =
{
    { "-2", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_attach, (void *)2, NULL, NULL,
      "<Name>", "Attach <name> as a tape image" },
    CMDLINE_LIST_END
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

    /* Add tape 2 option for pet */
    if (machine_class == VICE_MACHINE_PET) {
        if (cmdline_register_options(cmdline_pet_options) < 0) {
            return -1;
        }
    }

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
        autostart_string = lib_strdup(argv[1]);
        autostart_mode = AUTOSTART_MODE_RUN;
        argc--;
        argv++;
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
            if (autostart_autodetect_opt_prgname(autostart_string, 0, autostart_mode) < 0) {
                log_error(LOG_DEFAULT,
                        "Failed to autostart '%s'", autostart_string);
                if (autostart_string != NULL) {
                    lib_free(autostart_string);
                }
                archdep_vice_exit(1);
            }
        }
        /* `-8', `-9', `-10' and `-11': Attach specified disk image.  */
        {
            int i;

            for (i = 0; i < 4; i++) {
                if (startup_disk_images[i] != NULL
                    && file_system_attach_disk(i + 8, 0, startup_disk_images[i])
                    < 0) {
                    log_error(LOG_DEFAULT,
                              "Cannot attach disk image `%s' to unit %d.",
                              startup_disk_images[i], i + 8);
                }
            }
            for (i = 4; i < 8; i++) {
                if (startup_disk_images[i] != NULL
                    && file_system_attach_disk(i + 4, 1, startup_disk_images[i])
                    < 0) {
                    log_error(LOG_DEFAULT,
                              "Cannot attach disk image `%s' to unit %d drive 1.",
                              startup_disk_images[i], i + 4);
                }
            }
        }

        /* `-1': Attach specified tape image.  */
        if (startup_tape_image[TAPEPORT_PORT_1] && tape_image_attach(TAPEPORT_PORT_1 + 1, startup_tape_image[TAPEPORT_PORT_1]) < 0) {
            log_error(LOG_DEFAULT, "Cannot attach tape image `%s'.",
                      startup_tape_image[TAPEPORT_PORT_1]);
        }

        /* `-2': Attach specified tape image.  */
        if (startup_tape_image[TAPEPORT_PORT_2] && tape_image_attach(TAPEPORT_PORT_2 + 1, startup_tape_image[TAPEPORT_PORT_2]) < 0) {
            log_error(LOG_DEFAULT, "Cannot attach tape image `%s'.",
                      startup_tape_image[TAPEPORT_PORT_2]);
        }
    }

    cmdline_free_autostart_string();
}
