/*
 * zfile.h - Transparent handling of compressed files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_ZFILE_H
#define VICE_ZFILE_H

#include <stdio.h>

/* actions to be done when a zfile is closed */
typedef enum {
    ZFILE_KEEP,         /* Nothing, keep original file (default).  */
    ZFILE_REQUEST,      /* Request the user what to do.  */
    ZFILE_DEL           /* Remove original file.  */
} zfile_action_t;

extern FILE *zfile_fopen(const char *name, const char *mode);
extern int zfile_fclose(FILE *stream);

extern void zfile_shutdown(void);

extern int zfile_close_action(const char *filename, zfile_action_t action,
                              const char *request_string);

#if 0

/*
 it seems, that a direct zlib-aaproach runs into trouble,
 because mainly gzseek doesn't work exactly as it
 'standard' counterpart.
 Any ideas or suggestions?
 */

#include <zlib.h>

extern int gerror(FILE *f);

#define fopen                      gzopen
#define fclose                     gzclose
#define fread(buf, sz1, sz2, fd)   gzread(fd, buf, (sz1) * (sz2))
#define fwrite(buf, sz1, sz2, fd)  gzwrite(fd, buf, (sz1) * (sz2))
#define fprintf                    gzprintf
#define fputs(str, fd)             gzputs(fd, str)
#define fgets(str, len, fd)        gzgets(fd, str, len)
#define fputc(c, fd)               gzputc(fd, c)
#define fgetc                      gzgetc
#define fflush                     gzflush
#define fseek                      gzseek
#define rewind(file)               ((int)gzseek(file, 0L, SEEK_SET))
#define ftell                      gztell
#define feof                       gzeof
#define ferror                     gerror

#endif


#endif
