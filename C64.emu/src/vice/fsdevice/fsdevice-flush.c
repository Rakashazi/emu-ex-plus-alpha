/*
 * fsdevice-flush.c - File system device.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cbmdos.h"
#include "charset.h"
#include "fileio.h"
#include "fsdevice-flush.h"
#include "fsdevice-resources.h"
#include "fsdevice.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vdrive-command.h"
#include "vdrive.h"

static int fsdevice_flush_reset(void)
{
    return CBMDOS_IPE_DOS_VERSION;
}

static int fsdevice_flush_cd(vdrive_t* vdrive, char *arg)
{
    int er;

    /* arrow left also works for dir up */
    if (!strcmp("_", arg)) {
        arg = "..";
    }

    er = CBMDOS_IPE_OK;
    if (ioutil_chdir(fsdevice_get_path(vdrive->unit)) || ioutil_chdir(arg)) {
        er = CBMDOS_IPE_NOT_FOUND;
        if (ioutil_errno(IOUTIL_ERRNO_EPERM)) {
            er = CBMDOS_IPE_PERMISSION;
        }
    } else { /* get full path and save */
        arg = ioutil_current_dir();
        fsdevice_set_directory(arg, vdrive->unit);
        lib_free(arg);
    }

    return er;
}

static int fsdevice_flush_cdup(vdrive_t* vdrive)
{
    return fsdevice_flush_cd(vdrive, "..");
}

static int fsdevice_flush_mkdir(char *arg)
{
    int er;

    er = CBMDOS_IPE_OK;
    if (ioutil_mkdir(arg, IOUTIL_MKDIR_RWXUG)) {
        er = CBMDOS_IPE_INVAL;
        if (ioutil_errno(IOUTIL_ERRNO_EEXIST)) {
            er = CBMDOS_IPE_FILE_EXISTS;
        }
        if (ioutil_errno(IOUTIL_ERRNO_EACCES)) {
            er = CBMDOS_IPE_PERMISSION;
        }
        if (ioutil_errno(IOUTIL_ERRNO_ENOENT)) {
            er = CBMDOS_IPE_NOT_FOUND;
        }
    }

    return er;
}

static int fsdevice_flush_partition(vdrive_t* vdrive, char* arg)
{
    char* comma;
    int er;

    if (arg == NULL || *arg == '\0') {
        er = CBMDOS_IPE_SYNTAX; /* change to root partition not implemented */
    } else if ((comma = strchr(arg, ',')) == NULL) {
        er = fsdevice_flush_cd(vdrive, arg);
    } else { /* create partition: check syntax */
        int i = 0;
        *comma++ = '\0';
        for (i = 0; i < 4 && *comma++; i++) {
        }
        if (i == 4 && *comma++ == ',' && *comma++ == 'c' && !*comma) {
            er = fsdevice_flush_mkdir(arg);
        } else {
            er = CBMDOS_IPE_SYNTAX;
        }
    }
    return er;
}

static int fsdevice_flush_remove(char *arg)
{
    int er;

    er = CBMDOS_IPE_OK;
    if (ioutil_remove(arg)) {
        er = CBMDOS_IPE_NOT_EMPTY;
        if (ioutil_errno(IOUTIL_ERRNO_EPERM)) {
            er = CBMDOS_IPE_PERMISSION;
        }
    }

    return er;
}

static int fsdevice_flush_rename(vdrive_t *vdrive, char *realarg)
{
    char *src, *dest, *tmp;
    unsigned int format = 0, rc;

    tmp = strchr(realarg, '=');

    if (tmp == NULL) {
        return CBMDOS_IPE_SYNTAX;
    }
    if (tmp == realarg) {
        return CBMDOS_IPE_SYNTAX;
    }
    if (tmp[1] == '\0') {
        return CBMDOS_IPE_SYNTAX;
    }

    tmp[0] = '\0';

    src = &tmp[1];
    dest = realarg;

    if (fsdevice_convert_p00_enabled[(vdrive->unit) - 8]) {
        format |= FILEIO_FORMAT_P00;
    }
    if (!fsdevice_hide_cbm_files_enabled[vdrive->unit - 8]) {
        format |= FILEIO_FORMAT_RAW;
    }

    rc = fileio_rename(src, dest, fsdevice_get_path(vdrive->unit), format);

    switch (rc) {
        case FILEIO_FILE_NOT_FOUND:
            return CBMDOS_IPE_NOT_FOUND;
        case FILEIO_FILE_EXISTS:
            return CBMDOS_IPE_FILE_EXISTS;
        case FILEIO_FILE_PERMISSION:
            return CBMDOS_IPE_PERMISSION;
    }

    return CBMDOS_IPE_OK;
}

static int fsdevice_flush_scratch(vdrive_t *vdrive, char *realarg)
{
    unsigned int format = 0, rc;

    if (realarg[0] == '\0') {
        return CBMDOS_IPE_SYNTAX;
    }

    if (fsdevice_convert_p00_enabled[(vdrive->unit) - 8]) {
        format |= FILEIO_FORMAT_P00;
    }
    if (!fsdevice_hide_cbm_files_enabled[vdrive->unit - 8]) {
        format |= FILEIO_FORMAT_RAW;
    }

    rc = fileio_scratch(realarg, fsdevice_get_path(vdrive->unit), format);

    switch (rc) {
        case FILEIO_FILE_NOT_FOUND:
            return CBMDOS_IPE_NOT_FOUND;
        case FILEIO_FILE_PERMISSION:
            return CBMDOS_IPE_PERMISSION;
        case FILEIO_FILE_SCRATCHED:
            return CBMDOS_IPE_DELETED;
    }

    return CBMDOS_IPE_OK;
}

/*
    fake drive memory access
*/

/* M-R - Memory Read */
static int fsdevice_flush_mr(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int length;
    WORD addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 6 + ((realarg != NULL) ? strlen(realarg) : 0); /* FIXME */
    return vdrive_command_memory_read(vdrive, &fsdevice_dev[dnr].cmdbuf[5], addr, length);
}

/* M-W - Memory Write */
static int fsdevice_flush_mw(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int length;
    WORD addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 6 + ((realarg != NULL) ? strlen(realarg) : 0); /* FIXME */
    return vdrive_command_memory_write(vdrive, &fsdevice_dev[dnr].cmdbuf[5], addr, length);
}

/* M-E - Memory Execute */
static int fsdevice_flush_me(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int length;
    WORD addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 5 + ((realarg != NULL) ? strlen(realarg) : 0); /* FIXME */
    return vdrive_command_memory_exec(vdrive, &fsdevice_dev[dnr].cmdbuf[5], addr, length);
}

/*
    fake block access
*/

static void get4args(char *realarg, unsigned int *a1, unsigned int *a2, unsigned int *a3, unsigned int *a4)
{
    char *cmd;

    if (a1) {
        *a1 = 0;
    }
    if (a2) {
        *a2 = 0;
    }
    if (a3) {
        *a3 = 0;
    }
    if (a4) {
        *a4 = 0;
    }

    if (realarg == NULL) {
        return;
    }

    cmd = realarg;
    while (*cmd) {
        if (*cmd == ',') {
            *cmd = ' ';
        }
        cmd++;
    }
    cmd = realarg;
    while ((*cmd == ' ') && (cmd != 0)) {
        cmd++;
    }
    if (a1) {
        *a1 = atoi(cmd);
    }

    while ((*cmd != ' ') && (cmd != 0)) {
        cmd++;
    }
    while ((*cmd == ' ') && (cmd != 0)) {
        cmd++;
    }
    if (a2) {
        *a2 = atoi(cmd);
    }

    while ((*cmd != ' ') && (cmd != 0)) {
        cmd++;
    }
    while ((*cmd == ' ') && (cmd != 0)) {
        cmd++;
    }
    if (a3) {
        *a3 = atoi(cmd);
    }

    while ((*cmd != ' ') && (cmd != 0)) {
        cmd++;
    }
    while ((*cmd == ' ') && (cmd != 0)) {
        cmd++;
    }
    if (a4) {
        *a4 = atoi(cmd);
    }
}

static unsigned int get_bamptr(unsigned int trk, unsigned int sec)
{
    return (((trk - 1) * FSDEVICE_SECTOR_MAX) + sec) >> 3;
}

static unsigned int get_bammask(unsigned int trk, unsigned int sec)
{
    return (((trk - 1) * FSDEVICE_SECTOR_MAX) + sec) & 7;
}

/* B-A - Block Allocate */
static int fsdevice_flush_ba(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int drv, trk, sec;
    unsigned int bamptr, bammask;
    int err = CBMDOS_IPE_OK;

    get4args(realarg, &drv, &trk, &sec, NULL);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-A: %d %d %d (block access needs disk image)", drv, trk, sec);

    bamptr = get_bamptr(trk, sec);
    bammask = get_bammask(trk, sec);

    if ((fsdevice_dev[dnr].bam[bamptr] & bammask) == bammask) {
        err = CBMDOS_IPE_NO_BLOCK;

        while ((fsdevice_dev[dnr].bam[bamptr] & bammask) == bammask) {
            sec++;
            if (sec >= FSDEVICE_SECTOR_MAX) {
                sec = 0;
                trk++;
                if (trk > FSDEVICE_TRACK_MAX) {
                    trk = 0;
                    sec = 0;
                    goto exitba;
                }
            }
            bamptr = get_bamptr(trk, sec);
            bammask = get_bammask(trk, sec);
        }
    } else {
        fsdevice_dev[dnr].bam[bamptr] |= bammask;
    }

exitba:
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;
    return err;
}

/* B-F - Block Free */
static int fsdevice_flush_bf(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int drv, trk, sec;
    unsigned int bamptr, bammask;

    get4args(realarg, &drv, &trk, &sec, NULL);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-F: %d %d %d (block access needs disk image)", drv, trk, sec);

    bamptr = get_bamptr(trk, sec);
    bammask = get_bammask(trk, sec);
    fsdevice_dev[dnr].bam[bamptr] &= ~bammask;

    return CBMDOS_IPE_OK;
}

/* B-P - Block Pointer */
static int fsdevice_flush_bp(vdrive_t *vdrive, char *realarg)
{
    unsigned int chn, pos;
    get4args(realarg, &chn, &pos, NULL, NULL);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-P: %d %d (block access needs disk image)", chn, pos);
    return CBMDOS_IPE_OK;
}

/* B-E - Block Execute */
static int fsdevice_flush_be(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int chn, drv, trk, sec;
    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-E: %d %d %d %d (needs TDE)", chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;
    return CBMDOS_IPE_OK;
}

/* B-W - Block Read */
static int fsdevice_flush_br(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int chn, drv, trk, sec;
    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-R: %d %d %d %d (block access needs disk image)", chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;
    return CBMDOS_IPE_OK;
}

/* U1, like B-R */
static int fsdevice_flush_u1(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - 8;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - U1: %d %d %d %d (block access needs disk image)", chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* B-W - Block Write */
static int fsdevice_flush_bw(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - 8;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - B-W: %d %d %d %d (block access needs disk image)", chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* U2, like B-W */
static int fsdevice_flush_u2(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - 8;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT, "Fsdevice: Warning - U2: %d %d %d %d (block access needs disk image)", chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* I - Initialize Disk */
static int fsdevice_flush_initialize(vdrive_t *vdrive)
{
    int dnr = vdrive->unit - 8;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

/* V - Validate Disk */
static int fsdevice_flush_validate(vdrive_t *vdrive)
{
    int dnr = vdrive->unit - 8;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

/* N - Format Disk */
static int fsdevice_flush_new(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - 8;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

void fsdevice_flush(vdrive_t *vdrive, unsigned int secondary)
{
    unsigned int dnr;
    char *cmd, *realarg, *arg;
    char *cbmcmd;
    int er = CBMDOS_IPE_SYNTAX;

    dnr = vdrive->unit - 8;

    if ((secondary != 15) || (!(fsdevice_dev[dnr].cptr))) {
        return;
    }

    cbmcmd = lib_malloc(ioutil_maxpathlen());

    /* FIXME: Use `vdrive_command_parse()'! */
    /* remove trailing cr */
    while (fsdevice_dev[dnr].cptr
           && (fsdevice_dev[dnr].cmdbuf[fsdevice_dev[dnr].cptr - 1] == 13)) {
        (fsdevice_dev[dnr].cptr)--;
    }

    fsdevice_dev[dnr].cmdbuf[fsdevice_dev[dnr].cptr] = 0;

    strcpy(cbmcmd, (char *)(fsdevice_dev[dnr].cmdbuf));
    charset_petconvstring((BYTE *)cbmcmd, 1);   /* CBM name to FSname */
    cmd = cbmcmd;

    while (*cmd == ' ') {
        cmd++;
    }

    arg = strchr(cbmcmd, ':');

    if (arg != NULL) {
        *arg++ = '\0';
    }

    realarg = strchr((char *)(fsdevice_dev[dnr].cmdbuf), ':');

    if (realarg != NULL) {
        *realarg++ = '\0';
    }

    /*
       i
       v
       n
       r
       s
       c

       b-r chn drv trk sec
       u1  chn drv trk sec
       b-w chn drv trk sec
       u2  chn drv trk sec
       b-p chn pos
       b-a drv trk sec
       b-f drv trk sec
       b-e chn drv trk sec

       m-r lo hi len
       m-w lo hi len <data>
       m-e lo hi

       u9/ui switch mode
       u:/uj reset

       cd
       cd_
       cd:_
       /
       md
       rd
    */
    if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-R", 3)) {
        er = fsdevice_flush_mr(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-W", 3)) {
        er = fsdevice_flush_mw(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-E", 3)) {
        er = fsdevice_flush_me(vdrive, realarg);
    } else if (!strcmp(cmd, "u1")) {
        er = fsdevice_flush_u1(vdrive, realarg);
    } else if (!strcmp(cmd, "u2")) {
        er = fsdevice_flush_u2(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-A", 3)) {
        er = fsdevice_flush_ba(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-F", 3)) {
        er = fsdevice_flush_bf(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-R", 3)) {
        er = fsdevice_flush_br(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-W", 3)) {
        er = fsdevice_flush_bw(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-P", 3)) {
        er = fsdevice_flush_bp(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "B-E", 3)) {
        er = fsdevice_flush_be(vdrive, realarg);
    } else if (!strcmp(cmd, "cd")) {
        er = fsdevice_flush_cd(vdrive, arg);
    } else if (!strcmp((char *)(fsdevice_dev[dnr].cmdbuf), "CD_")) {
        er = fsdevice_flush_cdup(vdrive);
    } else if (!strcmp((char *)(fsdevice_dev[dnr].cmdbuf), "CD:_")) {
        er = fsdevice_flush_cdup(vdrive);
    } else if (*cmd == '/') {
        er = fsdevice_flush_partition(vdrive, arg);
    } else if (!strcmp(cmd, "md")) {
        er = fsdevice_flush_mkdir(arg);
    } else if (!strcmp(cmd, "rd")) {
        er = fsdevice_flush_remove(arg);
    } else if ((!strcmp(cmd, "ui")) || (!strcmp(cmd, "u9"))) {
        er = fsdevice_flush_reset();
    } else if ((!strcmp(cmd, "uj")) || (!strcmp(cmd, "u:"))) {
        er = fsdevice_flush_reset();
    } else if (*cmd == 'i') { /* additional args for I are ignored */
        er = fsdevice_flush_initialize(vdrive);
    } else if (*cmd == 'v') { /* additional args for V are ignored */
        er = fsdevice_flush_validate(vdrive);
    } else if (*cmd == 'n' && arg != NULL) {
        er = fsdevice_flush_new(vdrive, realarg);
    } else if (*cmd == 'r' && arg != NULL) {
        er = fsdevice_flush_rename(vdrive, realarg);
    } else if (*cmd == 's' && arg != NULL) {
        er = fsdevice_flush_scratch(vdrive, realarg);
    }

    fsdevice_error(vdrive, er);

    fsdevice_dev[dnr].cptr = 0;

    lib_free(cbmcmd);
}

int fsdevice_flush_write_byte(vdrive_t *vdrive, BYTE data)
{
    unsigned int dnr;
    int rc;

    dnr = vdrive->unit - 8;
    rc = SERIAL_OK;

    /* FIXME: Consider the real size of the input buffer. */
    if (fsdevice_dev[dnr].cptr < ioutil_maxpathlen() - 1) {
        fsdevice_dev[dnr].cmdbuf[(fsdevice_dev[dnr].cptr)++] = data;
        rc = SERIAL_OK;
    } else {
        fsdevice_error(vdrive, CBMDOS_IPE_LONG_LINE);
        rc = SERIAL_ERROR;
    }

    return rc;
}
