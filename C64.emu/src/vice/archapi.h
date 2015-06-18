/*
 * archapi.h - Common system-specific API.
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

/* Do not include this header file, include `archdep.h' instead.  */

#ifndef VICE_ARCHAPI
#define VICE_ARCHAPI

#include <stdarg.h>
#include <stdio.h>


/* Program start.  */
extern int archdep_init(int *argc, char **argv);
extern void archdep_startup_log_error(const char *format, ...);

/* Filesystem related functions.  */
extern char *archdep_program_name(void);
extern const char *archdep_boot_path(void);
extern char *archdep_default_sysfile_pathlist(const char *emu_id);
extern int archdep_path_is_relative(const char *path);
extern int archdep_expand_path(char **return_path, const char *filename);
extern char *archdep_make_backup_filename(const char *fname);
extern int archdep_mkdir(const char *pathname, int mode);
extern int archdep_stat(const char *file_name, unsigned int *len, unsigned int *isdir);
extern int archdep_rename(const char *oldpath, const char *newpath);

/* set permissions of given file to rw, respecting current umask */
extern int archdep_fix_permissions(const char *file_name);

/* Resource handling.  */
extern char *archdep_default_resource_file_name(void);
extern char *archdep_default_save_resource_file_name(void);

/* Fliplist.  */
extern char *archdep_default_fliplist_file_name(void);

/* RTC. */
extern char *archdep_default_rtc_file_name(void);

/* Autostart-PRG */
extern char *archdep_default_autostart_disk_image_file_name(void);

/* Logfile stuff.  */
extern FILE *archdep_open_default_log_file(void);
extern int archdep_default_logger(const char *level_string, const char *txt);

/* Launch program `name' (searched via the PATH environment variable)
   passing `argv' as the parameters, wait for it to exit and return its
   exit status. If `pstdout_redir' or `stderr_redir' are != NULL,
   redirect stdout or stderr to the corresponding file.  */
extern int archdep_spawn(const char *name, char **argv,
                         char **pstdout_redir, const char *stderr_redir);

/* Spawn need quoting the params or expanding the filename in some archs.  */
extern char *archdep_filename_parameter(const char *name);
extern char *archdep_quote_parameter(const char *name);

/* Allocates a filename for a tempfile.  */
extern char *archdep_tmpnam(void);

/* Allocates a filename and creates a tempfile.  */
extern FILE *archdep_mkstemp_fd(char **filename, const char *mode);

/* Check file for gzip extension.  */
extern int archdep_file_is_gzip(const char *name);
extern int archdep_file_set_gzip(const char *name);

/* Check file name for block or char device.  */
extern int archdep_file_is_blockdev(const char *name);
extern int archdep_file_is_chardev(const char *name);

/* Networking. */
extern int archdep_network_init(void);
extern void archdep_network_shutdown(void);

/* Free everything on exit.  */
extern void archdep_shutdown(void);

/* RTC. */
extern int archdep_rtc_get_centisecond(void);

/* runtime info */
extern char *archdep_get_runtime_os(void);
extern char *archdep_get_runtime_cpu(void);

#endif
