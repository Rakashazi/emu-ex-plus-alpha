/** \file   uihotkeys.c
 * \brief   UI-agnostic hotkeys
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
 */

#include "vice.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "archdep_defs.h"
#include "archdep_exit.h"
#include "archdep_file_exists.h"
#include "archdep_get_vice_hotkeysdir.h"
#include "archdep_user_config_path.h"
#include "cmdline.h"
#include "hotkeystypes.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "parser.h"
#include "vhkkeysyms.h"
#include "uiapi.h"
#include "uiactions.h"
#include "util.h"
#include "version.h"
#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif

#include "uihotkeys.h"

/* to disable debugging, uncomment the #define, but keep the #include: */
/* #define DEBUG_VHK */
#include "vhkdebug.h"


/** \brief  Length of the filename string for VICE user-defined hotkeys files
 *
 * This excludes the prefix set by ui_hotkeys_init(), the machine name and the
 * terminating null
 *
 * "{prefix}-hotkeys-{machine}.vhk"
 */
#define VHK_FILENAME_LEN   (1u + 7u + 1u + 4u)


/* Forward declarations */
static void update_vhk_source_type(const char *path);


/** \brief  Log for the hotkeys system
 */
log_t vhk_log;

/** \brief  Prefix for .vhk filenames
 *
 * Set with ui_hotkeys_init(), this string is used to differenciate between
 * UIs/archs, so each can have their own hotkeys on the same system.
 * For example, for the Gtk3 UI, this is "gtk3", which results in the default
 * filename being "gtk3-hotkeys.vhk".
 *
 * \note    Freed with ui_hotkeys_shutdown() on emulator shutdown.
 */
static char *vhk_prefix = NULL;

/** \brief  Path to current hotkey file
 *
 * \note    Freed with ui_hotkeys_shutdown() on emulator shutdown.
 */
static char *vhk_filename = NULL;

/** \brief  A .vhk file is pending
 *
 * When the 'HotkeyFile' resource is initially set the UI hasn't finished
 * building the menu and registering actions, so trying to parse a hotkeys
 * file and adding hotkeys for actions will fail. This flag is used to
 * indicate a hotkey file is pending to be parsed.
 */
static bool vhk_file_pending = false;

/** \brief  The hotkeys system is initialized
 *
 * This flag is used to determine if a new HotkeyFile resource value can be
 * used to parse a hotkeys file, or if it needs to be marked as pending,
 * waiting for the UI to be fully initialized.
 */
static bool vhk_init_done = false;

#if 0
/** \brief  The command line option -default was used
 *
 * When \c -default is used we want to load the default VICE-provided hotkeys,
 * not custom hotkeys in the user config dir, nor what the resource "HotkeyFile"
 * contains.
 */
static bool vhk_default_requested = false;
#endif

/** \brief  Source of vhk file parsed
 *
 * Enum indicating the source of the last succesfully parsed hotkeys file.
 * Initially this will be \c VHK_SOURCE_NONE, indicating no hotkeys file has
 * been loaded. If a hotkeys file is succesfully parsed it will be set to
 * either \c VHK_SOURCE_VICE for hotkeys loaded from the VICE data dir,
 * \c VHK_SOURCE_USER for hotkeys loaded from the user's configuration dir, or
 * \c VHK_SOURCE_RESOURCE for hotkeys loaded from a custom file specified in
 * the "HotkeyFile" resource.
 */
static vhk_source_t vhk_source = VHK_SOURCE_NONE;


/* VICE resources, command line options and their handlers */

/** \brief  Set hotkey file and process its contents
 *
 * \param[in]   val     new hotkey file
 * \param[in]   param   extra argument (unused)
 *
 * \return  0 on success, -1 on error
 */
static int vhk_filename_set(const char *val, void *param)
{
    if (util_string_set(&vhk_filename, val) != 0) {
        /* new value was the same as the old value, don't do anything */
        return 0;
    }

    /* process hotkeys */
    if (help_requested) {
        return 0;
    }

    if (val != NULL && *val != '\0') {
        if (vhk_init_done) {
            /* UI is properly initialized, directly parse the hotkeys file */
            log_message(vhk_log, "Parsing '%s':", val);
            ui_hotkeys_load(val);
            vhk_file_pending = false;
        } else {
            /* UI is not yet fully initialized, mark parsing of hotkeys file
             * pending */
            vhk_file_pending = true;
        }
    }
    return 0;
}

/** \brief  String type resources
 */
static resource_string_t resources_string[] = {
    { "HotkeyFile", "", RES_EVENT_NO, NULL,
      &vhk_filename, vhk_filename_set, NULL },
    RESOURCE_STRING_LIST_END
};

/** \brief  List of command line options
 */
static const cmdline_option_t cmdline_options[] = {
    { "-hotkeyfile", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "HotkeyFile", NULL,
      "<name>", "Specify name of hotkey file" },
    CMDLINE_LIST_END
};


/** \brief  Initialize resources used by the custom hotkeys
 *
 * \return  0 on success
 */
int ui_hotkeys_resources_init(void)
{
    return resources_register_string(resources_string);
}


/** \brief  Initialize command line options used by the custom hotkeys
 *
 * \return  0 on success
 */
int ui_hotkeys_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}


/** \brief  Initialize hotkeys system
 *
 * Initialize the hotkeys system, setting a UI prefix for user-defined hotkeys
 * files and loading hotkeys.
 * Although all UIs share the VICE-provided hotkeys files, we need a
 * prefix per UI so saving custom hotkeys in the user's configuration directory
 * can be done from multiple UIs.
 *
 * Predence of hotkeys file to parse:
 * - When \c -default is used on the command line we load the VICE-provided file
 *   "hotkeys.vhk" in VICE_DATADIR/hotkeys.
 * - When the "HotkeyFile" resource contains a non-empty string, we load that
 *   file.
 * - When a user-defined hotkeys file for the current emulator and UI is found
 *   in the user's config dir (say "~/.config/vice/gtk3-hotkeys-PET.vhk"), we
 *   load that file.
 * - When none of the above is true we load the VICE-provided file "hotkeys.vhk"
 *   in VICE_DATADIR/hotkeys.
 *
 * \param[in]   prefix  UI name prefix for user-defined .vhk files
 */
void ui_hotkeys_init(const char *prefix)
{
    vhk_log = log_open("Hotkeys");
    log_message(vhk_log, "Initializing hotkeys.");
    if (prefix == NULL || *prefix == '\0') {
        log_error(vhk_log,
                  "%s(): `prefix` cannot be empty or NULL.",
                  __func__);
        archdep_vice_exit(1);
    }
    vhk_prefix = lib_strdup(prefix);
    vhk_source = VHK_SOURCE_NONE;

    vhk_parser_init();

    /* When we get to here the UI has been initialized */
    vhk_init_done = true;

    /* determine which file to load */
    if (default_settings_requested) {
        /* user specific -default on the command line: ignore HotkeyFile and
         * any custom hotkeys file in the user's config dir
         */
        ui_hotkeys_load_vice_default();
    } else {
        if (vhk_filename == NULL || *vhk_filename == '\0') {
            /* no -hotkeyfile, first try to load the custom hotkeys from their
             * user config dir */
            if (!ui_hotkeys_load_user_default()) {
                /* no hotkeys file present, load VICE defaults */
                ui_hotkeys_load_vice_default();
            }
        } else {
            if (vhk_file_pending) {
                /* We have a pending hotkeys file to parse */
                ui_hotkeys_load(vhk_filename);
                vhk_file_pending = false;
                vhk_source = VHK_SOURCE_RESOURCE;
            }
        }
    }
}


/** \brief  Shut down hotkeys system
 *
 * Clear up all resources used by the hotkeys system and close the log.
 * The "virtual method" ui_hotkeys_arch_shutdown() is called first to allow the
 * current UI/arch to clean up its hotkeys data, if any.
 */
void ui_hotkeys_shutdown(void)
{
    log_message(vhk_log, "shutting down.");
    /* call virtual method before tearing down the generic hotkeys data */
    ui_hotkeys_arch_shutdown();
    vhk_parser_shutdown();
    lib_free(vhk_prefix);
    lib_free(vhk_filename);
    log_close(vhk_log);
}


/** \brief  Load the default hotkeys
 *
 * Parse the VICE-provided hotkey files, clearing any user-defined hotkeys.
 * Also set the "HotkeyFile" resource to "".
 */
void ui_hotkeys_load_vice_default(void)
{
    const char *filename   = ui_hotkeys_vhk_filename_vice();
    char       *hotkeysdir = archdep_get_vice_hotkeysdir();
    char       *fullpath;

    fullpath = util_join_paths(hotkeysdir, filename, NULL);
    lib_free(hotkeysdir);
    log_message(vhk_log,
                "parsing default file '%s' for machine %s",
                fullpath, machine_name);
    lib_free(fullpath);

    if (ui_hotkeys_load(filename)) {
        log_message(vhk_log, "OK.");
        /* clear the custom hotkeys file resource */
        resources_set_string("HotkeyFile", "");
        vhk_source = VHK_SOURCE_VICE;
    } else {
        log_message(vhk_log, "failed, continuing anyway.");
    }
}


/** \brief  Try to load user-defined hotkeys from the default user file
 *
 * \return  \c false if the default user file doesn't exist, \c true otherwise
 */
bool ui_hotkeys_load_user_default(void)
{
    char *path = ui_hotkeys_vhk_full_path_user();

    if (!archdep_file_exists(path)) {
        lib_free(path);
        return false;
    }
    log_message(vhk_log,
                "parsing user-defined hotkeys in '%s'",
                path);
    if (ui_hotkeys_load(path)) {
        log_message(vhk_log, "OK.");
        vhk_source = VHK_SOURCE_USER;
    } else {
        log_message(vhk_log, "failed, continuing anyway.");
    }
    lib_free(path);
    return true;
}


/*
 * Save current hotkeys to file
 */

/** \brief  Helper: log I/O error
 *
 * Logs libc I/O errors to the hotkeys log, including errno and description.
 */
static void save_log_io_error(void)
{
    log_error(vhk_log,
              "Hotkeys: I/O error (%d: %s).",
               errno, strerror(errno));
}

/** \brief  Output the header for a custom hotkeys file
 *
 * \param[in]   fp  file descriptor
 *
 * \return  \c true on success
 */
static bool save_header(FILE *fp)
{
    const struct tm *tinfo;
    time_t           t;
    char             buffer[1024];
    int              result;

    result = fprintf(fp,
"# VICE hotkeys file for %s\n"
"#\n"
"# TODO: Add documentation of .vhk format\n"
"\n", machine_get_name());
    if (result < 0) {
        save_log_io_error();
        return false;
    }

    /* add current datetime */
    t = time(NULL);
    tinfo = localtime(&t);
    if (tinfo != NULL) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M%z", tinfo);
        result = fprintf(fp, "# Generated on %s\n", buffer);
        if (result < 0) {
            save_log_io_error();
            return false;
        }
    }

    /* add VICE version */
#ifdef USE_SVN_REVISION
    result = fprintf(fp, "# Generated by VICE %s r%s\n",
                     VERSION, VICE_SVN_REV_STRING);
#else
    result = fprintf(fp, "# Generated by VICE %s\n", VERSION);
#endif
     if (result < 0) {
        save_log_io_error();
        return false;
    }

    result = fprintf(fp,
"!debug disable\n"
"!clear\n"
"\n"
"# User-defined hotkeys:\n"
"\n");
    if (result < 0) {
        save_log_io_error();
        return false;
    }
    return true;
}


/** \brief  Save all currently registered hotkeys to file
 *
 * Generate file \a path and write all currently defined hotkeys to it. This
 * file will contain hotkeys specific to the current emulator, UI and OS,
 * meaning files generated on MacOS will lead to weird results when used on
 * non-MacOS, and vice-versa.
 *
 * \param[in]   path    filename of vhk file to generate
 *
 * \return  \c true on success
 */
bool ui_hotkeys_save_as(const char *path)
{
    FILE *fp;
    int   action;

    log_message(vhk_log, "Hotkeys: exporting current hotkeys to '%s'.", path);

    fp = fopen(path, "wb");
    if (fp == NULL) {
        log_error(vhk_log,
                  "Hotkeys: failed to open '%s' for writing (%d: %s).",
                  path, errno, strerror(errno));
        return false;
    }

    if (!save_header(fp)) {
        fclose(fp);
        return false;
    }

    /* iterate assigned hotkeys */
    for (action = 0; action < ACTION_ID_COUNT; action++) {
        ui_action_map_t *map = ui_action_map_get(action);
        if (map != NULL && map->vice_keysym != 0) {

            const char *action_name;
            const char *keysym_name;
            char       *modmask_name;
            int         result;

            action_name  = ui_action_get_name(map->action);
            keysym_name  = vhk_keysym_name(map->vice_keysym);
            modmask_name = vhk_modmask_name(map->vice_modmask);

            /* Print line with "<action>  [<modmask>]<keysym>".
             *
             * We can use the modmask name here unconditionally since it'll
             * be an empty string when no modifiers are present.
             */
            result = fprintf(fp, "%-30s  %s%s\n",
                             action_name, modmask_name, keysym_name);
            lib_free(modmask_name);

            if (result < 0) {
                save_log_io_error();
                fclose(fp);
                return false;
            }
        }
        update_vhk_source_type(path);
        if (vhk_source == VHK_SOURCE_RESOURCE) {
            util_string_set(&vhk_filename, path);
        }
    }

    fclose(fp);
    return true;
}


/** \brief  Install hotkey for a UI action and its menu item(s), if any
 *
 * Calls the arch-specific virtual method ui_hotkeys_arch_install_by_map().
 *
 * \param[in]   map     vhk map object
 */
void ui_hotkeys_install_by_map(ui_action_map_t *map)
{
    ui_hotkeys_arch_install_by_map(map);
    /* so far no additional bookkeeping required */
}


/** \brief  Update hotkey for a UI action and its menu item(s), if any
 *
 * Update hotkey for \a map, setting the new hotkey to \a vice_keysym +
 * \a vice_modmask. Calls virtual methods ui_hotkeys_arch_remove_by_map() and
 * ui_hotkeys_arch_install_by_map() and updates VICE and arch keysysm and
 * modmasks in \a map.
 *
 * \param[in]   map             UI action map object
 * \param[in]   vice_keysym     new VICE keysym
 * \param[in]   vice_modmask    new VICE modifier mask
 */
void ui_hotkeys_update_by_map(ui_action_map_t *map,
                              uint32_t         vice_keysym,
                              uint32_t         vice_modmask)
{
    if (map != NULL) {
        uint32_t arch_keysym;
        uint32_t arch_modmask;

        /* call virtual method to remove old hotkey */
        ui_hotkeys_arch_remove_by_map(map);

        /* update map */
        arch_keysym  = ui_hotkeys_arch_keysym_to_arch(vice_keysym);
        arch_modmask = ui_hotkeys_arch_modmask_to_arch(vice_modmask);
        map->vice_keysym  = vice_keysym;
        map->vice_modmask = vice_modmask;
        map->arch_keysym  = arch_keysym;
        map->arch_modmask = arch_modmask;

        /* call virtual method to install new hotkey */
        ui_hotkeys_arch_install_by_map(map);
    }
}


/** \brief  Update hotkey for a UI action and its menu item(s), if any
 *
 * Update hotkey for \a action, setting the new hotkey to \a vice_keysym +
 * \a vice_modmask. Calls virtual methods ui_hotkeys_arch_remove_by_map() and
 * ui_hotkeys_arch_install_by_map() and updates VICE and arch keysysm and
 * modmasks for \a action.
 *
 * \param[in]   action          UI action ID
 * \param[in]   vice_keysym     new VICE keysym
 * \param[in]   vice_modmask    new VICE modifier mask
 */
void ui_hotkeys_update_by_action(int action,
                                 uint32_t vice_keysym,
                                 uint32_t vice_modmask)
{
    ui_action_map_t *map = ui_action_map_get(action);
    if (map != NULL) {
        ui_hotkeys_update_by_map(map, vice_keysym, vice_modmask);
    }
}


/** \brief  Remove hotkey from UI action and its menu item(s), if any
 *
 * Clear hotkey associated with the UI action in \a map and remove any menu
 * item accelerator labels if present.
 * Calls virtual method ui_hotkeys_arch_remove_by_map().
 *
 * \param[in]   map     UI action map object
 */
void ui_hotkeys_remove_by_map(ui_action_map_t *map)
{
    if (map != NULL) {
        ui_hotkeys_arch_remove_by_map(map);
        ui_action_map_clear_hotkey(map);
    }
}


/** \brief  Remove hotkey from UI action and its menu item(s), if any
 *
 * Clear hotkey associated with UI \a action and remove any menu item accelerator
 * labels if present.
 * Indirectly calls virtual method ui_hotkeys_arch_remove_by_map().
 *
 * \param[in]   map     vhk map object
 */
void ui_hotkeys_remove_by_action(int action)
{
    ui_action_map_t *map = ui_action_map_get(action);
    if (map != NULL) {
        ui_hotkeys_remove_by_map(map);
    }
}


/** \brief  Remove all hotkeys
 *
 * Remove all hotkeys registered and clear all accelerator labels on menu items.
 * Calls virtual method ui_hotkeys_arch_remove_by_map().
 */
void ui_hotkeys_remove_all(void)
{
    int action;

    for (action = 0 ; action < ACTION_ID_COUNT; action++) {
        ui_action_map_t *map = ui_action_map_get(action);
        if (map != NULL && map->vice_keysym != 0) {
            debug_vhk("removing hotkeys for action %d (%s)",
                      action, ui_action_get_name(action));
            ui_hotkeys_arch_remove_by_map(map);
            ui_action_map_clear_hotkey(map);
        }
    }
}


/** \brief  Load hotkeys file
 *
 * \param[in]   path    path to hotkeys file
 *
 * \return  \c true on success
 */
bool ui_hotkeys_load(const char *path)
{
    bool result;

    if (path == NULL || *path == '\0') {
        log_error(vhk_log, "Could not load hotkeys: missing filename.");
        return false;
    }

    result = vhk_parser_parse(path);
    if (result) {
        /* update source type */
        update_vhk_source_type(path);
        /* set resource */
        if (vhk_source == VHK_SOURCE_RESOURCE) {
            /* not loaded from either VICE dir or user config dir, set resource */
            util_string_set(&vhk_filename, path);
        }
    }
    return result;
}


/** \brief  Generate default hotkeys file name
 *
 * Returns either "hotkeys.vhk" (non-VSID) or "hotkeys-vsid.vhk (VSID).
 *
 * \return  filename
 */
const char *ui_hotkeys_vhk_filename_vice(void)
{
    if (machine_class == VICE_MACHINE_VSID) {
        return "hotkeys-vsid.vhk";
    } else {
        return "hotkeys.vhk";
    }
}


/** \brief  Generate default filename for user-defined .vhk files
 *
 * Generate string in the form "$ARCH-hotkeys[-mac]-$MACHINE.vhk".
 *
 * \return  filename for user-defined hotkeys
 * \note    Free result with \c lib_free() after use
 */
char *ui_hotkeys_vhk_filename_user(void)
{
    const char *suffix;
    char       *name;
    size_t      plen;
    size_t      mlen;
    size_t      size;

    /* Fix machine names: x64, x64sc and vsid use "C64", xcbm5x0 and xcbm2
     * use "CBM-II", while we want to be able to differentiate between all emus.
     */
    if (machine_class == VICE_MACHINE_VSID) {
        suffix = "VSID";    /* machine_name == "C64" */
    } else if (machine_class == VICE_MACHINE_C64SC) {
        suffix = "C64SC";   /* machine_name == "C64" */
    } else if (machine_class == VICE_MACHINE_CBM5x0) {
        suffix = "CBM5x0";  /* machine_name == "CBM-II" */
    } else if (machine_class == VICE_MACHINE_CBM6x0) {
        suffix = "CBM6x0";
    } else {
        suffix = machine_name;
    }
    plen = strlen(vhk_prefix);
    mlen = strlen(suffix);
    size = plen + VHK_FILENAME_LEN + mlen + 1u;
    name = lib_malloc(size);
    snprintf(name, size, "%s-hotkeys-%s.vhk", vhk_prefix, suffix);
    return name;
}


/** \brief  Get full path to user-defined hotkeys for machine and UI
 *
 * Generate full path to user-defined hotkeys in the user config directory for
 * the current emulator and UI/arch.
 * For example: \c /home/billy-bob/.config/vice/gtk3-hotkeys-C64SC.vhk
 *
 * \return  full path to user-defined hotkeys
 * \note    Free result with \c lib_free() after use
 */
char *ui_hotkeys_vhk_full_path_user(void)
{
    char *path;
    char *name;

    name = ui_hotkeys_vhk_filename_user();
    path = util_join_paths(archdep_user_config_path(), name, NULL);
    lib_free(name);
    return path;
}


/** \brief  Get full path to VICE-provided hotkeys for machine and UI
 *
 * \return  full path to hotkeys file in VICE data dir
 * \note    Free result with \c lib_free() after use
 */
char *ui_hotkeys_vhk_full_path_vice(void)
{
    char *fullpath;
    char *hotkeysdir;

    hotkeysdir = archdep_get_vice_hotkeysdir();
    fullpath   = util_join_paths(hotkeysdir, ui_hotkeys_vhk_filename_vice(), NULL);
    lib_free(hotkeysdir);
    return fullpath;
}


/** \brief  Update vhk source type
 *
 * Check \a path against the default hotkeys file path in VICE's data directory
 * and against the UI and emu-specific hotkeys file path in the user's
 * configuration directory and set \c vhk_source accordingly.
 *
 * \param[in]   path    path to hotkeys file
 */
static void update_vhk_source_type(const char *path)
{
    char *user_path;
    char *vice_path;

    user_path = ui_hotkeys_vhk_full_path_user();
    vice_path = ui_hotkeys_vhk_full_path_vice();

    if (strcmp(user_path, path) == 0) {
        vhk_source = VHK_SOURCE_USER;
    } else if (strcmp(vice_path, path) == 0) {
        vhk_source = VHK_SOURCE_VICE;
    } else {
       vhk_source = VHK_SOURCE_RESOURCE;
    }

    lib_free(user_path);
    lib_free(vice_path);
}


/** \brief  Get source type of last succesfully parsed hotkeys file
 *
 * \return  enum indicating source
 *
 * \see Enum vhk_source_t in \c shared/hotkeys/hotkeystypes.h
 */
vhk_source_t ui_hotkeys_vhk_source_type(void)
{
    return vhk_source;
}


/** \brief  Get full path to current hotkeys source
 *
 * Determine which hotkeys file was last succesfully parsed.
 *
 * \return  path to hotkeys file
 * \note    free with \c lib_free() after use
 */
char *ui_hotkeys_vhk_source_path(void)
{
    char *path = NULL;

    switch (vhk_source) {
        case VHK_SOURCE_VICE:
            path = ui_hotkeys_vhk_full_path_vice();
            break;
        case VHK_SOURCE_USER:
            path = ui_hotkeys_vhk_full_path_user();
            break;
        case VHK_SOURCE_RESOURCE:
            path = lib_strdup(vhk_filename);
            break;
        default:
            path = NULL;
    }
    return path;
}

#if 0
/** \brief  Set flag indicating whether the -default command line was used
 *
 * If the default setting were requested with the \c -default command line option
 * we don't want to load the custom hotkeys in the user config directory, if
 * present, nor the hotkeys file in the "HotkeyFile" resource,  but load the
 * VICE-provided hotkeys instead.
 *
 * \param[in]   requested   the default settings were requested
 */
void ui_hotkeys_set_default_requested(bool requested)
{
    vhk_default_requested = requested;
}
#endif

/** \brief  Reload hotkeys
 *
 * (Re)load hotkeys, using the same order as when starting the emulator, except
 * we're skipping the \c -default check.
 *
 * \see ui_hotkeys_init() for an explanation of logic used to determine which
 *      file to load.
 */
void ui_hotkeys_reload(void)
{
    if (vhk_filename != NULL && *vhk_filename != '\0') {
        ui_hotkeys_load(vhk_filename);
    } else if (!ui_hotkeys_load_user_default()) {
        ui_hotkeys_load_vice_default();
    }
}


/** \brief  Save current hotkeys
 *
 * Save current hotkeys to either the filename in the resource "HotkeyFile"
 * or to the UI+emu-specific filename in the user's configuration directory.
 *
 * \return  \c true on success
 */
bool ui_hotkeys_save(void)
{
    bool result;

    if (vhk_filename != NULL && *vhk_filename != '\0') {
        /* save to file in "HotkeyFile" resource */
        result = ui_hotkeys_save_as(vhk_filename);
    } else {
        /* save to user config dir */
        char *filename = ui_hotkeys_vhk_full_path_user();

        result = ui_hotkeys_save_as(filename);
        lib_free(filename);
    }
    return result;
}
