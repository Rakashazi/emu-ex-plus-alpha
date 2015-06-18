/*
 * wd1770.c - WD1770/1772 emulation for the 1571 and 1581 disk drives.
 *
 * Rewritten by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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
#include <string.h>

#include "clkguard.h"
#include "diskimage.h"
#include "drive.h"
#include "drivetypes.h"
#include "log.h"
#include "types.h"
#include "wd1770.h"
#include "fdd.h"
#include "lib.h"
#include "snapshot.h"

/* FIXME: msvc sux at var arg defines */
#ifdef WD1770_DEBUG
#define debug1(x) log_message(wd1770_log, x)
#define debug2(x, y) log_message(wd1770_log, x, y)
#define debug3(x, y, z) log_message(wd1770_log, x, y, z)
#else
#define debug1(x)
#define debug2(x, y)
#define debug3(x, y, z)
#endif

static const int wd1770_step_rate[2][4] = {
    {6000, 12000, 20000, 30000}, /* WD1770 */
    {6000, 12000, 2000, 3000},   /* WD1772 */
};

#define SETTLING (drv->clock_frequency * 30000)
#define BYTE_RATE (drv->clock_frequency * 8000 / 250)
#define STEP_RATE (drv->clock_frequency * wd1770_step_rate[drv->is1772][drv->cmd & WD_R])
#define PREPARE (drv->clock_frequency * 24)

/*-----------------------------------------------------------------------*/

/* WD1770/1772 registers.  */
typedef enum wd_reg_e {
    WD_STATUS  = 0,
    WD_COMMAND = 0,
    WD_TRACK   = 1,
    WD_SECTOR  = 2,
    WD_DATA    = 3
} wd_reg_t;

/* WD1770/1772 command bits  */
typedef enum wd_cflags_e {
    WD_A  = 0x01,
    WD_P  = 0x02,
    WD_R  = 0x03,
    WD_V  = 0x04,
    WD_E  = 0x04,
    WD_H  = 0x08,
    WD_U  = 0x10,
    WD_M  = 0x10,
    WD_I0 = 0x01,
    WD_I1 = 0x02,
    WD_I2 = 0x04,
    WD_I3 = 0x08,
} wd_cflags_t;

/* WD1770/1772 status bits  */
typedef enum wd_status_e {
    WD_MO  = 0x80,
    WD_WP  = 0x40,
    WD_SU  = 0x20,
    WD_RT  = 0x20,
    WD_SE  = 0x10,
    WD_RNF = 0x10,
    WD_CRC = 0x08,
    WD_T0  = 0x04,
    WD_LD  = 0x04,
    WD_IP  = 0x02,
    WD_DRQ = 0x02,
    WD_BSY = 0x01
} wd_status_t;

/* WD1770/1772 commands */
typedef enum wd_cmd_e {
    WD_RESTORE            = 0x00,
    WD_SEEK               = 0x10,
    WD_STEP               = 0x20,
    WD_STEP_IN            = 0x40,
    WD_STEP_OUT           = 0x60,
    WD_READ_SECTOR        = 0x80,
    WD_WRITE_SECTOR       = 0xa0,
    WD_READ_ADDRESS       = 0xc0,
    WD_FORCE_INTERRUPT    = 0xd0,
    WD_READ_TRACK         = 0xe0,
    WD_WRITE_TRACK        = 0xf0
} wd_cmd_t;

/* WD1770/1772 commands, masks, types */
static const struct {
    BYTE mask;
    wd_cmd_t command;
    BYTE type;
} wd_commands[11]={
    {0xf0, WD_RESTORE        , 1},
    {0xf0, WD_SEEK           , 1},
    {0xe0, WD_STEP           , 1},
    {0xe0, WD_STEP_IN        , 1},
    {0xe0, WD_STEP_OUT       , 1},
    {0xe0, WD_READ_SECTOR    , 2},
    {0xe0, WD_WRITE_SECTOR   , 2},
    {0xf0, WD_READ_ADDRESS   , 3},
    {0xf0, WD_READ_TRACK     , 3},
    {0xf0, WD_FORCE_INTERRUPT, 4},
    {0xf0, WD_WRITE_TRACK    , 3}
};

struct wd1770_s {
    char *myname;

    /* WD1770/1772 registers.  */
    BYTE data, track, sector, status, cmd;
    WORD crc;

    /* Command and type  */
    wd_cmd_t command;
    int type;

    /* Floppy drive */
    fd_drive_t *fdd;
    int step;
    int byte_count;
    int tmp;
    int direction;
    int clock_frequency; /* MHz of main CPU*/

    CLOCK clk, *cpu_clk_ptr;

    int irq;
    int dden;
    int sync;
    int is1772;
};

static log_t wd1770_log = LOG_ERR;

/*-----------------------------------------------------------------------*/
/* WD1770 external interface.  */

/* Clock overflow handling.  */
static void clk_overflow_callback(CLOCK sub, void *data)
{
    wd1770_t *drv = (wd1770_t *)data;

    if (drv->clk > (CLOCK) 0) {
        drv->clk -= sub;
    }
}

/* Functions using drive context.  */
void wd1770d_init(drive_context_t *drv)
{
    if (wd1770_log == LOG_ERR) {
        wd1770_log = log_open("WD1770");
    }

    drv->wd1770 = lib_calloc(1, sizeof(wd1770_t));
    drv->wd1770->myname = lib_msprintf("WD1770%d", drv->mynumber);
    drv->wd1770->fdd = fdd_init(4 * drv->mynumber, drv->drive);
    drv->wd1770->cpu_clk_ptr = drv->clk_ptr;
    drv->wd1770->is1772 = 0;
    drv->wd1770->clock_frequency = 2;

    clk_guard_add_callback(drv->cpu->clk_guard, clk_overflow_callback,
                           drv->wd1770);
}

void wd1770_shutdown(wd1770_t *drv)
{
    lib_free(drv->myname);
    fdd_shutdown(drv->fdd);
    lib_free(drv);
}

/* Execute microcode */
static void wd1770_execute(wd1770_t *drv)
{
    int res;

    for (;; ) {
        switch (drv->type) {
            case -1:
                drv->status &= ~(WD_WP | WD_IP | WD_T0);
                drv->status |= fdd_index(drv->fdd) ? WD_IP : 0;
                drv->status |= fdd_track0(drv->fdd) ? WD_T0 : 0;
                drv->status |= fdd_write_protect(drv->fdd) ? WD_WP : 0;
            case 0: /* idle */
                if (*drv->cpu_clk_ptr < drv->clk + PREPARE) {
                    return;
                }
                drv->status &= ~WD_BSY;
                drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                if (fdd_index_count(drv->fdd) >= 10) {
                    drv->status &= ~WD_MO;
                }
                if ((drv->cmd & WD_I2) && fdd_index_count(drv->fdd) != drv->tmp) {
                    drv->irq = 1;
                    drv->tmp = fdd_index_count(drv->fdd);
                }
                return;
            case 1: /* type 1 */
                switch (drv->step) {
                    case 0:
                        if (*drv->cpu_clk_ptr < drv->clk + PREPARE) {
                            return;
                        }
                        drv->clk += PREPARE;
                        drv->status |= WD_BSY;
                        drv->status &= ~(WD_CRC | WD_SE | WD_DRQ);
                        drv->irq = 0;
                        drv->step++;
                    case 1:
                        if ((drv->cmd & WD_H) || (drv->status & WD_MO)) {
                            drv->status |= WD_MO;
                            drv->step += 2;
                            continue;
                        }
                        drv->status |= WD_MO;
                        fdd_index_count_reset(drv->fdd);
                        drv->step++;
                    case 2:
                        drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                        if (fdd_index_count(drv->fdd) < 6) {
                            return;
                        }
                        drv->step++;
                    case 3:
                        switch (drv->command) {
                            case WD_STEP:
                                break;
                            case WD_STEP_IN:
                                drv->direction = 1;
                                break;
                            case WD_STEP_OUT:
                                drv->direction = 0;
                                break;
                            case WD_RESTORE:
                                drv->track = 0xff;
                                drv->data = 0x00;
                            default:
                                drv->step++;
                                continue;
                        }
                        drv->step = (drv->cmd & WD_U) ? 5 : 6;
                        continue;
                    case 4:
                        if (drv->data == drv->track) {
                            drv->step = 8;
                            continue;
                        }
                        drv->direction = (drv->data > drv->track);
                        drv->step++;
                    case 5:
                        drv->track += drv->direction ? 1 : -1;
                        drv->step++;
                    case 6:
                        if (fdd_track0(drv->fdd) && !drv->direction) {
                            drv->track = 0;
                            drv->step = 8;
                            continue;
                        }
                        fdd_seek_pulse(drv->fdd, drv->direction);
                        drv->step++;
                    case 7:
                        if (*drv->cpu_clk_ptr < drv->clk + STEP_RATE) {
                            return;
                        }
                        drv->clk += STEP_RATE;
                        if (drv->cmd < WD_STEP) {
                            drv->step = 4;
                            continue;
                        }
                        drv->step++;
                    case 8:
                        if (!(drv->cmd & WD_V)) {
                            drv->type = -1;
                            break;
                        }
                        drv->step++;
                    case 9:
                        if (*drv->cpu_clk_ptr < drv->clk + SETTLING) {
                            return;
                        }
                        drv->clk += SETTLING;
                        fdd_index_count_reset(drv->fdd);
                        drv->sync = 0;
                        drv->step++;
                    case 10:
                        if (fdd_index_count(drv->fdd) >= 6) {
                            drv->status |= WD_SE;
                            drv->type = -1;
                            break;
                        }
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (!drv->dden || res != 0x1fe) {
                            if (!drv->sync || res != 0xfe) {
                                drv->sync = (res == 0x1a1);
                                continue;
                            }
                        }
                        drv->sync = 0;
                        drv->crc = 0xb230;
                        drv->byte_count = 6;
                        drv->step++;
                    case 11:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (drv->byte_count == 6 && res != drv->track) {
                            drv->step--;
                            continue;
                        }
                        drv->crc = fdd_crc(drv->crc, (BYTE)res);
                        if (--drv->byte_count) {
                            continue;
                        }
                        if (drv->crc) {
                            drv->status |= WD_CRC;
                            drv->step--;
                            continue;
                        }
                        drv->status &= ~WD_CRC;
                        drv->type = -1;
                        break;
                }
                break;
            case 2: /* type 2 */
                switch (drv->step) {
                    case 0:
                        if (*drv->cpu_clk_ptr < drv->clk + PREPARE) {
                            return;
                        }
                        drv->clk += PREPARE;
                        drv->status |= WD_BSY;
                        drv->status &= ~(WD_DRQ | WD_LD | WD_RNF | WD_RT | WD_WP);
                        drv->step++;
                    case 1:
                        if ((drv->cmd & WD_H) || (drv->status & WD_MO)) {
                            drv->status |= WD_MO;
                            drv->step += 2;
                            continue;
                        }
                        drv->status |= WD_MO;
                        fdd_index_count_reset(drv->fdd);
                        drv->step++;
                    case 2:
                        drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                        if (fdd_index_count(drv->fdd) < 6) {
                            return;
                        }
                        drv->step++;
                    case 3:
                        if (!(drv->cmd & WD_E)) {
                            drv->step += 2;
                            continue;
                        }
                        drv->step++;
                    case 4:
                        if (*drv->cpu_clk_ptr < drv->clk + SETTLING) {
                            return;
                        }
                        drv->clk += SETTLING;
                        drv->step++;
                    case 5:
                        if (drv->command == WD_WRITE_SECTOR && fdd_write_protect(drv->fdd)) {
                            drv->status |= WD_WP;
                            drv->type = 0;
                            break;
                        }
                        fdd_index_count_reset(drv->fdd);
                        drv->sync = 0;
                        drv->step++;
                    case 6:
                        if (fdd_index_count(drv->fdd) >= 5) {
                            drv->status |= WD_RNF;
                            drv->type = 0;
                            break;
                        }
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (!drv->dden || res != 0x1fe) {
                            if (!drv->sync || res != 0xfe) {
                                drv->sync = (res == 0x1a1);
                                continue;
                            }
                        }
                        drv->sync = 0;
                        drv->crc = 0xb230;
                        drv->byte_count = 6;
                        drv->step++;
                    case 7:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (drv->byte_count == 6 && res != drv->track) {
                            drv->step--;
                            continue;
                        }
                        if (drv->byte_count == 4 && res != drv->sector) {
                            drv->step--;
                            continue;
                        }
                        if (drv->byte_count == 3) {
                            drv->tmp = res;
                        }
                        drv->crc = fdd_crc(drv->crc, (BYTE)res);
                        if (--drv->byte_count) {
                            continue;
                        }
                        if (drv->crc) {
                            drv->status |= WD_CRC;
                            drv->step--;
                            continue;
                        }
                        drv->status &= ~WD_CRC;
                        drv->crc = 0xffff;
                        if (drv->command == WD_WRITE_SECTOR) {
                            drv->byte_count = 0;
                            drv->step = 10;
                            continue;
                        }
                        drv->byte_count = 43;
                        drv->step++;
                    case 8:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        if (!drv->byte_count--) {
                            drv->step -= 2;
                            continue;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (!drv->dden || (res != 0x1fb && res != 0x1f8)) {
                            if (!drv->sync || (res != 0xfb && res != 0xf8)) {
                                if (!drv->sync) {
                                    drv->crc = 0xffff;
                                }
                                drv->crc = fdd_crc(drv->crc, (BYTE)res);
                                drv->sync = (res == 0x1a1);
                                continue;
                            }
                        }
                        drv->crc = fdd_crc(drv->crc, (BYTE)res);
                        drv->status |= ((res & 0xff) == 0xf8) ? WD_RT : 0;
                        drv->byte_count = (128 << drv->tmp) + 2;
                        drv->step++;
                    case 9:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (drv->byte_count > 2) {
                            drv->status |= (drv->status & WD_DRQ) ? WD_LD : WD_DRQ;
                            drv->data = res;
                        }
                        drv->crc = fdd_crc(drv->crc, (BYTE)res);
                        if (--drv->byte_count) {
                            continue;
                        }
                        if (drv->crc) {
                            drv->status |= WD_CRC;
                            drv->type = 0;
                            break;
                        }
                        if (drv->cmd & WD_M) {
                            drv->sector++;
                            drv->step = 5;
                            continue;
                        }
                        drv->type = 0;
                        break;
                    case 10:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        drv->byte_count++;
                        drv->status |= (drv->byte_count == 2) ? WD_DRQ : 0;
                        if (drv->byte_count == (2 + 9) && (drv->status & WD_DRQ)) {
                            drv->status ^= WD_DRQ | WD_LD;
                            drv->type = 0;
                            break;
                        }
                        if (drv->byte_count <= (drv->dden ? 0 : 11) + 2 + 9) {
                            fdd_read(drv->fdd);
                            continue;
                        }
                        if (drv->byte_count <= (drv->dden ? 6 : (11 + 12)) + 2 + 9) {
                            fdd_write(drv->fdd, 0);
                            continue;
                        }
                        if (!drv->dden && drv->byte_count <= (11 + 12 + 2 + 9 + 3)) {
                            fdd_write(drv->fdd, 0x1a1);
                            drv->crc = fdd_crc(drv->crc, 0xa1);
                            continue;
                        }
                        res = ((drv->cmd & WD_A) ? 0xf8 : 0xfb) | (drv->dden ? 0x100 : 0);
                        fdd_write(drv->fdd, (BYTE)res);
                        drv->crc = fdd_crc(drv->crc, (BYTE)res);
                        drv->byte_count = (128 << drv->tmp) + 3;
                        drv->step++;
                    case 11:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        switch (--drv->byte_count) {
                            case 0:
                                fdd_write(drv->fdd, 0xff);
                                break;
                            case 1:
                                fdd_write(drv->fdd, (BYTE)(drv->crc & 0xff));
                                continue;
                            case 2:
                                fdd_write(drv->fdd, (BYTE)(drv->crc >> 8));
                                continue;
                            default:
                                drv->status |= (drv->status & WD_DRQ) ? WD_LD : WD_DRQ;
                                drv->crc = fdd_crc(drv->crc, drv->data);
                                fdd_write(drv->fdd, drv->data);
                                drv->data = 0;
                                continue;
                        }
                        if (drv->cmd & WD_M) {
                            drv->sector++;
                            drv->step = 5;
                            continue;
                        }
                        drv->type = 0;
                        break;
                }
                break;
            case 3: /* type 3 */
                switch (drv->step) {
                    case 0:
                        if (*drv->cpu_clk_ptr < drv->clk + PREPARE) {
                            return;
                        }
                        drv->clk += PREPARE;
                        drv->status |= WD_BSY;
                        drv->status &= ~(WD_DRQ | WD_LD | WD_RNF | WD_CRC);
                        drv->step++;
                    case 1:
                        if ((drv->cmd & WD_H) || (drv->status & WD_MO)) {
                            drv->status |= WD_MO;
                            drv->step += 2;
                            continue;
                        }
                        drv->status |= WD_MO;
                        fdd_index_count_reset(drv->fdd);
                        drv->step++;
                    case 2:
                        drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                        if (fdd_index_count(drv->fdd) < 6) {
                            return;
                        }
                        drv->step++;
                    case 3:
                        if (!(drv->cmd & WD_E)) {
                            drv->step += 2;
                            continue;
                        }
                        drv->step++;
                    case 4:
                        if (*drv->cpu_clk_ptr < drv->clk + SETTLING) {
                            return;
                        }
                        drv->clk += SETTLING;
                        drv->step++;
                    case 5:
                        fdd_index_count_reset(drv->fdd);
                        drv->sync = 0;
                        drv->step++;
                        if (drv->command == WD_WRITE_TRACK) {
                            if (fdd_write_protect(drv->fdd)) {
                                drv->status |= WD_WP;
                                drv->type = 0;
                                break;
                            }
                            drv->status |= WD_DRQ;
                            drv->byte_count = 3;
                            drv->step = 9;
                            continue;
                        }
                        if (drv->command != WD_READ_TRACK) {
                            drv->step++;
                            continue;
                        }
                    case 6:
                        if (fdd_index_count(drv->fdd) < 1) {
                            drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                            return;
                        }
                        if (fdd_index_count(drv->fdd) > 1) {
                            drv->type = 0;
                            break;
                        }
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        drv->data = (BYTE)fdd_read(drv->fdd);
                        drv->status |= (drv->status & WD_DRQ) ? WD_LD : WD_DRQ;
                        continue;
                    case 7:
                        if (fdd_index_count(drv->fdd) >= 6) {
                            drv->status |= WD_RNF;
                            drv->type = 0;
                            break;
                        }
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = fdd_read(drv->fdd);
                        if (!drv->dden || res != 0x1fe) {
                            if (!drv->sync || res != 0xfe) {
                                drv->sync = (res == 0x1a1);
                                continue;
                            }
                        }
                        drv->crc = 0xb230;
                        drv->byte_count = 6;
                        drv->step++;
                    case 8:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->status |= (drv->status & WD_DRQ) ? WD_LD : WD_DRQ;
                        drv->clk += BYTE_RATE;
                        drv->data = (BYTE)fdd_read(drv->fdd);
                        if (drv->byte_count == 6) {
                            drv->sector = drv->data;
                        }
                        drv->crc = fdd_crc(drv->crc, drv->data);
                        if (--drv->byte_count) {
                            continue;
                        }
                        drv->status |= drv->crc ? WD_CRC : 0;
                        drv->type = 0;
                        break;
                    case 9:
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        fdd_read(drv->fdd);
                        if (--drv->byte_count) {
                            continue;
                        }
                        if (drv->status & WD_DRQ) {
                            drv->status ^= WD_DRQ | WD_LD;
                            drv->type = 0;
                            break;
                        }
                        drv->byte_count = 0;
                        drv->tmp = 0;
                        drv->step++;
                    case 10:
                        if (fdd_index_count(drv->fdd) < 1) {
                            drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
                            return;
                        }
                        if (fdd_index_count(drv->fdd) > 1) {
                            drv->status &= ~WD_DRQ;
                            drv->type = 0;
                            break;
                        }
                        if (*drv->cpu_clk_ptr < drv->clk + BYTE_RATE) {
                            return;
                        }
                        drv->clk += BYTE_RATE;
                        res = drv->data;
                        if (drv->byte_count) {
                            fdd_write(drv->fdd, (BYTE)(drv->crc & 0xff));
                            drv->byte_count--;
                        } else {
                            drv->status |= (drv->status & WD_DRQ) ? WD_LD : WD_DRQ;

                            if (drv->dden) {
                                switch (res) {
                                    case 0xf7:
                                        drv->byte_count = 1;
                                        res = drv->crc >> 8;
                                        drv->tmp = 0;
                                        break;
                                    case 0xf8:
                                    case 0xf9:
                                    case 0xfa:
                                    case 0xfb:
                                    case 0xfe:
                                        if (!drv->tmp) {
                                            drv->crc = 0xffff;
                                            drv->tmp = 1;
                                        }
                                    case 0xfc:
                                        res |= 0x100;
                                }
                            } else {
                                switch (res) {
                                    case 0xf5:
                                        res = 0x1a1;
                                        if (!drv->tmp) {
                                            drv->crc = 0xffff;
                                            drv->tmp = 1;
                                        }
                                        break;
                                    case 0xf6:
                                        res = 0x1c2;
                                        break;
                                    case 0xf7:
                                        drv->byte_count = 1;
                                        res = drv->crc >> 8;
                                        drv->tmp = 0;
                                        break;
                                }
                            }
                            if (drv->tmp) {
                                drv->crc = fdd_crc(drv->crc, (BYTE)res);
                            }
                            fdd_write(drv->fdd, (BYTE)res);
                            drv->data = 0;
                        }
                        continue;
                }
                break;
            case 4: /* type 4 */
                if (*drv->cpu_clk_ptr < drv->clk + PREPARE) {
                    return;
                }
                drv->clk += PREPARE;
                drv->status &= WD_BSY;
                if (drv->cmd & WD_I3) {
                    drv->irq = 1;
                }
                fdd_index_count_reset(drv->fdd);
                drv->tmp = fdd_index_count(drv->fdd);
                drv->type = (drv->status & WD_BSY) ? 0 : -1;
                continue;
        }
        drv->cmd = 0;
        drv->irq = 1;
        fdd_index_count_reset(drv->fdd);
    }
}
/*-----------------------------------------------------------------------*/
/* WD1770 register read/write access.  */

static void wd1770_store(wd1770_t *drv, WORD addr, BYTE byte)
{
    int i;

    wd1770_execute(drv);

    switch (addr) {
        case WD_COMMAND:
            drv->cmd = byte;
            for (i = 0; i < sizeof(wd_commands) / sizeof(wd_commands[0]); i++) {
                if (wd_commands[i].command == (wd_cmd_t)(wd_commands[i].mask & byte)) {
                    break;
                }
            }
            drv->command = wd_commands[i].command;
            drv->type = wd_commands[i].type;
            drv->clk += fdd_rotate(drv->fdd, (*drv->cpu_clk_ptr - drv->clk) / BYTE_RATE) * BYTE_RATE;
            drv->step = 0;
            switch (drv->command) {
                case WD_RESTORE:
                    debug1("RESTORE");
                    break;
                case WD_SEEK:
                    debug2("SEEK %d", drv->data);
                    break;
                case WD_STEP:
                    debug2("STEP %d", drv->direction ? 1 : -1);
                    break;
                case WD_STEP_IN:
                    debug1("STEP IN");
                    break;
                case WD_STEP_OUT:
                    debug1("STEP OUT");
                    break;
                case WD_READ_SECTOR:
                    debug3("READ SECTOR %d/%d", drv->track, drv->sector);
                    break;
                case WD_WRITE_SECTOR:
                    debug3("WRITE SECTOR %d/%d", drv->track, drv->sector);
                    break;
                case WD_READ_ADDRESS:
                    debug1("READ ADDRESS");
                    break;
                case WD_READ_TRACK:
                    debug1("READ TRACK");
                    break;
                case WD_FORCE_INTERRUPT:
                    debug1("FORCE INTERRUPT");
                    break;
                case WD_WRITE_TRACK:
                    debug1("WRITE TRACK");
                    break;
            }
            wd1770_execute(drv);
            break;
        case WD_TRACK:
            drv->track = byte;
            break;
        case WD_SECTOR:
            drv->sector = byte;
            break;
        case WD_DATA:
            drv->status &= ~WD_DRQ;
            drv->data = byte;
            break;
    }
}

static BYTE wd1770_read(wd1770_t *drv, WORD addr)
{
    wd1770_execute(drv);

    switch (addr) {
        case WD_STATUS:
            drv->irq = 0;
            return drv->status;
        case WD_TRACK:
            return drv->track;
        case WD_SECTOR:
            return drv->sector;
        case WD_DATA:
            drv->status &= ~WD_DRQ;
            return drv->data;
    }
    return 0;
}

void wd1770_reset(wd1770_t *drv)
{
    drv->type = 0;
    drv->status = 0;
    drv->track = 0;
    drv->sector = 0;
    drv->data = 0;
    drv->cmd = 0;
    drv->step = -1;
    drv->clk = *drv->cpu_clk_ptr;
}

/*-----------------------------------------------------------------------*/

int wd1770_attach_image(disk_image_t *image, unsigned int unit)
{
    if (unit < 8 || unit > 8 + DRIVE_NUM) {
        return -1;
    }

    switch (image->type) {
        case DISK_IMAGE_TYPE_D81:
        case DISK_IMAGE_TYPE_D1M:
            disk_image_attach_log(image, wd1770_log, unit);
            break;
        default:
            return -1;
    }

    fdd_image_attach(drive_context[unit - 8]->wd1770->fdd, image);
    return 0;
}

int wd1770_detach_image(disk_image_t *image, unsigned int unit)
{
    if (image == NULL || unit < 8 || unit > 8 + DRIVE_NUM) {
        return -1;
    }

    switch (image->type) {
        case DISK_IMAGE_TYPE_D81:
        case DISK_IMAGE_TYPE_D1M:
            disk_image_detach_log(image, wd1770_log, unit);
            break;
        default:
            return -1;
    }

    fdd_image_detach(drive_context[unit - 8]->wd1770->fdd);
    return 0;
}

inline void wd1770_set_side(wd1770_t *drv, int side)
{
    fdd_select_head(drv->fdd, side);
}

inline void wd1770_set_motor(wd1770_t *drv, int on)
{
    fdd_set_motor(drv->fdd, on);
}

inline int wd1770_disk_change(wd1770_t *drv)
{
    return fdd_disk_change(drv->fdd);
}

inline void wd1770d_store(drive_context_t *drv, WORD addr, BYTE byte)
{
    wd1770_store(drv->wd1770, (WORD)(addr & 3), byte);
}

inline BYTE wd1770d_read(drive_context_t *drv, WORD addr)
{
    return wd1770_read(drv->wd1770, (WORD)(addr & 3));
}

#define WD1770_SNAP_MAJOR 1
#define WD1770_SNAP_MINOR 0

int wd1770_snapshot_write_module(wd1770_t *drv, struct snapshot_s *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, drv->myname,
                               WD1770_SNAP_MAJOR, WD1770_SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    SMW_B(m, drv->data);
    SMW_B(m, drv->track);
    SMW_B(m, drv->sector);
    SMW_B(m, drv->status);
    SMW_B(m, drv->cmd);
    SMW_W(m, drv->crc);
    SMW_B(m, (BYTE)drv->command);
    SMW_DW(m, drv->type);
    SMW_DW(m, drv->step);
    SMW_DW(m, drv->byte_count);
    SMW_DW(m, drv->tmp);
    SMW_DW(m, drv->direction);
    SMW_DW(m, drv->clk);
    SMW_B(m, (BYTE)drv->irq);
    SMW_B(m, (BYTE)drv->dden);
    SMW_B(m, (BYTE)drv->sync);
    SMW_B(m, (BYTE)drv->is1772);

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return fdd_snapshot_write_module(drv->fdd, s);
}

int wd1770_snapshot_read_module(wd1770_t *drv, struct snapshot_s *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int command;

    m = snapshot_module_open(s, drv->myname, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != WD1770_SNAP_MAJOR) || (vminor != WD1770_SNAP_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_B(m, &drv->data);
    SMR_B(m, &drv->track);
    SMR_B(m, &drv->sector);
    SMR_B(m, &drv->status);
    SMR_B(m, &drv->cmd);
    SMR_W(m, &drv->crc);
    SMR_B_INT(m, &command);
    drv->command = command;
    SMR_DW_INT(m, &drv->type);
    SMR_DW_INT(m, &drv->step);
    SMR_DW_INT(m, &drv->byte_count);
    SMR_DW_INT(m, &drv->tmp);
    SMR_DW_INT(m, &drv->direction);
    SMR_DW(m, &drv->clk);
    SMR_B_INT(m, &drv->irq);
    SMR_B_INT(m, &drv->dden);
    SMR_B_INT(m, &drv->sync);
    SMR_B_INT(m, &drv->is1772);

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return fdd_snapshot_read_module(drv->fdd, s);
}
