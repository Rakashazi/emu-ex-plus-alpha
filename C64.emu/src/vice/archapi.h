/** \file   archapi.h
 * \brief   Common system-specific API.
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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
 *
 */

/* Do not include this header file, include `archdep.h' instead.  */
#ifndef VICE_ARCHAPI_PRIVATE_API
#error "Do not include this header file, include `archdep.h' instead."
#endif

#ifndef VICE_ARCHAPI
#define VICE_ARCHAPI

#include "vice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

/* Program start.  */
extern int archdep_init(int *argc, char **argv);
extern void archdep_startup_log_error(const char *format, ...);

/* 
 * refactored into src/arch/shared/
 */

/* Filesystem related functions.  */
void        archdep_program_path_set_argv0(char *argv0);
const char *archdep_program_path(void);
void        archdep_program_path_free(void);

const char *archdep_program_name(void);
void        archdep_program_name_free(void);

const char *archdep_boot_path(void);
void        archdep_boot_path_free(void);

const char *archdep_home_path(void);
void        archdep_home_path_free(void);

const char *archdep_pref_path(void);

int         archdep_mkdir(const char *pathname, int mode);
int         archdep_rmdir(const char *pathname);

char *      archdep_join_paths(const char *path, ...);
int         archdep_path_is_relative(const char *path);
int         archdep_expand_path(char **return_path, const char *filename);
void        archdep_sanitize_filename(char *name);
char *      archdep_make_backup_filename(const char *fname);
int         archdep_stat(const char *file_name,
                         unsigned int *len,
                         unsigned int *isdir);
int         archdep_rename(const char *oldpath, const char *newpath);

char *      archdep_default_sysfile_pathlist(const char *emu_id);
void        archdep_default_sysfile_pathlist_free(void);

char *      archdep_extra_title_text(void);
void        archdep_extra_title_text_free(void);

int         archdep_vice_atexit(void (*function)(void));
void        archdep_vice_exit(int excode);

/* Get the absolute path to the directory that contains the documentation */
char *      archdep_get_vice_docsdir(void);
/* Get the absolute path to the directory that contains resources, icons, etc */
char *      archdep_get_vice_datadir(void);

char *      archdep_user_config_path(void);
void        archdep_user_config_path_free(void);

char *      archdep_default_hotkey_file_name(void);
char *      archdep_default_joymap_file_name(void);

/* Register CBM font with the OS without installing */
int         archdep_register_cbmfont(void);
/* Unregister CBM font */
void        archdep_unregister_cbmfont(void);

/* set permissions of given file to rw, respecting current umask */
int         archdep_fix_permissions(const char *file_name);

/* Resource handling.  */
char *      archdep_default_resource_file_name(void);

/* Fliplist.  */
char *      archdep_default_fliplist_file_name(void);

/* RTC. */
char *      archdep_default_rtc_file_name(void);
int         archdep_rtc_get_centisecond(void);

/* Autostart-PRG */
char *      archdep_default_autostart_disk_image_file_name(void);

/* Logfile stuff.  */
FILE *      archdep_open_default_log_file(void);

/* Allocates a filename for a tempfile.  */
char *      archdep_tmpnam(void);

/* Virtual keyboard handling */
int         archdep_require_vkbd(void);

/* returns a NULL terminated list of strings. Both the list and the strings
 * must be freed by the caller using lib_free(void*) */
char **     archdep_list_drives(void);

/* returns a string that corresponds to the current drive. The string must
 * be freed by the caller using lib_free(void*) */
char *      archdep_get_current_drive(void);

/* sets the current drive to the given string */
void        archdep_set_current_drive(const char *drive);

int         archdep_default_logger(const char *level_string, const char *txt);

/* Launch program `name' (searched via the PATH environment variable)
   passing `argv' as the parameters, wait for it to exit and return its
   exit status. If `pstdout_redir' or `stderr_redir' are != NULL,
   redirect stdout or stderr to the corresponding file.  */
int         archdep_spawn(const char *name, char **argv,
                            char **pstdout_redir, const char *stderr_redir);

/* Spawn need quoting the params or expanding the filename in some archs.  */
char *      archdep_filename_parameter(const char *name);
char *      archdep_quote_parameter(const char *name);

/* Allocates a filename and creates a tempfile.  */
FILE *      archdep_mkstemp_fd(char **filename, const char *mode);

/* Check file name for block or char device.  */
int         archdep_file_is_blockdev(const char *name);
int         archdep_file_is_chardev(const char *name);

void        archdep_usleep(uint64_t waitTime);

/*
 * Not yet moved to arch/shared/
 */

/* Networking. (in src/socketdrv) */
extern int archdep_network_init(void);
extern void archdep_network_shutdown(void);

/* Free everything on exit. (in arch/../archdep.c)  */
extern void archdep_shutdown(void);

#endif
