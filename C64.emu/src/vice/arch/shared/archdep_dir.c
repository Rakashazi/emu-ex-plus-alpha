/** \file   archdep_dir.c
 * \brief   Read host directory
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"
#include "archdep.h"

#include <string.h>
#include <stdbool.h>

#if defined(WINDOWS_COMPILE)
# include <windows.h>
#elif defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
# include <errno.h>
# include <sys/types.h>
# include <dirent.h>
#else
# error "Unsupported OS!"
#endif

#include "lib.h"
#include "util.h"

#include "archdep_dir.h"


/** \brief  Initial size of an array of file/directory entries
 *
 * Number of elements in the arrays when initially allocating them.
 *
 * This number was chosen arbitrarily, reasonable trade-off between wasting
 * memory for small directories and not triggering too many realloc calls with
 * larger directories.
 */
#define INITIAL_ARRAY_SIZE  64


/** \brief  Comparision function for qsort() in archdep_opendir()
 *
 * \param[in]   a   first item
 * \param[in]   b   second item
 *
 * \return  \< 0 if \a a \< \a b, 0 if \a a == \a b, \> 0 if \a a \> \a b
 */
static int compare_names(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}


#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
/** \brief  Check if an entry must be shown
 *
 * Check if \a name matches the \a mode filter.
 *
 * Checks if a dotfile must be shown according to \a mode.
 *
 * \return  true if the file or directory must be shown
 */
static bool must_show_dotfile(const char *name, int mode)
{
    if (mode & ARCHDEP_OPENDIR_NO_HIDDEN_FILES) {
        /* checks for a *nix "dotfile" */
        if ((name[0] == '.') &&
            (name[1] != 0) &&
            (name[1] != '.')) {
            return false;
        }
    }
    return true;
}
#endif


/** \brief  Add dir entry to directory object
 *
 * Add \a path to the list of directory in \a dir.
 *
 * \param[in]   dir     directory object
 * \param[in]   path    dirname to add
 */
static void add_dir_entry(archdep_dir_t *dir, const char *path)
{
    if (dir->dir_amount == dir->dirs_size) {
        /* double array size */
        dir->dirs_size *= 2;
        dir->dirs = lib_realloc(dir->dirs, dir->dirs_size * sizeof *(dir->dirs));
    }
    dir->dirs[dir->dir_amount++] = lib_strdup(path);
}


/** \brief  Add file entry to directory object
 *
 * Add \a path to the list of files in \a dir.
 *
 * \param[in]   dir     directory object
 * \param[in]   path    filename to add
 */
static void add_file_entry(archdep_dir_t *dir, const char *path)
{
    if (dir->file_amount == dir->files_size) {
        /* double array size */
        dir->files_size *= 2;
        dir->files = lib_realloc(dir->files, dir->files_size * sizeof *(dir->files));
    }
    dir->files[dir->file_amount++] = lib_strdup(path);
}


/** \brief  Populate the directory and file lists
 *
 * Scan \a path for files and directories, filtering them according to \a mode.
 *
 * \param[in]   dir     directory object
 * \param[in]   path    directory to scan
 * \param[in]   mode    filter to apply to each directory entry
 *
 * \return  true on success
 */
static bool scan_directory(archdep_dir_t *dir, const char *path, int mode)
{
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    DIR *dirp = NULL;
    struct dirent *dp = NULL;
    char *filename;
    size_t len;
    unsigned int isdir;

    dirp = opendir(path);
    if (dirp == NULL) {
        return false;
    }

    dp = readdir(dirp);

    while (dp != NULL) {
        if (must_show_dotfile(dp->d_name, mode)) {
#ifdef _DIRENT_HAVE_D_TYPE
            if (dp->d_type != DT_UNKNOWN) {
                if (dp->d_type == DT_DIR) {
                    add_dir_entry(dir, dp->d_name);
#ifdef DT_LNK
                } else if (dp->d_type == DT_LNK) {
                    filename = util_concat(path, ARCHDEP_DIR_SEP_STR, dp->d_name, NULL);
                    if (archdep_stat(filename, &len, &isdir) == 0) {
                        if (isdir) {
                            add_dir_entry(dir, dp->d_name);
                        } else {
                            add_file_entry(dir, dp->d_name);
                        }
                    }
                    if (filename) {
                        lib_free(filename);
                        filename = NULL;
                    }
#endif /* DT_LNK */
                } else {
                    add_file_entry(dir, dp->d_name);
                }
                dp = readdir(dirp);
            } else {
#endif /* _DIRENT_HAVE_D_TYPE */
                filename = util_concat(path, ARCHDEP_DIR_SEP_STR, dp->d_name, NULL);
                if (archdep_stat(filename, &len, &isdir) == 0) {
                    if (isdir) {
                        add_dir_entry(dir, dp->d_name);
                    } else {
                        add_file_entry(dir, dp->d_name);
                    }
                }
                dp = readdir(dirp);
                lib_free(filename);
#ifdef _DIRENT_HAVE_D_TYPE
            }
#endif
        } else {
            dp = readdir(dirp);
        }
    }
    closedir(dirp);

#elif defined(WINDOWS_COMPILE)
    HANDLE ffhandle;
    WIN32_FIND_DATA ffdata;
    char pattern[ARCHDEP_PATH_MAX];

    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    pattern[sizeof(pattern) - 1] = '\0';

    ffhandle = FindFirstFile(pattern, &ffdata);
    if (ffhandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        /* show hidden files? */
        if ((ffdata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
                (mode & ARCHDEP_OPENDIR_NO_HIDDEN_FILES)) {
            continue;   /* nope */
        }

        if (ffdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            add_dir_entry(dir, ffdata.cFileName);
        } else {
            add_file_entry(dir, ffdata.cFileName);
        }
    } while (FindNextFile(ffhandle, &ffdata));

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        FindClose(ffhandle);
        return false;
    }
    FindClose(ffhandle);
#endif
    return true;
}


/** \brief  Scan directory and return a directory object
 *
 * \param[in]   path    directory to scan
 * \param[in]   mode    filter to apply on each directory entry
 *
 * \return  directory object or NULL on failure
 */
archdep_dir_t *archdep_opendir(const char *path, int mode)
{
    archdep_dir_t *dir = lib_malloc(sizeof *dir);

    dir->pos = 0;
    dir->dir_amount = 0;
    dir->dirs_size = INITIAL_ARRAY_SIZE;
    dir->dirs = lib_malloc(sizeof *(dir->dirs) * dir->dirs_size);
    dir->file_amount = 0;
    dir->files_size = INITIAL_ARRAY_SIZE;
    dir->files = lib_malloc(sizeof *(dir->files) * dir->files_size);

    if (!scan_directory(dir, path, mode)) {
        /* clean up */
        archdep_closedir(dir);
        return NULL;
    }

    qsort(dir->dirs, dir->dir_amount, sizeof *(dir->dirs), compare_names);
    qsort(dir->files, dir->file_amount, sizeof *(dir->files), compare_names);

    return dir;
}


/** \brief  Get an entry from the directory
 *
 * \param[in]   dir     directory object
 *
 * \return  entry or NULL when the directory is exhausted
 */
const char *archdep_readdir(archdep_dir_t *dir)
{
    const char *retval = NULL;
    int dir_amount = dir->dir_amount;
    int file_amount = dir->file_amount;
    int pos = dir->pos;

    if (pos >= 0 && pos < dir_amount) {
        retval = dir->dirs[pos];
        dir->pos++;
    } else if (pos >= dir_amount && pos < (dir_amount + file_amount)) {
        retval = dir->files[pos - dir_amount];
        dir->pos++;
    }
    return retval;
}



/** \brief  Close directory
 *
 * Free memory used by \a dir and its members.
 *
 * \param[in]   dir     directory object
 */
void archdep_closedir(archdep_dir_t *dir)
{
    int i;

    for (i = 0; i < dir->dir_amount; i++) {
        lib_free(dir->dirs[i]);
    }
    for (i = 0; i < dir->file_amount; i++) {
        lib_free(dir->files[i]);
    }
    lib_free(dir->dirs);
    lib_free(dir->files);
    lib_free(dir);
}


/** \brief  Rewind position in directory
 *
 * Rewind position in \a dir to the first entry.
 *
 * \param[in]   dir     directory object
 */
void archdep_rewinddir(archdep_dir_t *dir)
{
    dir->pos = 0;
}


/** \brief  Set position in directory
 *
 * \param[in]   dir     directory object
 * \param[in]   pos     new position
 */
void archdep_seekdir(archdep_dir_t *dir, int pos)
{
    if (pos >= 0 && pos < (dir->dir_amount + dir->file_amount)) {
        dir->pos = pos;
    }
}


/** \brief  Get position in directory
 *
 * \param[in]   dir     directory object
 *
 * \return  position in \a dir or -1 when \a dir is exhausted
 */
int archdep_telldir(const archdep_dir_t *dir)
{
    if (dir->pos >= (dir->dir_amount + dir->file_amount)) {
        return -1;
    }
    return dir->pos;
}


/** \brief  Get directory entry from directory object
 *
 * \param[in]   dir     directory object
 * \param[in]   pos     position in directories list in \a dir
 *
 * \return  directory name or NULL when \a pos is out of bounds
 */
const char *archdep_readdir_get_dir(const archdep_dir_t *dir, int pos)
{
    if (pos >= 0 && pos < dir->dir_amount) {
        return dir->dirs[pos];
    }
    return NULL;
}


/** \brief  Get filename entry from directory object
 *
 * \param[in]   dir     directory object
 * \param[in]   pos     position in files list in \a dir
 *
 * \return  filename or NULL when \a pos is out of bounds
 */
const char *archdep_readdir_get_file(const archdep_dir_t *dir, int pos)
{
    if (pos >= 0 && pos < dir->file_amount) {
        return dir->files[pos];
    }
    return NULL;
}


/** \brief  Get entry from directory object
 *
 * Get entry from \a dir with the assumption that the directories come before
 * the files, joining the two lists as a single list for the \a pos argument.
 *
 * \param[in]   dir     directory object
 * \param[in]   pos     position in directories and files list in \a dir
 *
 * \return  entry or NULL when \a pos is out of bounds
 */
const char *archdep_readdir_get_entry(const archdep_dir_t *dir, int pos)
{
    if (pos < dir->dir_amount) {
        return archdep_readdir_get_dir(dir, pos);
    } else {
        return archdep_readdir_get_file(dir, pos - dir->dir_amount);
    }
}


/** \brief  Get total number of entries
 *
 * \param[in]   dir directory object
 *
 * \return  total number of entries (dirs + files)
 */
int archdep_readdir_num_entries(const archdep_dir_t *dir)
{
    return dir->dir_amount + dir->file_amount;
}


/** \brief  Get number of directory entries
 *
 * \param[in]   dir directory object
 *
 * \return  number of directory entries
 */
int archdep_readdir_num_dirs(const archdep_dir_t *dir)
{
    return dir->dir_amount;
}


/** \brief  Get number of file entries
 *
 * \param[in]   dir directory object
 *
 * \return  number of file entries
 */
int archdep_readdir_num_files(const archdep_dir_t *dir)
{
    return dir->file_amount;
}
