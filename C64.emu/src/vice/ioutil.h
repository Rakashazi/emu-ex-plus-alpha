/*
 * ioutil.h - Miscellaneous IO utility functions.
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

#ifndef VICE_IOUTIL_H
#define VICE_IOUTIL_H

#define IOUTIL_ACCESS_R_OK 4
#define IOUTIL_ACCESS_W_OK 2
#define IOUTIL_ACCESS_X_OK 1
#define IOUTIL_ACCESS_F_OK 0

#define IOUTIL_ERRNO_EPERM  0
#define IOUTIL_ERRNO_EEXIST 1
#define IOUTIL_ERRNO_EACCES 2
#define IOUTIL_ERRNO_ENOENT 3
#define IOUTIL_ERRNO_ERANGE 4

extern int ioutil_access(const char *pathname, int mode);
extern int ioutil_chdir(const char *path);
extern int ioutil_errno(unsigned int check);
extern char *ioutil_getcwd(char *buf, int size);
extern int ioutil_isatty(int desc);
extern unsigned int ioutil_maxpathlen(void);
extern int ioutil_mkdir(const char *pathname, int mode);
extern int ioutil_remove(const char *name);
extern int ioutil_rename(const char *oldpath, const char *newpath);
extern int ioutil_stat(const char *file_name, unsigned int *len, unsigned int *isdir);

extern char *ioutil_current_dir(void);

struct ioutil_name_table_s {
    char *name;
};
typedef struct ioutil_name_table_s ioutil_name_table_t;

struct ioutil_dir_s {
    ioutil_name_table_t *dirs;
    ioutil_name_table_t *files;
    int dir_amount;
    int file_amount;
    int counter;
};
typedef struct ioutil_dir_s ioutil_dir_t;

extern ioutil_dir_t *ioutil_opendir(const char *path);
extern char *ioutil_readdir(ioutil_dir_t *ioutil_dir);
extern void ioutil_closedir(ioutil_dir_t *ioutil_dir);

#endif
