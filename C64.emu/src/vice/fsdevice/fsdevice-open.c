/** \file   fsdevice-open.c
 * \brief   File system device
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Teemu Rantanen <tvr@cs.hut.fi>
 * \author  Jarkko Sonninen <sonninen@lut.fi>
 * \author  Jouko Valta <jopi@stekt.oulu.fi>
 * \author  Olaf Seibert <rhialto@mbfys.kun.nl>
 * \author  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  pottendo <pottendo@gmx.net>
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

/* #define DEBUG_DRIVEOPEN */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cbmdos.h"
#include "charset.h"
#include "fileio.h"
#include "fsdevice-filename.h"
#include "fsdevice-read.h"
#include "fsdevice-resources.h"
#include "fsdevice-write.h"
#include "fsdevicetypes.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "tape.h"
#include "vdrive-command.h"
#include "vdrive.h"
#include "util.h"

#include "fsdevice-open.h"

#ifdef DEBUG_DRIVEOPEN
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* shorten the path shown in the disk header to 16 characters */
static uint8_t *makeshortheader(uint8_t *p)
{
    int longnames = 0;
    size_t n;
    uint8_t *d;

    if (resources_get_int("FSDeviceLongNames", &longnames) < 0) {
        return p;
    }
    n = strlen((char*)p);
    if (!longnames && (n > 16)) {
        d = p + (n - 1);
        /* scan backwards until path seperator */
        while (d != p) {
            if (*d == '/') { /* FIXME: use macro */
                d++; n = 0;
                /* copy last part to the beginning */
                while (d) {
                    p[n] = *d;
                    n++;
                    if (n == 16) {
                        break;
                    }
                    d++;
                }
                p[n] = 0;
                return p;
            }
            d--;
        }
    }
    return p;
}

static int fsdevice_open_directory(vdrive_t *vdrive, unsigned int secondary,
                                   bufinfo_t *bufinfo,
                                   cbmdos_cmd_parse_t *cmd_parse, char *rname)
{
    archdep_dir_t *host_dir;
    char *mask;
    uint8_t *p;
    int i;

    if ((secondary != 0) || (bufinfo[secondary].mode != Read)) {
        fsdevice_error(vdrive, CBMDOS_IPE_NOT_WRITE);
        return FLOPPY_ERROR;
    }

    if (!(mask = strrchr(rname, '/'))) {
        mask = rname;
    }

    /* Test on wildcards.  */
    if (cbmdos_parse_wildcard_check(mask, (unsigned int)strlen(mask))) {
        if (*mask == '/') {
            strcpy(bufinfo[secondary].dirmask, mask + 1);
            *mask++ = 0;
        } else {
            strcpy(bufinfo[secondary].dirmask, mask);
            lib_free(cmd_parse->parsecmd);
            cmd_parse->parsecmd = lib_strdup(fsdevice_get_path(vdrive->unit));
        }
    } else {
        bufinfo[secondary].dirmask[0] = '\0';
        if (!*(cmd_parse->parsecmd)) {
            lib_free(cmd_parse->parsecmd);
            cmd_parse->parsecmd = lib_strdup(fsdevice_get_path(vdrive->unit));
        }
    }

    /* trying to open */
    host_dir = archdep_opendir((char *)(cmd_parse->parsecmd), ARCHDEP_OPENDIR_ALL_FILES);
    if (host_dir == NULL) {
        for (p = (uint8_t *)(cmd_parse->parsecmd); *p; p++) {
            if (isupper((unsigned char)*p)) {
                *p = tolower((unsigned char)*p);
            }
        }
        host_dir = archdep_opendir((char *)(cmd_parse->parsecmd), ARCHDEP_OPENDIR_ALL_FILES);
        if (host_dir == NULL) {
            fsdevice_error(vdrive, CBMDOS_IPE_NOT_FOUND);
            return FLOPPY_ERROR;
        }
    }
    strcpy(bufinfo[secondary].dir, cmd_parse->parsecmd);
    /*
     * Start Address, Line Link and Line number 0
     */

    p = bufinfo[secondary].name;

    *p++ = 1;
    *p++ = 4;

    *p++ = 1;
    *p++ = 1;

    *p++ = 0;
    *p++ = 0;

    *p++ = (uint8_t)0x12;     /* Reverse on */

    *p++ = '"';
    strcpy((char *)p, bufinfo[secondary].dir); /* Dir name */
    charset_petconvstring((uint8_t *)p, CONVERT_TO_PETSCII);   /* ASCII name to PETSCII */
    i = 0;

    makeshortheader(p);

    while (*p) {
        ++p;
        i++;
    }
    while (i < 16) {
        *p++ = ' ';
        i++;
    }

    /* put the drive-unit and drive number into the "format id" */
    *p++ = '"';
    *p++ = ' ';
    if (vdrive->unit < 10) {
        *p++ = ' ';
        *p++ = '#';
        *p++ = '0' + vdrive->unit;
    } else {
        *p++ = '#';
        *p++ = '1';
        *p++ = '0' + (vdrive->unit - 10);
    }
    *p++ = ':';
    *p++ = '0';
    *p++ = 0;

    bufinfo[secondary].buflen = (int)(p - bufinfo[secondary].name);
    bufinfo[secondary].bufp = bufinfo[secondary].name;
    bufinfo[secondary].mode = Directory;
    bufinfo[secondary].host_dir = host_dir;
    bufinfo[secondary].eof = 0;

    return FLOPPY_COMMAND_OK;
}

static int fsdevice_open_file(vdrive_t *vdrive, unsigned int secondary,
                              bufinfo_t *bufinfo,
                              cbmdos_cmd_parse_t *cmd_parse, char *rname)
{
    char *comma;
    char *newrname;
    tape_image_t *tape;
    unsigned int format = 0;
    fileio_info_t *finfo;
    int fileio_command;

    if (fsdevice_convert_p00_enabled[(vdrive->unit) - 8]) {
        format |= FILEIO_FORMAT_P00;
    }
    if (!fsdevice_hide_cbm_files_enabled[vdrive->unit - 8]) {
        format |= FILEIO_FORMAT_RAW;
    }

    /* Remove comma.  */
    if ((cmd_parse->parsecmd)[0] == ',') {
        (cmd_parse->parsecmd)[1] = '\0';
    } else {
        comma = strchr(cmd_parse->parsecmd, ',');
        if (comma != NULL) {
            *comma = '\0';
        }
    }

    /* Test on wildcards.  */
    if (cbmdos_parse_wildcard_check(cmd_parse->parsecmd,
                                    (unsigned int)strlen(cmd_parse->parsecmd)) > 0) {
        if (bufinfo[secondary].mode == Write
            || bufinfo[secondary].mode == Append) {
            fsdevice_error(vdrive, CBMDOS_IPE_BAD_NAME);
            return FLOPPY_ERROR;
        }
    }

    /* Open file for write mode access.  */
    if (bufinfo[secondary].mode == Write) {

        if (fsdevice_save_p00_enabled[vdrive->unit - 8]) {
            format = FILEIO_FORMAT_P00;
        } else {
            format = FILEIO_FORMAT_RAW;
        }

        DBG(("fsdevice_open_file write '%s'\n", rname));
        fsdevice_limit_createnamelength(vdrive, rname);
        DBG(("fsdevice_open_file write limited: '%s'\n", rname));

        if (cmd_parse->atsign) {
            /* TODO: maybe rename to a backup name */
            DBG(("fsdevice_open_file overwrite @'%s'\n", rname));
            fileio_command = FILEIO_COMMAND_OVERWRITE;
        } else if (fsdevice_overwrite_existing_files) {
            fileio_command = FILEIO_COMMAND_OVERWRITE;
        } else {
            fileio_command = FILEIO_COMMAND_WRITE;
        }

        finfo = fileio_open(rname, fsdevice_get_path(vdrive->unit), format,
                            fileio_command, bufinfo[secondary].type,
                            &bufinfo[secondary].reclen);

        if (finfo != NULL) {
            bufinfo[secondary].fileio_info = finfo;
            fsdevice_error(vdrive, CBMDOS_IPE_OK);
            return FLOPPY_COMMAND_OK;
        } else {
            fsdevice_error(vdrive, CBMDOS_IPE_FILE_EXISTS);
            return FLOPPY_ERROR;
        }
    }

    if (bufinfo[secondary].mode == Append) {
        /* Open file for append mode access.  */
        DBG(("fsdevice_open_file append '%s'\n", rname));
        newrname = fsdevice_expand_shortname(vdrive, rname);
        DBG(("fsdevice_open_file append expanded '%s'\n", newrname));
        finfo = fileio_open(newrname, fsdevice_get_path(vdrive->unit), format,
                            FILEIO_COMMAND_APPEND_READ,
                            bufinfo[secondary].type,
                            &bufinfo[secondary].reclen);
        lib_free(newrname);

        if (finfo != NULL) {
            bufinfo[secondary].fileio_info = finfo;
            fsdevice_error(vdrive, CBMDOS_IPE_OK);
            return FLOPPY_COMMAND_OK;
        } else {
            fsdevice_error(vdrive, CBMDOS_IPE_NOT_FOUND);
            return FLOPPY_ERROR;
        }
    }

    /* Open file for read or relative mode access.  */
    tape = bufinfo[secondary].tape;
    tape->name = util_concat(fsdevice_get_path(vdrive->unit),
                             ARCHDEP_DIR_SEP_STR, rname, NULL);
    charset_petconvstring((uint8_t *)(tape->name) +
                          strlen(fsdevice_get_path(vdrive->unit)) +
                          strlen(ARCHDEP_DIR_SEP_STR), CONVERT_TO_ASCII);
    tape->read_only = 1;
    /* Prepare for buffered reads */
    bufinfo[secondary].isbuffered = 0;
    bufinfo[secondary].iseof = 0;
    if (tape_image_open(tape) < 0) {
        lib_free(tape->name);
        tape->name = NULL;
    } else {
        tape_file_record_t *r;
        static uint8_t startaddr[2];
        tape_seek_start(tape);
        tape_seek_to_file(tape, 0);
        r = tape_get_current_file_record(tape);
        if ((r->type == 1) || (r->type == 3)) {
            startaddr[0] = r->start_addr & 255;
            startaddr[1] = r->start_addr >> 8;
            bufinfo[secondary].bufp = startaddr;
            bufinfo[secondary].buflen = 2;
        } else {
            bufinfo[secondary].buflen = 0;
        }

        return FLOPPY_COMMAND_OK;
    }


    DBG(("fsdevice_open_file read '%s'\n", rname));
    newrname = fsdevice_expand_shortname(vdrive, rname);
    DBG(("fsdevice_open_file read expanded '%s'\n", newrname));

    fileio_command =
        bufinfo[secondary].mode == Relative ? FILEIO_COMMAND_READ_WRITE
                                            : FILEIO_COMMAND_READ;

    finfo = fileio_open(newrname, fsdevice_get_path(vdrive->unit), format,
                        fileio_command, bufinfo[secondary].type,
                        &bufinfo[secondary].reclen);

    lib_free(newrname);

    if (finfo != NULL) {
        bufinfo[secondary].fileio_info = finfo;
        fsdevice_error(vdrive, CBMDOS_IPE_OK);

        if (bufinfo[secondary].mode == Relative) {
            fsdevice_relative_switch_record(vdrive, &bufinfo[secondary], 0, 0);
        }

        return FLOPPY_COMMAND_OK;
    }

    fsdevice_error(vdrive, CBMDOS_IPE_NOT_FOUND);
    return FLOPPY_ERROR;
}

static int fsdevice_open_buffer(vdrive_t *vdrive, unsigned int secondary,
                                bufinfo_t *bufinfo,
                                cbmdos_cmd_parse_t *cmd_parse, char *rname)
{
    log_message(LOG_DEFAULT, "Fsdevice: Warning - open channel '%s'. (block access needs disk image)", rname);
    fsdevice_error(vdrive, CBMDOS_IPE_OK);
    return FLOPPY_COMMAND_OK;
}

int fsdevice_open(vdrive_t *vdrive, const uint8_t *name, unsigned int length,
                  unsigned int secondary, cbmdos_cmd_parse_t *cmd_parse_ext)
{
    char rname[ARCHDEP_PATH_MAX];
    int status = 0, rc;
    unsigned int i;
    cbmdos_cmd_parse_t cmd_parse;
    bufinfo_t *bufinfo;

    DBG(("fsdevice_open name:'%s' (secondary:%u)\n", name, secondary));

    bufinfo = fsdevice_dev[vdrive->unit - 8].bufinfo;

    if (bufinfo[secondary].fileio_info != NULL) {
        return FLOPPY_ERROR;
    }

    if (secondary == 15) {
        for (i = 0; i < length; i++) {
            status = fsdevice_write(vdrive, name[i], 15);
        }
        return status;
    }
    cmd_parse.cmd = name;
    cmd_parse.cmdlength = length;
    cmd_parse.secondary = secondary;

    DBG(("fsdevice_open cmd_parse '%s'\n", name));

    rc = cbmdos_command_parse(&cmd_parse);
    if (rc != SERIAL_OK) {
        status = SERIAL_ERROR;
        goto out;
    }

    /*
       Check for '@0:filename' or '@:filename'. Must include a ':'.
       (original filename starts with '@', but parsed version doesn't)
       '@filename' will open a file starting with '@'!
     */
    if (length > 0 && name[0] == '@' &&
          !(cmd_parse.parselength > 0 && cmd_parse.parsecmd[0] == '@')) {
        cmd_parse.atsign = 1;
    }

    bufinfo[secondary].type = cmd_parse.filetype;
    bufinfo[secondary].reclen = cmd_parse.recordlength;
    bufinfo[secondary].num_records = -1;

    cmd_parse.parsecmd[cmd_parse.parselength] = 0;
    strncpy(rname, cmd_parse.parsecmd, cmd_parse.parselength + 1);

    /* CBM name to FSname */
    charset_petconvstring((uint8_t *)(cmd_parse.parsecmd), CONVERT_TO_ASCII);
    DBG(("fsdevice_open rname: %s\n", rname));

    if (cmd_parse.filetype == CBMDOS_FT_REL) {
        /* REL files override whatever rwmode has been inferred by
         * parsecmd() */
        bufinfo[secondary].mode = Relative;
    } else {
        switch (cmd_parse.readmode) {
            case CBMDOS_FAM_WRITE:
                bufinfo[secondary].mode = Write;
                break;
            case CBMDOS_FAM_READ:
                bufinfo[secondary].mode = Read;
                break;
            case CBMDOS_FAM_APPEND:
                bufinfo[secondary].mode = Append;
                break;
        }
    }

    /*
        check wether the length of the name string does not match the
        passed length. this might happen if a) the cbm filename in the
        running program is zero terminated and b) a wrong namelen is
        passed to SETNAM. this would almost certainly result in a "file
        not found" - so it is the best we can do in that case, too.
    */
    if (strlen((const char*)name) != length) {
        log_message(LOG_DEFAULT,
                "Fsdevice: Warning - filename '%s' with bogus length '%u'.",
                cmd_parse.parsecmd, length);
        status = CBMDOS_IPE_NOT_FOUND;
        goto out;
    }

    if (*name == '$') {
        status = fsdevice_open_directory(vdrive, secondary, bufinfo, &cmd_parse, rname);
    } else if (*name == '#') {
        status = fsdevice_open_buffer(vdrive, secondary, bufinfo, &cmd_parse, rname);
    } else {
        status = fsdevice_open_file(vdrive, secondary, bufinfo, &cmd_parse, rname);
    }

    if (status != FLOPPY_COMMAND_OK) {
        goto out;
    }

    fsdevice_error(vdrive, CBMDOS_IPE_OK);

out:
    lib_free(cmd_parse.parsecmd);

    return status;
}
