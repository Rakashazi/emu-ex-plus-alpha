/*
 * ioutil.c - Miscellaneous IO utility functions.
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

#include <stdio.h>

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#if defined(HAVE_DIRENT_H) || defined(AMIGA_AROS)
#include <dirent.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifdef __OS2__
#include "snippets/dirport.h"
#endif

#ifdef __NeXT__
#include <sys/dir.h>
#define dirent direct
#endif

#include "archdep.h"
#include "ioutil.h"
#include "lib.h"
#include "types.h"
#include "util.h"
#include "vicemaxpath.h"

/* Mostly POSIX compatibily */

int ioutil_access(const char *pathname, int mode)
{
    int access_mode = 0;

    if ((mode & IOUTIL_ACCESS_R_OK) == IOUTIL_ACCESS_R_OK) {
        access_mode |= ARCHDEP_R_OK;
    }
    if ((mode & IOUTIL_ACCESS_W_OK) == IOUTIL_ACCESS_W_OK) {
        access_mode |= ARCHDEP_W_OK;
    }
    if ((mode & IOUTIL_ACCESS_X_OK) == IOUTIL_ACCESS_X_OK) {
        access_mode |= ARCHDEP_X_OK;
    }
    if ((mode & IOUTIL_ACCESS_F_OK) == IOUTIL_ACCESS_F_OK) {
        access_mode |= ARCHDEP_F_OK;
    }

    return access(pathname, access_mode);
}

int ioutil_chdir(const char *path)
{
    return chdir((char*)path);
}

int ioutil_errno(unsigned int check)
{
    switch (check) {
#ifndef __OS2__
        case IOUTIL_ERRNO_EPERM:
            if (errno == EPERM) {
                return 1;
            }
            break;
#endif
        case IOUTIL_ERRNO_EEXIST:
            if (errno == EEXIST) {
                return 1;
            }
            break;
        case IOUTIL_ERRNO_EACCES:
            if (errno == EACCES) {
                return 1;
            }
            break;
        case IOUTIL_ERRNO_ENOENT:
            if (errno == ENOENT) {
                return 1;
            }
            break;
#ifndef __OS2__
        case IOUTIL_ERRNO_ERANGE:
            if (errno == ERANGE) {
                return 1;
            }
            break;
#endif
        default:
            return 0;
    }

    return 0;
}

#if !defined(VMS) && !defined(__VAX)
#ifndef HAVE_GETCWD
char *getcwd (char *buf, size_t len)
{
    char ourbuf[PATH_MAX];
    char *result;

    result = getwd (ourbuf);
    if (result) {
        if (strlen (ourbuf) >= len) {
            errno = ERANGE;
            return 0;
        }
        strcpy (buf, ourbuf);
    }
    return buf;
}
#endif
#endif

char *ioutil_getcwd(char *buf, int size)
{
    return getcwd(buf, (size_t)size);
}

int ioutil_isatty(int desc)
{
    return isatty(desc);
}

unsigned int ioutil_maxpathlen(void)
{
    return PATH_MAX;
}

int ioutil_mkdir(const char *pathname, int mode)
{
    return archdep_mkdir(pathname, mode);
}

int ioutil_remove(const char *name)
{
    return unlink(name);
}

int ioutil_rename(const char *oldpath, const char *newpath)
{
    return archdep_rename(oldpath, newpath);
}

int ioutil_stat(const char *file_name, unsigned int *len, unsigned int *isdir)
{
    return archdep_stat(file_name, len, isdir);
}

/* ------------------------------------------------------------------------- */
/* IO helper functions.  */
char *ioutil_current_dir(void)
{
    static size_t len = 128;
    char *p = lib_malloc(len);

    while (getcwd(p, len) == NULL) {
        if (errno == ERANGE) {
            len *= 2;
            p = lib_realloc(p, len);
        } else {
            return NULL;
        }
    }

    return p;
}

#ifndef DINGOO_NATIVE
static int dirs_amount = 0;
static int files_amount = 0;

static int ioutil_compare_names(const void* a, const void* b)
{
    ioutil_name_table_t *arg1 = (ioutil_name_table_t*)a;
    ioutil_name_table_t *arg2 = (ioutil_name_table_t*)b;
    return strcmp(arg1->name, arg2->name);
}

/*
    NOTE: even when _DIRENT_HAVE_D_TYPE is defined, d_type may still be returned
          as DT_UNKNOWN - in that case we must fall back to using stat instead.
 */
static int ioutil_count_dir_items(const char *path)
{
    DIR *dirp;
    struct dirent *dp;
/* #ifndef _DIRENT_HAVE_D_TYPE */
    unsigned int len, isdir;
    char *filename;
    int retval;
/* #endif */

    dirs_amount = 0;
    files_amount = 0;

    dirp = opendir(path);

    if (dirp == NULL) {
        return -1;
    }

    dp = readdir(dirp);

    while (dp != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
        if (dp->d_type != DT_UNKNOWN) {
            if (dp->d_type == DT_DIR) {
                dirs_amount++;
#ifdef DT_LNK
            } else if (dp->d_type == DT_LNK) {
                filename = util_concat(path, FSDEV_DIR_SEP_STR, dp->d_name, NULL);
                retval = ioutil_stat(filename, &len, &isdir);
                if (retval == 0) {
                    if (isdir) {
                        dirs_amount++;
                    } else {
                        files_amount++;
                    }
                }
                if (filename) {
                    lib_free(filename);
                    filename = NULL;
                }
#endif /* DT_LNK */
            } else {
                files_amount++;
            }
            dp = readdir(dirp);
        } else {
#endif
            filename = util_concat(path, FSDEV_DIR_SEP_STR, dp->d_name, NULL);
            retval = ioutil_stat(filename, &len, &isdir);
            if (retval == 0) {
                if (isdir) {
                    dirs_amount++;
                } else {
                    files_amount++;
                }
            }
            dp = readdir(dirp);
            lib_free(filename);
#ifdef _DIRENT_HAVE_D_TYPE
        }
#endif
    }
    closedir(dirp);
    return 0;
}

static void ioutil_filldir(const char *path, ioutil_name_table_t *dirs, ioutil_name_table_t *files)
{
    DIR *dirp = NULL;
    struct dirent *dp = NULL;
    int dir_count = 0;
    int file_count = 0;
/* #ifndef _DIRENT_HAVE_D_TYPE */
    unsigned int len, isdir;
    char *filename;
    int retval;
/* #endif */

    dirp = opendir(path);

    dp = readdir(dirp);

    while (dp != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
        if (dp->d_type != DT_UNKNOWN) {
            if (dp->d_type == DT_DIR) {
                dirs[dir_count].name = lib_stralloc(dp->d_name);
                dir_count++;
#ifdef DT_LNK
            } else if (dp->d_type == DT_LNK) {
                filename = util_concat(path, FSDEV_DIR_SEP_STR, dp->d_name, NULL);
                retval = ioutil_stat(filename, &len, &isdir);
                if (retval == 0) {
                    if (isdir) {
                        dirs[dir_count].name = lib_stralloc(dp->d_name);
                        dir_count++;
                    } else {
                        files[file_count].name = lib_stralloc(dp->d_name);
                        file_count++;
                    }
                }
                if (filename) {
                    lib_free(filename);
                    filename = NULL;
                }
#endif // DT_LNK
            } else {
                files[file_count].name = lib_stralloc(dp->d_name);
                file_count++;
            }
            dp = readdir(dirp);
        } else {
#endif
            filename = util_concat(path, FSDEV_DIR_SEP_STR, dp->d_name, NULL);
            retval = ioutil_stat(filename, &len, &isdir);
            if (retval == 0) {
                if (isdir) {
                    dirs[dir_count].name = lib_stralloc(dp->d_name);
                    dir_count++;
                } else {
                    files[file_count].name = lib_stralloc(dp->d_name);
                    file_count++;
                }
            }
            dp = readdir(dirp);
            lib_free(filename);
#ifdef _DIRENT_HAVE_D_TYPE
        }
#endif
    }
    closedir(dirp);
}

ioutil_dir_t *ioutil_opendir(const char *path)
{
    int retval;
    ioutil_dir_t *ioutil_dir;

    retval = ioutil_count_dir_items(path);
    if (retval < 0) {
        return NULL;
    }

    ioutil_dir = lib_malloc(sizeof(ioutil_dir_t));

    ioutil_dir->dirs = lib_malloc(sizeof(ioutil_name_table_t) * dirs_amount);
    ioutil_dir->files = lib_malloc(sizeof(ioutil_name_table_t) * files_amount);

    ioutil_filldir(path, ioutil_dir->dirs, ioutil_dir->files);
    qsort(ioutil_dir->dirs, dirs_amount, sizeof(ioutil_name_table_t), ioutil_compare_names);
    qsort(ioutil_dir->files, files_amount, sizeof(ioutil_name_table_t), ioutil_compare_names);

    ioutil_dir->dir_amount = dirs_amount;
    ioutil_dir->file_amount = files_amount;
    ioutil_dir->counter = 0;

    return ioutil_dir;
}
#endif

char *ioutil_readdir(ioutil_dir_t *ioutil_dir)
{
    char *retval;

    if (ioutil_dir->counter >= (ioutil_dir->dir_amount + ioutil_dir->file_amount)) {
        return NULL;
    }

    if (ioutil_dir->counter >= ioutil_dir->dir_amount) {
        retval = ioutil_dir->files[ioutil_dir->counter - ioutil_dir->dir_amount].name;
    } else {
        retval = ioutil_dir->dirs[ioutil_dir->counter].name;
    }
    ioutil_dir->counter++;

    return retval;
}

void ioutil_closedir(ioutil_dir_t *ioutil_dir)
{
    int i;

    for (i = 0; i < ioutil_dir->dir_amount; i++) {
        lib_free(ioutil_dir->dirs[i].name);
    }
    for (i = 0; i < ioutil_dir->file_amount; i++) {
        lib_free(ioutil_dir->files[i].name);
    }
    lib_free(ioutil_dir->dirs);
    lib_free(ioutil_dir->files);
    lib_free(ioutil_dir);
}
