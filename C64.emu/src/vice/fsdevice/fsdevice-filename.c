/*
 * fsdevice-filename.c - File system device.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

/* #define DEBUGFILENAME */

#include "vice.h"

#include <string.h>

#include "charset.h"
#include "fsdevice-filename.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "vdrive.h"

#ifdef DEBUGFILENAME
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* A lot of programs will not work right with filenames that are longer than 16 
   characters, so we must shorten them somehow. unfortunately this is less than 
   trivial :/ 

   - when creating a new file, we can just pad the name to 16 characters. this
     is pretty much what a real CBM drive would do and should cause no side
     effects.

   - when listing the directory, we create a short filename using a simple
     algorithm:

     - count the number of files that are the same for the first 14 chars
     - replace the last two chars by a) a marker that represents the counter for
       the current file and b) a character that is valid in a petscii filename,
       but invalid on the host filesystem.

    for example:

        1234567890123456789012              1234567890123456
        testfoobartestest.prg   becomes     testfoobartest0/
        testfoobartestAB.prg    becomes     testfoobartest1/

   - when opening an existing file, we iterate through the current work 
     directory, convert each filename to a short name using the algorithm above,
     and then compare if the result matches the filename we want to open. if so,
     we can use the long name of the file to open it.

    all functions below should be completely transparent (ie not change the
    provided names in any way) when "FSDeviceLongNames" is set to "1".

*/

/* character to be used as a marker for long names. this must be a valid character
   in a petscii filename, but an invalid character in the host filesystem. in
   practise that means we have to use the forward slash, as this is the only
   invalid character in filenames on linux. */
#define LONGNAMEMARKER '/'

/*
    convert real (long) name into shortened representation

    mode    0 - name is ASCII
            1 - name is PETSCII
*/

#define MAXDIRPOSMARK (10+26+26)

static int _limit_longname(struct ioutil_dir_s *ioutil_dir, vdrive_t *vdrive, char *longname, int mode)
{
    char *direntry;
    char *newname;
    int longnames;
    int dirpos = 0;
    int tmppos;
    char *dirposmark[2] = { 
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};

    DBG(("limit_longname enter '%s' mode: %d\n", longname, mode));
    if (resources_get_int("FSDeviceLongNames", &longnames) < 0) {    
        return -1;
    }

    /* get a buffer for the new name */
    newname = lib_malloc(ioutil_maxpathlen());

    if (!longnames) {
        if (strlen(longname) > 16) {
            tmppos = ioutil_getdirpos(ioutil_dir);
            ioutil_resetdir(ioutil_dir);

            while(1) {
                direntry = ioutil_readdir(ioutil_dir);
                if (direntry == NULL) {
                    break;
                }
                strcpy(newname, direntry);
                if (mode) {
                    charset_petconvstring((uint8_t *)newname, CONVERT_TO_PETSCII);   /* ASCII name to PETSCII */
                }
                if (!strncmp(newname, longname, 14)) {
                    dirpos++;
                    /* handle max count */
                    if (dirpos == MAXDIRPOSMARK) {
                        log_error(LOG_DEFAULT, "could not make a unique short name for '%s'", longname);
                        ioutil_setdirpos(ioutil_dir, tmppos);
                        return -1;
                    }
                    DBG(("limit_longname found partial '%s'\n", longname));
                }
                DBG(("limit_longname>%d '%s'->'%s' (%s)\n", dirpos, direntry, newname, longname));
                if (!strcmp(newname, longname)) {
                    DBG(("limit_longname found full '%s'\n", longname));
                    longname[14] = dirposmark[mode][dirpos];
                    longname[15] = LONGNAMEMARKER;
                    longname[16] = 0;
                    break;
                }
            }
            ioutil_setdirpos(ioutil_dir, tmppos);
        }
    }
    DBG(("limit_longname return '%s'\n", longname));
    lib_free(newname);

    return 0;
}

static int limit_longname(vdrive_t *vdrive, char *longname, int mode)
{
    struct ioutil_dir_s *ioutil_dir;
    char *prefix;
    int ret = -1;

    prefix = fsdevice_get_path(vdrive->unit);
    DBG(("limit_longname path '%s'\n", prefix));

    ioutil_dir = ioutil_opendir(prefix, IOUTIL_OPENDIR_ALL_FILES);
    ret = _limit_longname(ioutil_dir, vdrive, longname, mode);
    ioutil_closedir(ioutil_dir);

    return ret;
}

/*
    convert shortened name into the actual (long) name

    mode    0 - name is ASCII
            1 - name is PETSCII
*/

static char *expand_shortname(vdrive_t *vdrive, char *shortname, int mode)
{
    struct ioutil_dir_s *ioutil_dir;
    char *direntry;
    char *prefix;
    char *longname;
    int longnames;

    if (resources_get_int("FSDeviceLongNames", &longnames) < 0) {    
        longnames = 0;
    }

    DBG(("expand_shortname shortname '%s' mode: %d\n", shortname, mode));

    /* get a buffer for the new name */
    longname = lib_malloc(ioutil_maxpathlen());

    if (!longnames) {
        prefix = fsdevice_get_path(vdrive->unit);
        DBG(("expand_shortname path '%s'\n", prefix));

        ioutil_dir = ioutil_opendir(prefix, IOUTIL_OPENDIR_ALL_FILES);

        while(1) {
            direntry = ioutil_readdir(ioutil_dir);
            if (direntry == NULL) {
                break;
            }
            /* create the short name for this entry and see if it matches */
            strcpy(longname, direntry);
            _limit_longname(ioutil_dir, vdrive, longname, 0);
            if (mode) {
                charset_petconvstring((uint8_t *)longname, CONVERT_TO_PETSCII);   /* ASCII name to PETSCII */
            }
            DBG(("expand_shortname>'%s'->'%s'('%s')\n", direntry, longname, shortname));
            if (!strcmp(longname, shortname)) {
                strcpy(longname, direntry);
                if (mode) {
                    charset_petconvstring((uint8_t *)longname, CONVERT_TO_PETSCII);   /* ASCII name to PETSCII */
                }
                ioutil_closedir(ioutil_dir);
                return longname;
            }
        }
        ioutil_closedir(ioutil_dir);
    }
    /* copy original string to the new name */
    strcpy(longname, shortname);
    DBG(("expand_shortname return '%s'\n", longname));
    return longname;
}


/* takes a short name and returns a pointer to a long name

    shortname: pointer to PETSCII string (filename)
*/
char *fsdevice_expand_shortname(vdrive_t *vdrive, char *name)
{
#ifdef DEBUGFILENAME
    char *ptr = expand_shortname(vdrive, name, 1);
    DBG(("fsdevice_expand_shortname '%s' (%02x) -> '%s'\n", name, (unsigned char)name[14], ptr));
    return ptr;
#else
    return expand_shortname(vdrive, name, 1);
#endif
}

/* takes a short name and returns a pointer to a long name

    shortname: pointer to ASCII string (filename)
*/
char *fsdevice_expand_shortname_ascii(vdrive_t *vdrive, char *name)
{
#ifdef DEBUGFILENAME
    char *ptr = expand_shortname(vdrive, name, 1);
    DBG(("fsdevice_expand_shortname_ascii '%s' (%02x) -> '%s'\n", name, (unsigned char)name[14], ptr));
    return ptr;
#else
    return expand_shortname(vdrive, name, 0);
#endif
}


/* limit a filename length to 16 characters. works in-place, ie it changes
   the input string 

   used when listing the directory, in this case we must create a unique short
   name that can be expanded to the full long name later.

   name: pointer to PETSCII string (filename)
*/
int fsdevice_limit_namelength(vdrive_t *vdrive, uint8_t *name)
{
    return limit_longname(vdrive, (char*)name, 1);
}

/* limit a filename length to 16 characters. works in-place, ie it changes
   the input string 

   used when listing the directory, in this case we must create a unique short
   name that can be expanded to the full long name later.

   name: pointer to ASCII string (filename)
*/
int fsdevice_limit_namelength_ascii(vdrive_t *vdrive, char *name)
{
    return limit_longname(vdrive, name, 0);
}

/* limit a filename length to 16 characters. works in-place, ie it changes
   the input string 

    used when creating a file. in this case we can simply cut off the long
    name after 16 chars - just like a real CBM drive would do.
*/
int fsdevice_limit_createnamelength(vdrive_t *vdrive, char *name)
{
    int longnames;

    if (resources_get_int("FSDeviceLongNames", &longnames) < 0) {    
        return -1;
    }

    if (!longnames) {
        if (strlen((char*)name) > 16) {
            name[16] = 0;
        }
    }
    return 0;
}
