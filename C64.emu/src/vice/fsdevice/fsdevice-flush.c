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

/* #define DEBUGFLUSH */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "archdep.h"
#include "cbmdos.h"
#include "charset.h"
#include "fileio.h"
#include "fsdevice-flush.h"
#include "fsdevice-filename.h"
#include "fsdevice-read.h"
#include "fsdevice-resources.h"
#include "fsdevice.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "vdrive-command.h"
#include "vdrive.h"

#ifdef DEBUGFLUSH
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define DRIVE_UNIT_MIN          8

static int fsdevice_flush_reset(void)
{
    return CBMDOS_IPE_DOS_VERSION;
}

static int fsdevice_flush_cd(vdrive_t* vdrive, char *arg)
{
    int er;

    DBG(("fsdevice_flush_cd '%s'\n", arg));
    
    /* guard against NULL */
    if (arg == NULL) {
        return CBMDOS_IPE_SYNTAX;
    }

    /* arrow left also works for dir up */
    if (strcmp("_", arg) == 0) {
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


/** \brief  Create directory \a arg
 *
 * \param[in]   vdrive  vdrive reference
 * \param[in]   arg     directory name
 *
 * \return  CBMDOS error code
 */
static int fsdevice_flush_mkdir(vdrive_t *vdrive, char *arg)
{
    int er;
    char *prefix;
    char *path;

    DBG(("fsdevice_flush_mkdir '%s'\n", arg));
    
    /* get proper FS device path */
    prefix = fsdevice_get_path(vdrive->unit);

    /* construct absolute path */
    path = util_concat(prefix, FSDEV_DIR_SEP_STR, arg, NULL);

    er = CBMDOS_IPE_OK;
    if (ioutil_mkdir(path, IOUTIL_MKDIR_RWXUG)) {
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

    lib_free(path);

    return er;
}

static int fsdevice_flush_partition(vdrive_t *vdrive, char* arg)
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
            er = fsdevice_flush_mkdir(vdrive, arg);
        } else {
            er = CBMDOS_IPE_SYNTAX;
        }
    }
    return er;
}


/** \brief  Remove directory \a arg
 *
 * \param[in]   vdrive  vdrive reference
 * \param[in]   arg     directory to remove
 *
 * \return  CBMDOS error code
 */
static int fsdevice_flush_rmdir(vdrive_t *vdrive, char *arg)
{
    int er = CBMDOS_IPE_OK;

    /* if no dir is set via 'FSDevice[8=11]Dir' this returns '.' */
    char *prefix = fsdevice_get_path(vdrive->unit);

    /* since the cwd can differ from the FSDeviceDir, we need to obtain the
     * absolute path to the directory to remove.
     */
    char *path = util_concat(prefix, FSDEV_DIR_SEP_STR, arg, NULL);

    DBG(("fsdevice_flush_rmdir '%s'\n", arg));

    /* FIXME: rmdir() can set a lot of different errors codes, so this probably
     *        is a little naive
     */
    if (ioutil_rmdir(path) != 0) {
        er = CBMDOS_IPE_NOT_EMPTY;
        if (ioutil_errno(IOUTIL_ERRNO_EPERM)) {
            er = CBMDOS_IPE_PERMISSION;
        }
    }

    DBG(("fsdevice_flush_rmdir %d: %s\n", errno, strerror(errno)));

    lib_free(path);
    return er;
}

static int fsdevice_flush_rename(vdrive_t *vdrive, char *realarg)
{
    char *src, *dest, *tmp, *realsrc;
    unsigned int format = 0, rc;

    DBG(("fsdevice_flush_rename '%s'\n", realarg));
    
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

    if (fsdevice_convert_p00_enabled[(vdrive->unit) - DRIVE_UNIT_MIN]) {
        format |= FILEIO_FORMAT_P00;
    }
    if (!fsdevice_hide_cbm_files_enabled[vdrive->unit - DRIVE_UNIT_MIN]) {
        format |= FILEIO_FORMAT_RAW;
    }

    realsrc = fsdevice_expand_shortname(vdrive, src);
    fsdevice_limit_createnamelength(vdrive, dest);

    DBG(("fsdevice_flush_rename '%s' to '%s'\n", realsrc, dest));
    rc = fileio_rename(realsrc, dest, fsdevice_get_path(vdrive->unit), format);
    
    lib_free(realsrc);

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

    /* FIXME: we need to handle a comma seperated list of files to scratch */
    DBG(("fsdevice_flush_scratch '%s'\n", realarg));
    
    if (realarg == NULL || *realarg == '\0') {
        return CBMDOS_IPE_SYNTAX;
    }

    if (fsdevice_convert_p00_enabled[(vdrive->unit) - DRIVE_UNIT_MIN]) {
        format |= FILEIO_FORMAT_P00;
    }
    if (!fsdevice_hide_cbm_files_enabled[vdrive->unit - DRIVE_UNIT_MIN]) {
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
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int length;
    uint16_t addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 6 + ((realarg != NULL) ? (unsigned int)strlen(realarg) : 0); /* FIXME */
    return vdrive_command_memory_read(vdrive, &fsdevice_dev[dnr].cmdbuf[5], addr, length);
}

/* M-W - Memory Write */
static int fsdevice_flush_mw(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int length;
    uint16_t addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 6 + ((realarg != NULL) ? (unsigned int)strlen(realarg) : 0); /* FIXME */
    return vdrive_command_memory_write(vdrive, &fsdevice_dev[dnr].cmdbuf[5], addr, length);
}

/* M-E - Memory Execute */
static int fsdevice_flush_me(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int length;
    uint16_t addr;

    addr = fsdevice_dev[dnr].cmdbuf[3] | (fsdevice_dev[dnr].cmdbuf[4] << 8);
    length = 5 + ((realarg != NULL) ? (unsigned int)strlen(realarg) : 0); /* FIXME */
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
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int drv, trk, sec;
    unsigned int bamptr, bammask;
    int err = CBMDOS_IPE_OK;

    get4args(realarg, &drv, &trk, &sec, NULL);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-A: %u %u %u (block access needs disk image)",
            drv, trk, sec);

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
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int drv, trk, sec;
    unsigned int bamptr, bammask;

    get4args(realarg, &drv, &trk, &sec, NULL);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-F: %u %u %u (block access needs disk image)",
            drv, trk, sec);

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
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-P: %u %u (block access needs disk image)",
            chn, pos);
    return CBMDOS_IPE_OK;
}

/* B-E - Block Execute */
static int fsdevice_flush_be(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int chn, drv, trk, sec;
    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-E: %u %u %u %u (needs TDE)",
            chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;
    return CBMDOS_IPE_OK;
}

/* B-W - Block Read */
static int fsdevice_flush_br(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int chn, drv, trk, sec;
    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-R: %u %u %u %u (block access needs disk image)",
            chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;
    return CBMDOS_IPE_OK;
}

/* U1, like B-R */
static int fsdevice_flush_u1(vdrive_t *vdrive, char *realarg)
{
    unsigned int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - U1: %u %u %u %u (block access needs disk image)",
            chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* B-W - Block Write */
static int fsdevice_flush_bw(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - B-W: %u %u %u %u (block access needs disk image)",
            chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* U2, like B-W */
static int fsdevice_flush_u2(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    unsigned int chn, drv, trk, sec;

    get4args(realarg, &chn, &drv, &trk, &sec);
    log_message(LOG_DEFAULT,
            "Fsdevice: Warning - U2: %u %u %u %u (block access needs disk image)",
            chn, drv, trk, sec);
    fsdevice_dev[dnr].track = trk;
    fsdevice_dev[dnr].sector = sec;

    return CBMDOS_IPE_OK;
}

/* I - Initialize Disk */
static int fsdevice_flush_initialize(vdrive_t *vdrive)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

/* V - Validate Disk */
static int fsdevice_flush_validate(vdrive_t *vdrive)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

/* N - Format Disk */
static int fsdevice_flush_new(vdrive_t *vdrive, char *realarg)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;

    fsdevice_dev[dnr].track = 1;
    fsdevice_dev[dnr].sector = 0;

    return CBMDOS_IPE_OK;
}

/* P - Position in RELative file */
static int fsdevice_flush_position(vdrive_t *vdrive, char *buf, int length)
{
    int dnr = vdrive->unit - DRIVE_UNIT_MIN;
    bufinfo_t *bufinfo;
    unsigned int channel = buf[1] & 0x0F,
                 rec_lo = buf[2] & 0xFF, rec_hi = buf[3] & 0xFF,
                 position = buf[4] & 0xFF;
    int recno;

    /* P <secaddr> <recno lo> <recno hi> [<pos in record>] */
    switch (length) {
        case 1: /* no channel was specified; return NO CHANNEL */
            return CBMDOS_IPE_NO_CHANNEL;
        case 2: /* default the record number to 1 */
            rec_lo = 1;
            /* fall through */
        case 3: /* default the record number's high byte to 0 */
            rec_hi = 0;
            /* fall through */
        case 4: /* default the position to 1 */
            position = 1;
        default:
            /* make compiler happy */
            break;
    }

    recno = rec_hi * 256 + rec_lo;

    /* Convert 1-based numbers to 0-based */
    if (position > 0)
        position--;
    if (recno > 0)
        recno--;

    DBG(("fsdevice_flush_position: secadr=%d  recno=%d  pos=%d\n", channel, recno, position));
    bufinfo = &fsdevice_dev[dnr].bufinfo[channel];

    return fsdevice_relative_switch_record(vdrive, bufinfo, recno, position);
}

void fsdevice_flush(vdrive_t *vdrive, unsigned int secondary)
{
    unsigned int dnr;
    char *cmd, *realarg, *arg, *realname;
    char *cbmcmd;
    int er = CBMDOS_IPE_SYNTAX;

    dnr = vdrive->unit - DRIVE_UNIT_MIN;

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
    charset_petconvstring((uint8_t *)cbmcmd, 1);   /* CBM name to FSname */
    cmd = cbmcmd;

    while (*cmd == ' ') {
        cmd++;
    }

    /* arg points to the ASCII string after the colon */
    arg = strchr(cbmcmd, ':');
    if (arg != NULL) {
        *arg++ = '\0';
    }

    /* realarg points to the PETSCII string after the colon */
    realarg = strchr((char *)(fsdevice_dev[dnr].cmdbuf), ':');
    if (realarg != NULL) {
        *realarg++ = '\0';
    }

    DBG(("fsdevice_flush arg:'%s' realarg:'%s'\n", arg, realarg));
    
    /*
                                            '41 '71 '81  FD
       i                                      *   *   *   *    initialize disk
       v                                      *   *   *   *    validate BAM
       n:diskname,id                          *   *   *   *    format disk
       r:newname=oldname                      *   *   *   *    rename file
       s:name1,name2,name3                    *   *   *   *    delete file
       c:newname=oldname                      *   *   *   *    copy file (or concat files)
       d:                                   n/a n/a n/a n/a    backup

       p chn lo hi pos                        *   *   *   *    pointer positioning (REL)
       
       b-r chn drv trk sec                    *   *   *   *    block-read
       u1  chn drv trk sec                    *   *   *   *    "
       ua  chn drv trk sec                    *   *   *        "
       b-R chn drv trk sec                  n/a n/a   *        block-read without range check
       b-w chn drv trk sec                    *   *   *   *    block-write 
       u2  chn drv trk sec                    *   *   *   *    "
       ub  chn drv trk sec                    *   *   *        "
       b-W chn drv trk sec                  n/a n/a   *        block-write without range check
       b-p chn pos                            *   *   *   *    buffer-pointer
       b-a drv trk sec                        *   *   *   *    block-allocate
       b-f drv trk sec                        *   *   *   *    block-free
       b-e chn drv trk sec                    *   *   *   *    block execute

       m-r lo hi len                          *   *   *   *    memory read
       m-w lo hi len <data>                   *   *   *   *    memory write
       m-e lo hi                              *   *   *   *    memory execute

       u9/ui                                  *   *   *   *    switch mode (NMI,warmstart)       
       u:/uj                                  *   *   *   *    reset (powerup)
       
       u3/uc                                  *   *   *   *    start at $0500
       u4/ud                                  *   *   *   *    start at $0503
       u5/ue                                  *   *   *   *    start at $0506
       u6/uf                                  *   *   *   *    start at $0509
       u7/ug                                  *   *   *   *    start at $050c
       u8/uh                                  *   *   *   *    start at $050f

       u0                                   n/a   *   *   *    restore user jumptable
       u0>mode                              n/a   * n/a n/a    switch 1541/71 mode
       u0>side                              n/a   * n/a n/a    select active disk side
       u0>devnr                             n/a   * n/a n/a    set device nr.
       u0+cmd                               n/a n/a   *   *    burst utility cmd
       
       cd                                   n/a n/a n/a   *    change directory
       cd_                                  n/a n/a n/a    
       cd:_                                 n/a n/a n/a   * 
       
       /<drv>:name trk src lenlo lenhi,c    n/a n/a   *        partition (create)
       /<drv>:name                          n/a n/a   *        partition (activate)
       
       md                                   n/a n/a n/a   *    make directory
       rd                                   n/a n/a n/a   *    remove directory
       
       cp<num>                              n/a n/a n/a   *    change partition
       g-p                                  n/a n/a n/a   *    get partition info
       
       t-ra                                 n/a n/a n/a   *    read RTC (ascii format)
       t-wa                                 n/a n/a n/a   *    write RTC (ascii format)
       t-rd                                 n/a n/a n/a   *    read RTC (decimal ormat)
       t-wd                                 n/a n/a n/a   *    write RTC (decimal format)
       
       r-h:<name>                           n/a n/a n/a   *    change directory header
       l:<name>                             n/a n/a n/a   *    (un)lock file (toggle)
       w-<state>                            n/a n/a n/a   *    set disk write protection
       s-<dev>                              n/a n/a n/a   *    swap device nr.             
       g-d                                  n/a n/a n/a   *    get disk change status     

    */
    if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-R", 3)) {
        er = fsdevice_flush_mr(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-W", 3)) {
        er = fsdevice_flush_mw(vdrive, realarg);
    } else if (!strncmp((char *)(fsdevice_dev[dnr].cmdbuf), "M-E", 3)) {
        er = fsdevice_flush_me(vdrive, realarg);
    } else if (!strcmp(cmd, "u0")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u1") || !strcmp(cmd, "ua")) {
        er = fsdevice_flush_u1(vdrive, realarg);
    } else if (!strcmp(cmd, "u2") || !strcmp(cmd, "ub")) {
        er = fsdevice_flush_u2(vdrive, realarg);
    } else if (!strcmp(cmd, "u3") || !strcmp(cmd, "uc")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u4") || !strcmp(cmd, "ud")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u5") || !strcmp(cmd, "ue")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u6") || !strcmp(cmd, "uf")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u7") || !strcmp(cmd, "ug")) {
        /* FIXME: not implemented */
    } else if (!strcmp(cmd, "u8") || !strcmp(cmd, "uh")) {
        /* FIXME: not implemented */
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
        realname = fsdevice_expand_shortname_ascii(vdrive, arg);
        er = fsdevice_flush_cd(vdrive, realname);
        lib_free(realname);
    } else if (!strcmp((char *)(fsdevice_dev[dnr].cmdbuf), "CD_")) {
        er = fsdevice_flush_cdup(vdrive);
    } else if (!strcmp((char *)(fsdevice_dev[dnr].cmdbuf), "CD:_")) {
        er = fsdevice_flush_cdup(vdrive);
    } else if (*cmd == '/') {
        er = fsdevice_flush_partition(vdrive, arg);
    } else if (!strcmp(cmd, "md")) {
        /* FIXME: is this really correct? perhaps we must consider a full path
                  here and only limit the last portion? */
        fsdevice_limit_createnamelength(vdrive, arg);
        er = fsdevice_flush_mkdir(vdrive, arg);
    } else if (!strcmp(cmd, "rd")) {
        realname = fsdevice_expand_shortname_ascii(vdrive, arg);
        er = fsdevice_flush_rmdir(vdrive, realname);
        lib_free(realname);
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
    } else if (*cmd == 'c' && arg != NULL) {
        /* FIXME: not implemented */
    } else if (*cmd == 'p') {
        er = fsdevice_flush_position(vdrive,
                (char *)(fsdevice_dev[dnr].cmdbuf),
                fsdevice_dev[dnr].cptr);
    } else if (*cmd == 's' && arg != NULL) {
        /* FIXME: a comma seperated list of files is not handled at all */
        realname = fsdevice_expand_shortname(vdrive, realarg);
        er = fsdevice_flush_scratch(vdrive, realarg);
        lib_free(realname);
    }

    fsdevice_error(vdrive, er);

    fsdevice_dev[dnr].cptr = 0;

    lib_free(cbmcmd);
}

int fsdevice_flush_write_byte(vdrive_t *vdrive, uint8_t data)
{
    unsigned int dnr;
    int rc;

    dnr = vdrive->unit - DRIVE_UNIT_MIN;
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
