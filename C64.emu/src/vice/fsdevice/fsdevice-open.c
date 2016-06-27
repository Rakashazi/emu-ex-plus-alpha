/*
 * fsdevice-open.c - File system device.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  pottendo <pottendo@gmx.net>
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

/* #define DEBUG_DRIVE */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cbmdos.h"
#include "charset.h"
#include "fileio.h"
#include "fsdevice-open.h"
#include "fsdevice-resources.h"
#include "fsdevice-write.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "tape.h"
#include "vdrive-command.h"
#include "vdrive.h"
#include "util.h"


static int fsdevice_open_directory(vdrive_t *vdrive, unsigned int secondary,
                                   bufinfo_t *bufinfo,
                                   cbmdos_cmd_parse_t *cmd_parse, char *rname)
{
    struct ioutil_dir_s *ioutil_dir;
    char *mask;
    BYTE *p;
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
            cmd_parse->parsecmd = lib_stralloc(fsdevice_get_path(vdrive->unit));
        }
    } else {
        bufinfo[secondary].dirmask[0] = '\0';
        if (!*(cmd_parse->parsecmd)) {
            lib_free(cmd_parse->parsecmd);
            cmd_parse->parsecmd = lib_stralloc(fsdevice_get_path(vdrive->unit));
        }
    }

    /* trying to open */
    ioutil_dir = ioutil_opendir((char *)(cmd_parse->parsecmd));
    if (ioutil_dir == NULL) {
        for (p = (BYTE *)(cmd_parse->parsecmd); *p; p++) {
            if (isupper((int)*p)) {
                *p = tolower((int)*p);
            }
        }
        ioutil_dir = ioutil_opendir((char *)(cmd_parse->parsecmd));
        if (ioutil_dir == NULL) {
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

    *p++ = (BYTE)0x12;     /* Reverse on */

    *p++ = '"';
    strcpy((char *)p, bufinfo[secondary].dir); /* Dir name */
    charset_petconvstring((BYTE *)p, 0);   /* ASCII name to PETSCII */
    i = 0;
    while (*p) {
        ++p;
        i++;
    }
    while (i < 16) {
        *p++ = ' ';
        i++;
    }
    *p++ = '"';
    *p++ = ' ';
    *p++ = 'V';
    *p++ = 'I';
    *p++ = 'C';
    *p++ = 'E';
    *p++ = ' ';
    *p++ = 0;

    bufinfo[secondary].buflen = (int)(p - bufinfo[secondary].name);
    bufinfo[secondary].bufp = bufinfo[secondary].name;
    bufinfo[secondary].mode = Directory;
    bufinfo[secondary].ioutil_dir = ioutil_dir;
    bufinfo[secondary].eof = 0;

    return FLOPPY_COMMAND_OK;
}

static int fsdevice_open_file(vdrive_t *vdrive, unsigned int secondary,
                              bufinfo_t *bufinfo,
                              cbmdos_cmd_parse_t *cmd_parse, char *rname)
{
    char *comma;
    tape_image_t *tape;
    unsigned int format = 0;
    fileio_info_t *finfo;

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

        finfo = fileio_open(rname, fsdevice_get_path(vdrive->unit), format,
                            FILEIO_COMMAND_WRITE, bufinfo[secondary].type);

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
        finfo = fileio_open(rname, fsdevice_get_path(vdrive->unit), format,
                            FILEIO_COMMAND_APPEND_READ,
                            bufinfo[secondary].type);

        if (finfo != NULL) {
            bufinfo[secondary].fileio_info = finfo;
            fsdevice_error(vdrive, CBMDOS_IPE_OK);
            return FLOPPY_COMMAND_OK;
        } else {
            fsdevice_error(vdrive, CBMDOS_IPE_NOT_FOUND);
            return FLOPPY_ERROR;
        }
    }

    /* Open file for read mode access.  */
    tape = bufinfo[secondary].tape;
    tape->name = util_concat(fsdevice_get_path(vdrive->unit),
                             FSDEV_DIR_SEP_STR, rname, NULL);
    charset_petconvstring((BYTE *)(tape->name) +
                          strlen(fsdevice_get_path(vdrive->unit)) +
                          strlen(FSDEV_DIR_SEP_STR), 1);
    tape->read_only = 1;
    /* Prepare for buffered reads */
    bufinfo[secondary].isbuffered = 0;
    bufinfo[secondary].iseof = 0;
    if (tape_image_open(tape) < 0) {
        lib_free(tape->name);
        tape->name = NULL;
    } else {
        tape_file_record_t *r;
        static BYTE startaddr[2];
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

    finfo = fileio_open(rname, fsdevice_get_path(vdrive->unit), format,
                        FILEIO_COMMAND_READ, bufinfo[secondary].type);

    if (finfo != NULL) {
        bufinfo[secondary].fileio_info = finfo;
        fsdevice_error(vdrive, CBMDOS_IPE_OK);
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

int fsdevice_open(vdrive_t *vdrive, const BYTE *name, unsigned int length,
                  unsigned int secondary, cbmdos_cmd_parse_t *cmd_parse_ext)
{
    char *rname;
    int status = 0, rc;
    unsigned int i;
    cbmdos_cmd_parse_t cmd_parse;
    bufinfo_t *bufinfo;

#ifdef DEBUG_DRIVE
    log_debug("fsdevice_open name:'%s'", name);
#endif

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

    rc = cbmdos_command_parse(&cmd_parse);
    if (rc != SERIAL_OK) {
        status = SERIAL_ERROR;
        goto out;
    }

    bufinfo[secondary].type = cmd_parse.filetype;

    rname = lib_malloc(ioutil_maxpathlen());

    cmd_parse.parsecmd[cmd_parse.parselength] = 0;
    strncpy(rname, cmd_parse.parsecmd, cmd_parse.parselength + 1);

    /* CBM name to FSname */
    charset_petconvstring((BYTE *)(cmd_parse.parsecmd), 1);

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

    /*
        check wether the length of the name string does not match the
        passed length. this might happen if a) the cbm filename in the
        running program is zero terminated and b) a wrong namelen is
        passed to SETNAM. this would almost certainly result in a "file
        not found" - so it is the best we can do in that case, too.
    */
    if (strlen((const char*)name) != length) {
        log_message(LOG_DEFAULT, "Fsdevice: Warning - filename '%s' with bogus length '%d'.", cmd_parse.parsecmd, length);
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

    lib_free(rname);

    if (status != FLOPPY_COMMAND_OK) {
        goto out;
    }

    fsdevice_error(vdrive, CBMDOS_IPE_OK);

out:
    lib_free(cmd_parse.parsecmd);

    return status;
}
