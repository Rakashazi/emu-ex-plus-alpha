/*
 * ata.c - ATA(PI) device emulation
 *
 * Written by
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

/* required for off_t on some platforms */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* VAC++ has off_t in sys/stat.h */
#ifdef __IBMC__
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "log.h"
#include "ata.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "lib.h"
#include "alarm.h"
#include "maincpu.h"
#include "monitor.h"

#ifndef HAVE_FSEEKO
#define fseeko(a, b, c) fseek(a, b, c)
#define ftello(a) ftell(a)
#endif

#define ATA_UNC  0x40
#define ATA_IDNF 0x10
#define ATA_ABRT 0x04
#define ATA_WP 0x40
#define ATA_DRDY 0x40
#define ATA_DRQ 0x08
#define ATA_ERR 0x01
#define ATA_COPYRIGHT "KAJTAR ZSOLT (SOCI/SINGULAR)"
#define ATA_SERIAL_NUMBER &"$Date:: 2015-02-17 13:41:45 #$"[8]
#define ATA_REVISION &"$Revision:: 29352    $"[12]

#ifdef ATA_DEBUG
#define debug(_x_) log_message _x_
#else
#define debug(_x_)
#endif
#define putw(a, b) {result[(a) * 2] = (b) & 0xff; result[(a) * 2 + 1] = (b) >> 8; }
#define setb(a, b, c) {result[(a) * 2 + (b) / 8] |= (c) ? (1 << ((b) & 7)) : 0; }

struct ata_drive_s {
    BYTE error;
    BYTE features;
    BYTE sector_count, sector_count_internal;
    BYTE sector;
    WORD cylinder;
    BYTE head;
    int lba, dev, legacy;
    BYTE control;
    BYTE cmd;
    BYTE power;
    BYTE packet[12];
    int bufp;
    BYTE *buffer;
    FILE *file;
    char *filename;
    char *myname;
    ata_drive_geometry_t geometry;
    int cylinders, heads, sectors;
    int slave;
    int readonly;
    int attention;
    int locked;
    int wcache;
    int lookahead;
    ata_drive_type_t type;
    int busy; /* bits: spinup, seek, reset */
    int pos;
    int standby, standby_max;
    alarm_t *spindle_alarm;
    alarm_t *head_alarm;
    alarm_t *standby_alarm;
    log_t log;
    int sector_size;
    int atapi, lbamode, pmcommands, wbuffer, rbuffer, flush;
    CLOCK seek_time;
    CLOCK spinup_time, spindown_time;
    CLOCK cycles_1s;
};

static const BYTE identify[128] = {
    0x40, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x30, 0x32, 0x31, 0x31,
    0x38, 0x30, 0x30, 0x32, 0x20, 0x20, 0x20, 0x20,

    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x36,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x49, 0x56,
    0x45, 0x43, 0x48, 0x2d, 0x44, 0x44, 0x20, 0x20,

    0x20, 0x20, 0x41, 0x4b, 0x54, 0x4a, 0x52, 0x41,
    0x5a, 0x20, 0x4f, 0x53, 0x54, 0x4c, 0x28, 0x20,
    0x4f, 0x53, 0x49, 0x43, 0x53, 0x2f, 0x4e, 0x49,
    0x55, 0x47, 0x41, 0x4c, 0x29, 0x52, 0x01, 0x00,

    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x04, 0x00,
    0x10, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x01,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void ident_update_string(BYTE *b, char *s, int n)
{
    int i;

    for (i = 0; i < n; i += 2) {
        b[i | 1] = *s ? *s++ : 0x20;
        b[i] = *s ? *s++ : 0x20;
    }
}

static void ata_change_power_mode(ata_drive_t *drv, BYTE value)
{
    if (drv->power == 0x00 && value != 0x00) {
        drv->busy |= 1;
        alarm_set(drv->spindle_alarm, maincpu_clk + drv->spinup_time);
    }
    if (drv->power != 0x00 && value == 0x00) {
        drv->busy |= 1;
        alarm_set(drv->spindle_alarm, maincpu_clk + drv->spindown_time);
    }
    if (value == 0x00) {
        drv->standby = 0;
    } else {
        drv->standby = drv->standby_max;
    }
    if (value != 0xff) {
        drv->pos = 0;
    }
    if (drv->standby) {
        alarm_set(drv->standby_alarm, maincpu_clk + 5 * drv->cycles_1s);
    } else {
        alarm_unset(drv->standby_alarm);
    }
    drv->power = value;
}

static int ata_set_standby(ata_drive_t *drv, int value)
{
    if (value == 254) {
        return -1;
    }
    drv->standby_max = value;
    if (value > 0 && value < 12) {
        drv->standby_max = 12;
    }
    if (value > 240 && value < 252) {
        drv->standby_max = (value - 240) * 12 * 30;
    }
    if (value == 253) {
        drv->standby_max = 8 * 12 * 60;
    }
    return 0;
}

static void drive_diag(ata_drive_t *drv)
{
    drv->error = 1;
    drv->sector_count = 1;
    drv->sector = 1;
    drv->cylinder = drv->atapi ? 0xeb14 : 0x0000;
    drv->head = 0;
    drv->lba = 0;
    drv->dev = drv->slave;
    drv->legacy = 0;
    drv->bufp = drv->sector_size;
    drv->cmd = 0x08;
}

static void ata_set_command_block(ata_drive_t *drv)
{
    if (drv->atapi) {
        return;
    }
    if (drv->lbamode && drv->lba) {
        drv->head = (drv->pos >> 24) & 0xf;
        drv->cylinder = drv->pos >> 8;
        drv->sector = drv->pos;
        return;
    }
    drv->sector = drv->pos % drv->sectors + 1;
    drv->head = (drv->pos / drv->sectors) % drv->heads;
    drv->cylinder = drv->pos / drv->sectors / drv->heads;
    return;
}

static int seek_sector(ata_drive_t *drv)
{
    int lba;

    drv->bufp = drv->sector_size;
    drv->error = 0;
    drv->cmd = 0x00;

    if (drv->atapi) {
        lba = (drv->packet[2] << 24) | (drv->packet[3] << 16) | (drv->packet[4] << 8) | drv->packet[5];
    } else if (drv->lbamode && drv->lba) {
        lba = (drv->head << 24) | (drv->cylinder << 8) | drv->sector;
    } else {
        if (drv->sector == 0 || drv->sector > drv->sectors || drv->head >= drv->heads ||
            drv->cylinder >= drv->cylinders) {
            lba = -1;
        }
        lba = (drv->cylinder * drv->heads + drv->head) * drv->sectors + drv->sector - 1;
    }

    if (!drv->file) {
        drv->error = drv->atapi ? 0x24 : ATA_ABRT;
        return drv->error;
    }
    if (lba >= drv->geometry.size || lba < 0) {
        drv->error = drv->atapi ? 0x54 : ATA_IDNF;
        return drv->error;
    }
    drv->busy |= 2;
    alarm_set(drv->head_alarm, maincpu_clk + (CLOCK)(abs(drv->pos - lba) * drv->seek_time / drv->geometry.size));
    ata_change_power_mode(drv, 0xff);
    if (fseeko(drv->file, (off_t)lba * drv->sector_size, SEEK_SET)) {
        drv->error = drv->atapi ? 0x54 : ATA_IDNF;
    }
    drv->pos = lba;
    return drv->error;
}

static void debug_addr(ata_drive_t *drv, char *cmd)
{
    if (drv->lbamode && drv->lba) {
        debug((drv->log, "%s (%d)*%d", cmd, (drv->head << 24) | (drv->cylinder << 8) | drv->sector, drv->sector_count ? drv->sector_count : 256));
    } else {
        debug((drv->log, "%s (%d/%d/%d)*%d", cmd, drv->cylinder, drv->head, drv->sector, drv->sector_count ? drv->sector_count : 256));
    }
}

static int read_sector(ata_drive_t *drv)
{
    drv->bufp = drv->sector_size;
    drv->error = 0;

    if (drv->attention && drv->atapi) {
        drv->attention = 0;
        drv->error = 0x64;
        drv->cmd = 0x00;
        return drv->error;
    }

    if (!drv->file) {
        ata_set_command_block(drv);
        drv->error = drv->atapi ? 0x24 : ATA_ABRT;
        drv->cmd = 0x00;
        return drv->error;
    }

    clearerr(drv->file);
    if (fread(drv->buffer, drv->sector_size, 1, drv->file) != 1) {
        memset(drv->buffer, 0, drv->sector_size);
    }

    if (ferror(drv->file)) {
        ata_set_command_block(drv);
        drv->error = drv->atapi ? 0x54 : (ATA_UNC | ATA_ABRT);
        drv->cmd = 0x00;
    } else {
        drv->pos++;
        drv->bufp = 0;
    }
    return drv->error;
}

static int write_sector(ata_drive_t *drv)
{
    drv->bufp = drv->sector_size;
    drv->error = 0;

    if (drv->attention && drv->atapi) {
        drv->attention = 0;
        drv->error = 0x64;
        drv->cmd = 0x00;
        return drv->error;
    }

    if (!drv->file) {
        ata_set_command_block(drv);
        drv->error = drv->atapi ? 0x24 : ATA_ABRT;
        drv->cmd = 0x00;
        return drv->error;
    }

    if (drv->readonly) {
        ata_set_command_block(drv);
        drv->error = drv->atapi ? 0x74 : (ATA_WP | ATA_ABRT);
        drv->cmd = 0x00;
        return drv->error;
    }

    if (fwrite(drv->buffer, 1, drv->sector_size, drv->file) != (size_t)drv->sector_size) {
        ata_set_command_block(drv);
        drv->error = drv->atapi ? 0x54 : (ATA_UNC | ATA_ABRT);
        drv->cmd = 0x00;
    } else {
        drv->pos++;
    }

    if (!drv->wcache) {
        if (fflush(drv->file)) {
            ata_set_command_block(drv);
            drv->error = drv->atapi ? 0x54 : (ATA_UNC | ATA_ABRT);
            drv->cmd = 0x00;
        }
    }
    return drv->error;
}

void ata_reset(ata_drive_t *drv)
{
    BYTE oldcmd = drv->cmd;

    drive_diag(drv);

    if (oldcmd != 0xe6) {
        drv->dev = 0;
        drv->sectors = drv->geometry.sectors;
        drv->heads = drv->geometry.heads;
        drv->cylinders = drv->geometry.cylinders;
    }
}

void ata_update_timing(ata_drive_t *drv, CLOCK cycles_1s)
{
    drv->cycles_1s = cycles_1s;
    switch (drv->type) {
        case ATA_DRIVE_FDD:
            drv->seek_time = (CLOCK)drv->cycles_1s * 120 / 1000;
            drv->spinup_time = (CLOCK)drv->cycles_1s * 800 / 1000;
            drv->spindown_time = (CLOCK)drv->cycles_1s * 500 / 1000;
            break;
        case ATA_DRIVE_CD:
            drv->seek_time = (CLOCK)drv->cycles_1s * 190 / 1000;
            drv->spinup_time = (CLOCK)drv->cycles_1s * 2800 / 1000;
            drv->spindown_time = (CLOCK)drv->cycles_1s * 2000 / 1000;
            break;
        case ATA_DRIVE_HDD:
            drv->seek_time = (CLOCK)drv->cycles_1s * 16 / 1000;
            drv->spinup_time = (CLOCK)drv->cycles_1s * 3000 / 1000;
            drv->spindown_time = (CLOCK)drv->cycles_1s * 2000 / 1000;
            break;
        case ATA_DRIVE_CF:
            drv->seek_time = (CLOCK)drv->cycles_1s * 10 / 1000000;
            drv->spinup_time = (CLOCK)drv->cycles_1s * 300 / 1000;
            drv->spindown_time = (CLOCK)drv->cycles_1s * 2 / 1000;
            break;
        default:
            drv->seek_time = (CLOCK)0;
            drv->spinup_time = (CLOCK)0;
            drv->spindown_time = (CLOCK)0;
            break;
    }
    return;
}

static void ata_poweron(ata_drive_t *drv, ata_drive_type_t type)
{
    int i, size, c, h, s;

    drv->wcache = 0;
    drv->lookahead = 0;
    drv->power = 0x00;
    drv->attention = 1;
    drv->cmd = 0x00;
    drv->standby_max = 0;
    drv->pos = 0;
    drv->busy = 0;
    drv->control = 0;

    drv->lbamode = 1;
    drv->flush = 1;
    drv->pmcommands = 1;
    drv->rbuffer = 1;
    drv->wbuffer = 1;
    drv->type = type;
    ata_update_timing(drv, drv->cycles_1s);
    switch (drv->type) {
        case ATA_DRIVE_FDD:
            drv->atapi = 1;
            drv->locked = 0;
            drv->sector_size = 512;
            drv->readonly = 0;
            break;
        case ATA_DRIVE_CD:
            drv->atapi = 1;
            drv->locked = 0;
            drv->sector_size = 2048;
            drv->readonly = 1;
            break;
        case ATA_DRIVE_HDD:
            drv->atapi = 0;
            drv->locked = 1;
            drv->sector_size = 512;
            drv->readonly = 0;
            break;
        case ATA_DRIVE_CF:
            drv->atapi = 0;
            drv->locked = 1;
            drv->sector_size = 512;
            drv->readonly = 0;
            break;
        default:
            drv->atapi = 0;
            drv->locked = 0;
            drv->sector_size = 512;
            drv->readonly = 1;
            drv->type = ATA_DRIVE_NONE;
            return;
    }

    if (!drv->atapi && (drv->geometry.sectors < 1 || drv->geometry.sectors > 63 || drv->geometry.cylinders > 65535 ||
                        (drv->geometry.sectors * drv->geometry.heads * drv->geometry.cylinders) > 16514064)) {
        size = drv->geometry.size;

        if (size > 16514064) {
            size = 16514064;
        }
        h = 1; s = 1; i = 63; c = size;
        while (i > 1 && c > 1) {
            if ((c % i) == 0) {
                if (s * i <= 63) {
                    s *= i; c /= i;
                    continue;
                }
                if (h * i <= 16) {
                    h *= i; c /= i;
                    continue;
                }
            }
            i--;
        }
        for (;; ) {
            if (size <= 1032192) {
                if (c <= 1024) {
                    break;
                }
            } else {
                if (h < 5 && c < 65536) {
                    break;
                }
                if (h < 9 && c < 32768) {
                    break;
                }
                if (c < 16384) {
                    break;
                }
            }
            if (s == 63 && h < 16) {
                h++;
            }
            if (s < 63) {
                s++;
            }
            c = size / (h * s);
        }
        drv->geometry.cylinders = c;
        drv->geometry.heads = h;
        drv->geometry.sectors = s;
    }

    ata_reset(drv);
    ata_change_power_mode(drv, 0xff);
}

static void ata_spindle_alarm_handler(CLOCK offset, void *data)
{
    ata_drive_t *drv = (ata_drive_t *)data;

    drv->busy &= ~1;
    alarm_unset(drv->spindle_alarm);
}

static void ata_head_alarm_handler(CLOCK offset, void *data)
{
    ata_drive_t *drv = (ata_drive_t *)data;

    drv->busy &= ~2;
    alarm_unset(drv->head_alarm);
}

static void ata_standby_alarm_handler(CLOCK offset, void *data)
{
    ata_drive_t *drv = (ata_drive_t *)data;

    if (drv->standby) {
        drv->standby--;
        alarm_set(drv->standby_alarm, maincpu_clk + 5 * drv->cycles_1s);
        drv->power = 0x80;
        drv->pos = 0;
    } else {
        alarm_unset(drv->standby_alarm);
        drv->power = 0x00;
    }
}

ata_drive_t *ata_init(int drive)
{
    char *name;
    ata_drive_t *drv = lib_malloc(sizeof(ata_drive_t));

    drv->myname = lib_msprintf("ATA%d", drive);
    drv->log = log_open(drv->myname);
    drv->file = NULL;
    drv->filename = NULL;
    drv->buffer = lib_malloc(2048);
    drv->slave = drive & 1;
    drv->cycles_1s = (CLOCK)1000000;
    ata_poweron(drv, ATA_DRIVE_NONE);
    name = lib_msprintf("%sSPINDLE", drv->myname);
    drv->spindle_alarm = alarm_new(maincpu_alarm_context, name, ata_spindle_alarm_handler, drv);
    lib_free(name);
    name = lib_msprintf("%sHEAD", drv->myname);
    drv->head_alarm = alarm_new(maincpu_alarm_context, name, ata_head_alarm_handler, drv);
    lib_free(name);
    name = lib_msprintf("%sSTANDBY", drv->myname);
    drv->standby_alarm = alarm_new(maincpu_alarm_context, name, ata_standby_alarm_handler, drv);
    lib_free(name);
    return drv;
}

void ata_shutdown(ata_drive_t *drv)
{
    if (drv->filename) {
        lib_free(drv->filename);
        drv->filename = NULL;
    }
    alarm_destroy(drv->spindle_alarm);
    alarm_destroy(drv->head_alarm);
    alarm_destroy(drv->standby_alarm);
    log_close(drv->log);
    lib_free(drv->myname);
    lib_free(drv->buffer);
    lib_free(drv);
}

static void ata_execute_command(ata_drive_t *drv, BYTE value)
{
    BYTE result[512];
    int i;

    if (drv->cmd == 0xe6) {
        return;
    }
    drv->bufp = drv->sector_size;
    drv->error = 0;
    drv->cmd = 0x00;
    switch (value) {
        case 0x00:
            debug((drv->log, "NOP"));
            drv->error = ATA_ABRT;
            return;
        case 0x20:
        case 0x21:
            debug_addr(drv, "READ SECTORS");
            drv->sector_count_internal = drv->sector_count;
            if (seek_sector(drv)) {
                return;
            }
            drv->cmd = 0x20;
            read_sector(drv);
            return;
        case 0x30:
        case 0x31:
            debug_addr(drv, "WRITE SECTORS");
            drv->sector_count_internal = drv->sector_count;
            if (seek_sector(drv)) {
                return;
            }
            if (drv->readonly) {
                drv->error = ATA_WP | ATA_ABRT;
                return;
            }
            drv->bufp = 0;
            drv->cmd = 0x30;
            return;
        case 0x40:
        case 0x41:
            debug_addr(drv, "READ VERIFY SECTORS");
            drv->sector_count_internal = drv->sector_count;
            if (seek_sector(drv)) {
                return;
            }
            do {
                read_sector(drv);
            } while (!drv->error && --drv->sector_count_internal);
            return;
        case 0x70:
            if (drv->lbamode && drv->lba) {
                debug((drv->log, "SEEK (%d)", (drv->head << 24) | (drv->cylinder << 8) | drv->sector));
            } else {
                debug((drv->log, "SEEK (%d/%d/%d)", drv->cylinder, drv->head, drv->sector));
            }
            seek_sector(drv);
            return;
        case 0x90:
            debug((drv->log, "EXECUTE DEVICE DIAGNOSTIC"));
            drive_diag(drv);
            return;
        case 0x91:
            drv->heads = drv->head + 1;
            drv->sectors = drv->sector_count;
            if (drv->sectors < 1 || drv->sectors > 63) {
                drv->cylinders = 0;
            } else {
                int size = drv->geometry.size;
                if (size > 16514064) {
                    size = 16514064;
                }
                size /= drv->heads * drv->sectors;
                drv->cylinders = (size > 65535) ? 65535 : size;
            }
            debug((drv->log, "INITIALIZE DEVICE PARAMETERS (%d/%d/%d)", drv->cylinders, drv->heads, drv->sectors));
            if (drv->cylinders == 0) {
                drv->heads = 0;
                drv->sectors = 0;
                drv->error = ATA_ABRT;
                return;
            }
            return;
        case 0x94:
        case 0xe0:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "STANDBY IMMEDIATE"));
            ata_change_power_mode(drv, 0x00);
            return;
        case 0x95:
        case 0xe1:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "IDLE IMMEDIATE"));
            ata_change_power_mode(drv, 0x80);
            return;
        case 0x96:
        case 0xe2:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "STANDBY %02x", drv->sector_count));
            if (ata_set_standby(drv, drv->sector_count)) {
                break;
            }
            ata_change_power_mode(drv, 0x00);
            return;
        case 0x97:
        case 0xe3:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "IDLE %02x", drv->sector_count));
            if (ata_set_standby(drv, drv->sector_count)) {
                break;
            }
            ata_change_power_mode(drv, 0x80);
            return;
        case 0xe4:
            if (!drv->rbuffer) {
                break;
            }
            debug((drv->log, "READ BUFFER"));
            drv->sector_count_internal = 1;
            drv->bufp = 0;
            drv->cmd = 0xe4;
            return;
        case 0x98:
        case 0xe5:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "CHECK POWER MODE"));
            drv->sector_count = drv->power;
            drv->cmd = 0xe5;
            return;
        case 0x99:
        case 0xe6:
            if (!drv->pmcommands) {
                break;
            }
            debug((drv->log, "SLEEP"));
            if (drv->type != ATA_DRIVE_CF) {
                drv->cmd = 0xe6;
            }
            ata_change_power_mode(drv, 0x00);
            return;
        case 0xe7:
            if (!drv->flush) {
                break;
            }
            debug((drv->log, "FLUSH CACHE"));
            if (drv->file) {
                if (fflush(drv->file)) {
                    drv->error = drv->atapi ? 0x54 : (ATA_UNC | ATA_ABRT);
                }
            }
            return;
        case 0xe8:
            if (!drv->wbuffer) {
                break;
            }
            debug((drv->log, "WRITE BUFFER"));
            drv->sector_count_internal = 1;
            drv->bufp = 0;
            drv->cmd = 0xe8;
            return;
        case 0xec:
            memset(result, 0, sizeof(result));
            debug((drv->log, "IDENTIFY DEVICE"));
            putw(0, (drv->type == ATA_DRIVE_HDD) ? 0x0040 : 0x848a);
            putw(1, drv->geometry.cylinders);
            putw(3, drv->geometry.heads);
            if (drv->type != ATA_DRIVE_CF) {
                putw(4, drv->sector_size * drv->geometry.sectors);
                putw(5, drv->sector_size);
            }
            putw(6, drv->geometry.sectors);
            if (drv->type == ATA_DRIVE_CF) {
                putw(7, drv->geometry.size >> 16);
                putw(8, drv->geometry.size);
            }
            ident_update_string(result + 20, ATA_SERIAL_NUMBER, 20);
            putw(21, BUFSIZ / drv->sector_size);
            ident_update_string(result + 46, ATA_REVISION, 8);
            if (drv->type == ATA_DRIVE_HDD) {
                ident_update_string(result + 54, "ATA-HDD " ATA_COPYRIGHT, 40);
            } else {
                ident_update_string(result + 54, "ATA-CFA " ATA_COPYRIGHT, 40);
            }
            setb(49, 13, 1); /* standard timers */
            setb(49, 9, drv->lbamode); /* LBA support */
            if (drv->sectors) {
                setb(53, 0, 1);
                putw(54, drv->cylinders);
                putw(55, drv->heads);
                putw(56, drv->sectors);
                i = drv->cylinders * drv->heads * drv->sectors;
                if (i > drv->geometry.size) {
                    i = drv->geometry.size;
                }
                putw(57, i);
                putw(58, i >> 16);
            }
            if (drv->lbamode) {
                putw(60, drv->geometry.size);
                putw(61, drv->geometry.size >> 16);
            }
            setb(82, 3, drv->pmcommands); /* pm command set */
            setb(82, 4, drv->atapi); /* packet */
            setb(82, 5, 1); /* write cache */
            setb(82, 6, 1); /* look-ahead */
            setb(82, 12, drv->wbuffer); /* write buffer */
            setb(82, 13, drv->rbuffer); /* read buffer */
            setb(83, 12, drv->flush); /* flush cache */
            setb(83, 14, 1);
            setb(84, 14, 1);
            setb(85, 3, drv->pmcommands); /* pm command set */
            setb(85, 4, drv->atapi); /* packet */
            setb(85, 5, drv->wcache);
            setb(85, 6, drv->lookahead);
            setb(85, 12, drv->wbuffer); /* write buffer */
            setb(85, 13, drv->rbuffer); /* read buffer */
            setb(86, 12, drv->flush); /* flush cache */
            setb(87, 14, 1);
            putw(255, 0xa5);
            for (i = 0; i < 511; i++) {
                result[511] -= result[i];
            }

            drv->sector_count_internal = 1;
            memcpy(drv->buffer + drv->sector_size - sizeof(result), result, sizeof(result));
            drv->bufp = drv->sector_size - sizeof(result);
            drv->cmd = 0xec;
            return;
        case 0xef:
            switch (drv->features) {
                case 0x02:
                    debug((drv->log, "SET ENABLE WRITE CACHE"));
                    drv->wcache = 1;
                    return;
                case 0x03:
                    debug((drv->log, "SET TRANSFER MODE %02x", drv->sector_count));
                    if (drv->sector_count > 1 && drv->sector_count != 8) {
                        drv->error = ATA_ABRT;
                    }
                    return;
                case 0x33:
                    debug((drv->log, "SET DISABLE RETRY"));
                    return;
                case 0x55:
                    debug((drv->log, "SET DISABLE LOOK-AHEAD"));
                    drv->lookahead = 0;
                    return;
                case 0x82:
                    debug((drv->log, "SET DISABLE WRITE CACHE"));
                    drv->wcache = 0;
                    if (drv->file) {
                        fflush(drv->file);
                    }
                    return;
                case 0x99:
                    debug((drv->log, "SET ENABLE RETRY"));
                    return;
                case 0xaa:
                    debug((drv->log, "SET ENABLE LOOK-AHEAD"));
                    drv->lookahead = 1;
                    return;
                default:
                    debug((drv->log, "SET FEATURES %02x", drv->features));
                    drv->error = ATA_ABRT;
            }
            return;
    }
    debug((drv->log, "COMMAND %02x", value));
    drv->error = ATA_ABRT;
    return;
}

static void atapi_execute_command(ata_drive_t *drv, BYTE value)
{
    BYTE result[512];
    int i;

    if (drv->cmd == 0xe6 && value != 0x08) {
        return;
    }
    drv->bufp = drv->sector_size;
    drv->error = 0;
    drv->cmd = 0x00;
    switch (value) {
        case 0x00:
            ata_execute_command(drv, value);
            return;
        case 0x08:
            debug((drv->log, "DEVICE RESET"));
            drive_diag(drv);
            return;
        case 0x20:
            drv->cylinder = 0xeb14;
            break;
        case 0x90:
            ata_execute_command(drv, value);
            return;
        case 0xa0:
            drv->sector_count_internal = 1;
            drv->bufp = drv->sector_size - sizeof(drv->packet);
            drv->cmd = 0xa0;
            return;
        case 0xa1:
            memset(result, 0, sizeof(result));
            debug((drv->log, "IDENTIFY PACKET DEVICE"));
            putw(0, (drv->type == ATA_DRIVE_FDD) ? 0x8180 : 0x8580);
            ident_update_string(result + 20, ATA_SERIAL_NUMBER, 20);
            putw(21, BUFSIZ / drv->sector_size);
            ident_update_string(result + 46, ATA_REVISION, 8);
            if (drv->type == ATA_DRIVE_FDD) {
                ident_update_string(result + 54, "ATA-FDD " ATA_COPYRIGHT, 40);
            } else {
                ident_update_string(result + 54, "ATA-DVD " ATA_COPYRIGHT, 40);
            }
            setb(49, 9, drv->lbamode); /* LBA support */
            setb(82, 3, drv->pmcommands); /* pm command set */
            setb(82, 4, drv->atapi); /* packet */
            setb(82, 5, 1); /* write cache */
            setb(82, 6, 1); /* look-ahead */
            setb(82, 9, 1); /* device reset */
            setb(83, 12, drv->flush); /* flush cache */
            setb(83, 14, 1);
            setb(84, 14, 1);
            setb(85, 3, drv->pmcommands); /* pm command set */
            setb(85, 4, drv->atapi); /* packet */
            setb(85, 5, drv->wcache);
            setb(85, 6, drv->lookahead);
            setb(86, 12, drv->flush); /* flush cache */
            setb(87, 14, 1);
            putw(255, 0xa5);
            for (i = 0; i < 511; i++) {
                result[511] -= result[i];
            }

            drv->sector_count_internal = 1;
            memcpy(drv->buffer + drv->sector_size - sizeof(result), result, sizeof(result));
            drv->bufp = drv->sector_size - sizeof(result);
            drv->cmd = 0xa1;
            return;
        case 0xe0:
        case 0xe1:
        case 0xe2:
        case 0xe3:
        case 0xe5:
        case 0xe6:
        case 0xe7:
            ata_execute_command(drv, value);
            return;
        case 0xec:
            drive_diag(drv);
            break;
        case 0xef:
            ata_execute_command(drv, value);
            return;
    }
    debug((drv->log, "COMMAND %02x", value));
    drv->error = ATA_ABRT;
    return;
}

static void atapi_packet_execute_command(ata_drive_t *drv)
{
    BYTE result[12];
    int len;

    drv->bufp = drv->sector_size;
    drv->error = 0x00;
    drv->cmd = 0x00;
    if (drv->attention) {
        drv->attention = 0;
        drv->error = 0x64;
        return;
    }
    switch (drv->packet[0]) {
        case 0x00:
            debug((drv->log, "TEST UNIT READY"));
            return;
        case 0x1b:
            debug((drv->log, "START/STOP UNIT (%d)", drv->packet[4] & 3));
            switch (drv->packet[4] & 3) {
                case 0:
                    ata_change_power_mode(drv, 0x00);
                    break;
                case 1:
                    ata_change_power_mode(drv, 0xff);
                    break;
                case 2:
                    if (drv->file) {
                        if (drv->locked) {
                            drv->error = 0x24;
                        } else {
                            ata_change_power_mode(drv, 0x00);
                            ata_image_detach(drv);
                        }
                    }
                    break;
                case 3:
                    if (!drv->file) {
                        ata_image_attach(drv, drv->filename, drv->type, drv->geometry);
                        if (!drv->file) {
                            drv->error = 0x24;
                        } else {
                            ata_change_power_mode(drv, 0xff);
                        }
                    }
                    break;
            }
            return;
        case 0x1e:
            debug((drv->log, "PREVENT/ALLOW MEDIUM REMOVAL (%d)", drv->packet[4] & 1));
            drv->locked = drv->packet[4] & 1;
            return;
        case 0x23:
            memset(result, 0, sizeof(result));
            debug((drv->log, "READ FORMAT CAPACITIES"));
            result[3] = 8;
            result[4] = drv->geometry.size >> 24;
            result[5] = drv->geometry.size >> 16;
            result[6] = drv->geometry.size >> 8;
            result[7] = drv->geometry.size;
            result[8] = drv->file ? 2 : 3;
            result[10] = drv->sector_size >> 8;
            result[11] = drv->sector_size;

            len = (drv->packet[8] < sizeof(result) && !drv->packet[7]) ? ((drv->packet[8] + 1) & 0xfe) : sizeof(result);
            drv->sector_count_internal = 1;
            memcpy(drv->buffer + drv->sector_size - len, result, len);
            if (len) {
                drv->bufp = drv->sector_size - len;
                drv->cmd = 0x23;
            }
            return;
        case 0x28:
            debug((drv->log, "READ 10 (%d)*%d", (drv->packet[2] << 24) | (drv->packet[3] << 16) | (drv->packet[4] << 8) | drv->packet[5], drv->packet[8]));
            drv->sector_count_internal = drv->packet[8];
            if (seek_sector(drv)) {
                return;
            }
            drv->cmd = 0x28;
            read_sector(drv);
            return;
        case 0x2a:
            debug((drv->log, "WRITE 10 (%d)*%d", (drv->packet[2] << 24) | (drv->packet[3] << 16) | (drv->packet[4] << 8) | drv->packet[5], drv->packet[8]));
            drv->sector_count_internal = drv->packet[8];
            if (seek_sector(drv)) {
                return;
            }
            if (drv->readonly) {
                drv->error = 0x54;
                return;
            }
            drv->bufp = 0;
            drv->cmd = 0x2a;
            return;
        case 0xbb:
            debug((drv->log, "SET CD SPEED %d/%d", drv->packet[2] | (drv->packet[3] << 8), drv->packet[4] | (drv->packet[5] << 8)));
            if (drv->type != ATA_DRIVE_CD) {
                drv->error = 0xB4;
            }
            return;
    }
    debug((drv->log, "PACKET COMMAND %02x", drv->packet[0]));
    drv->error = 0xB4;
    return;
}

WORD ata_register_read(ata_drive_t *drv, WORD addr, WORD bus)
{
    WORD res;

    if (drv->type == ATA_DRIVE_NONE) {
        return bus;
    }
    if (drv->dev != drv->slave) {
        return bus;
    }
    if (drv->cmd == 0xe6) {
        return bus;
    }
    if (drv->busy && addr > 0 && addr < 7) {
        addr = 14;
    }
    switch (addr) {
        case 0:
            if (drv->busy || drv->bufp >= drv->sector_size) {
                return bus;
            }
            switch (drv->cmd) {
                case 0x20:
                case 0x23:
                case 0x28:
                case 0xec:
                case 0xe4:
                case 0xa1:
                    res = drv->buffer[drv->bufp] | (drv->buffer[drv->bufp | 1] << 8);
                    drv->bufp += 2;
                    if (drv->bufp >= drv->sector_size) {
                        drv->sector_count_internal--;
                        if (!drv->sector_count_internal) {
                            drv->cmd = 0x00;
                        }
                        switch (drv->cmd) {
                            case 0x20:
                            case 0x28:
                                read_sector(drv);
                                break;
                            default:
                                drv->bufp = drv->sector_size;
                                drv->cmd = 0x00;
                                break;
                        }
                    }
                    return res;
            }
            return bus;
        case 1:
            return (bus & 0xff00) | drv->error;
        case 2:
            if (drv->atapi) {
                switch (drv->cmd) {
                    case 0xa0:
                    case 0x08:
                        res = 0x01;
                        break;
                    case 0xa1:
                    case 0x28:
                    case 0x23:
                        res = 0x02;
                        break;
                    case 0x2a:
                        res = 0x00;
                        break;
                    case 0xe5:
                        res = drv->sector_count;
                        break;
                    default:
                        res = 0x03;
                        break;
                }
            } else {
                res = drv->sector_count;
            }
            return (bus & 0xff00) | res;
        case 3:
            return (bus & 0xff00) | drv->sector;
        case 4:
            return (bus & 0xff00) | (drv->cylinder & 0xff);
        case 5:
            return (bus & 0xff00) | (drv->cylinder >> 8);
        case 6:
            return (bus & 0xff00) | drv->head | (drv->dev << 4) | (drv->lba << 6) | drv->legacy;
        case 7:
        case 14:
            return (bus & 0xff00) | (drv->busy ? 0x80 : 0) | ((drv->atapi && drv->cmd == 0x08) ? 0 : ATA_DRDY) | ((drv->bufp < drv->sector_size) ? ATA_DRQ : 0) | ((drv->error & 0xfe) ? ATA_ERR : 0);
        case 15:
            if (drv->busy & 0x04) {
                return bus & 0xff80;
            }
            return (bus & 0xff80) | (0x7f ^ (drv->head << 2) ^ (1 << drv->dev));
        default:
            return bus;
    }
}

WORD ata_register_peek(ata_drive_t *drv, WORD addr)
{
    if (addr == 0) {
        return 0;
    }
    if (addr == 7) {
        addr = 14;
    }
    return ata_register_read(drv, addr, 0);
}


void ata_register_store(ata_drive_t *drv, WORD addr, WORD value)
{
    if (drv->type == ATA_DRIVE_NONE) {
        return;
    }
    if (addr != 0 && addr != 14 && !(addr == 7 && drv->atapi && (BYTE)value == 0x08)) {
        if (drv->busy || drv->bufp < drv->sector_size) {
            return;
        }
    }
    if (drv->cmd == 0xe6 && addr != 14 && !(addr == 6 && drv->atapi) && !(addr == 7 && drv->atapi && (BYTE)value == 0x08)) {
        return;
    }
    switch (addr) {
        case 0:
            if (drv->busy || drv->bufp >= drv->sector_size) {
                return;
            }
            switch (drv->cmd) {
                case 0x30:
                case 0x2a:
                case 0xe8:
                case 0xa0:
                    drv->buffer[drv->bufp] = value & 0xff;
                    drv->buffer[drv->bufp | 1] = value >> 8;
                    drv->bufp += 2;
                    if (drv->bufp >= drv->sector_size) {
                        switch (drv->cmd) {
                            case 0x2a:
                            case 0x30:
                                if (write_sector(drv)) {
                                    return;
                                }
                                if (--drv->sector_count_internal) {
                                    drv->bufp = 0;
                                    return;
                                }
                                if (!drv->file || fflush(drv->file)) {
                                    drv->error = drv->atapi ? 0x54 : (ATA_UNC | ATA_ABRT);
                                    break;
                                }
                                break;
                            case 0xe8:
                                break;
                            case 0xa0:
                                memcpy(drv->packet, drv->buffer + drv->sector_size - sizeof(drv->packet), sizeof(drv->packet));
                                atapi_packet_execute_command(drv);
                                return;
                        }
                        drv->bufp = drv->sector_size;
                        drv->cmd = 0x00;
                    }
                    break;
            }
            return;
        case 1:
            drv->features = (BYTE)value;
            return;
        case 2:
            drv->sector_count = (BYTE)value;
            return;
        case 3:
            drv->sector = (BYTE)value;
            return;
        case 4:
            drv->cylinder = (drv->cylinder & 0xff00) | (value & 0xff);
            return;
        case 5:
            drv->cylinder = (drv->cylinder & 0xff) | (value << 8);
            return;
        case 6:
            drv->dev = (value >> 4) & 1;
            if (drv->cmd != 0xe6) {
                drv->head = value & 0xf;
                drv->lba = (value >> 6) & 1;
                drv->legacy = value & 0xa0;
            }
            return;
        case 7:
            if (drv->dev == drv->slave || (BYTE)value == 0x90) {
                if (drv->atapi) {
                    atapi_execute_command(drv, (BYTE)value);
                } else {
                    ata_execute_command(drv, (BYTE)value);
                }
            }
            return;
        case 14:
            drv->busy = (drv->busy & ~0x04) | (value & 0x04);
            if ((drv->control & 0x04) && ((value ^ 0x04) & 0x04)) {
                ata_reset(drv);
                debug((drv->log, "SOFTWARE RESET"));
            }
            drv->control = (BYTE)value;
            return;
    }
    return;
}

void ata_image_attach(ata_drive_t *drv, char *filename, ata_drive_type_t type, ata_drive_geometry_t geometry)
{
    if (drv->file != NULL) {
        fclose(drv->file);
        drv->file = NULL;
    }

    if (drv->filename != filename) {
        util_string_set(&drv->filename, filename);
    }
    drv->geometry = geometry;

    if (type != ATA_DRIVE_NONE) {
        if (drv->filename && drv->filename[0]) {
            if (type != ATA_DRIVE_CD) {
                drv->file = fopen(drv->filename, MODE_READ_WRITE);
            }
            if (!drv->file) {
                drv->file = fopen(drv->filename, MODE_READ);
            }
        }

        if (drv->geometry.size < 1) {
            drv->geometry.cylinders = identify[2] | (identify[3] << 8);
            drv->geometry.heads = identify[6];
            drv->geometry.sectors = identify[12];
            drv->geometry.size = drv->geometry.cylinders * drv->geometry.heads * drv->geometry.sectors;
            if ((identify[99] & 0x02) && (identify[120] || identify[121] || identify[122] || identify[123])) {
                drv->geometry.size = identify[120];
                drv->geometry.size |= identify[121] << 8;
                drv->geometry.size |= identify[122] << 16;
                drv->geometry.size |= identify[123] << 24;
                drv->lbamode = 1;
            } else {
                drv->lbamode = 0;
            }
            log_warning(drv->log, "Image size invalid, using default %d MiB.", drv->geometry.size / (1048576 / drv->sector_size));
        }
    }

    if (!drv->atapi || drv->type != type) {
        ata_poweron(drv, type); /* update actual geometry */
    } else {
        drv->attention = 1; /* disk change only */
    }

    if (drv->file) {
        if (drv->atapi) {
            log_message(drv->log, "Attached `%s' %u sectors total.", drv->filename, drv->geometry.size);
        } else {
            log_message(drv->log, "Attached `%s' %i/%i/%i CHS geometry, %u sectors total.", drv->filename, drv->geometry.cylinders, drv->geometry.heads, drv->geometry.sectors, drv->geometry.size);
        }
    } else {
        if (drv->filename && drv->filename[0] && drv->type != ATA_DRIVE_NONE) {
            log_warning(drv->log, "Cannot use image file `%s', drive disabled.", drv->filename);
        }
    }
    return;
}

void ata_image_detach(ata_drive_t *drv)
{
    if (drv->file != NULL) {
        fclose(drv->file);
        drv->file = NULL;
        log_message(drv->log, "Detached.");
    }
    return;
}

int ata_image_change(ata_drive_t *drv, char *filename, ata_drive_type_t type, ata_drive_geometry_t geometry)
{
    if (drv->type != type || drv->locked) {
        return 1;
    }
    ata_image_attach(drv, filename, type, geometry);
    return 0;
}

int ata_register_dump(ata_drive_t *drv)
{
    if (drv->dev != drv->slave || drv->type == ATA_DRIVE_NONE) {
        return -1;
    }
    mon_out("%s device %s\n", drv->atapi ? "ATAPI" : "ATA", drv->myname);
    mon_out("Error:        %02x\n", ata_register_peek(drv, 1));
    mon_out("Sector count: %02x\n", ata_register_peek(drv, 2));
    mon_out("LBA low:      %02x\n", ata_register_peek(drv, 3));
    mon_out("LBA mid:      %02x\n", ata_register_peek(drv, 4));
    mon_out("LBA high:     %02x\n", ata_register_peek(drv, 5));
    mon_out("Device:       %02x\n", ata_register_peek(drv, 6));
    mon_out("Status:       %02x\n", ata_register_peek(drv, 7));

    return 0;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

/* Please note that after loading a snapshot the the image is in readonly
 * mode to prevent any filesystem corruption. This could be solved later by
 * checksumming the image (this might be slow!) and comparing it to the stored
 * checksum to check if there was any modification meanwhile.
 */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   7

int ata_snapshot_write_module(ata_drive_t *drv, snapshot_t *s)
{
    snapshot_module_t *m;
    DWORD spindle_clk = CLOCK_MAX;
    DWORD head_clk = CLOCK_MAX;
    DWORD standby_clk = CLOCK_MAX;
    off_t pos = 0;

    m = snapshot_module_create(s, drv->myname,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (drv->busy & 1) {
        spindle_clk = drv->spindle_alarm->context->pending_alarms[drv->spindle_alarm->pending_idx].clk;
    }
    if (drv->busy & 2) {
        head_clk = drv->head_alarm->context->pending_alarms[drv->head_alarm->pending_idx].clk;
    }
    if (drv->standby) {
        standby_clk = drv->standby_alarm->context->pending_alarms[drv->standby_alarm->pending_idx].clk;
    }
    if (drv->file) {
        pos = ftello(drv->file);
        if (pos < 0) {
            pos = 0;
        }
    }

    SMW_STR(m, drv->filename);
    SMW_DW(m, drv->type);
    SMW_W(m, (WORD)drv->geometry.cylinders);
    SMW_B(m, (BYTE)drv->geometry.heads);
    SMW_B(m, (BYTE)drv->geometry.sectors);
    SMW_DW(m, drv->geometry.size);
    SMW_B(m, drv->error);
    SMW_B(m, drv->features);
    SMW_B(m, drv->sector_count);
    SMW_B(m, drv->sector_count_internal);
    SMW_B(m, drv->sector);
    SMW_W(m, drv->cylinder);
    SMW_B(m, (BYTE)(drv->head | (drv->dev << 4) | (drv->lba << 6) | drv->legacy));
    SMW_B(m, drv->control);
    SMW_B(m, drv->cmd);
    SMW_B(m, drv->power);
    SMW_BA(m, drv->packet, sizeof(drv->packet));
    SMW_W(m, (WORD)drv->bufp);
    SMW_BA(m, drv->buffer, drv->sector_size);
    SMW_W(m, (WORD)drv->cylinders);
    SMW_B(m, (BYTE)drv->heads);
    SMW_B(m, (BYTE)drv->sectors);
    SMW_DW(m, drv->pos);
    SMW_DW(m, pos / drv->sector_size);
    SMW_B(m, (BYTE)drv->wcache);
    SMW_B(m, (BYTE)drv->lookahead);
    SMW_B(m, (BYTE)drv->busy);
    SMW_DW(m, spindle_clk);
    SMW_DW(m, head_clk);
    SMW_DW(m, standby_clk);
    SMW_DW(m, drv->standby);
    SMW_DW(m, drv->standby_max);

    return snapshot_module_close(m);
}

int ata_snapshot_read_module(ata_drive_t *drv, snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    char *filename = NULL;
    DWORD spindle_clk;
    DWORD head_clk;
    DWORD standby_clk;
    int pos, type;

    m = snapshot_module_open(s, drv->myname, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_STR(m, &filename);
    if (!drv->filename || strcmp(filename, drv->filename)) {
        lib_free(filename);
        snapshot_module_close(m);
        return -1;
    }
    lib_free(filename);
    SMR_DW_INT(m, &type);
    drv->type = type;
    if (drv->type != ATA_DRIVE_HDD && drv->type != ATA_DRIVE_FDD && drv->type != ATA_DRIVE_CD) {
        drv->type = ATA_DRIVE_NONE;
    }
    SMR_W_INT(m, &drv->geometry.cylinders);
    if (drv->geometry.cylinders < 1 || drv->geometry.cylinders > 16) {
        drv->geometry.cylinders = 1;
    }
    SMR_B_INT(m, &drv->geometry.heads);
    if (drv->geometry.heads < 1 || drv->geometry.heads > 16) {
        drv->geometry.heads = 1;
    }
    SMR_B_INT(m, &drv->geometry.sectors);
    if (drv->geometry.sectors < 1 || drv->geometry.sectors > 16) {
        drv->geometry.sectors = 1;
    }
    SMR_DW_INT(m, &drv->geometry.size);
    if (drv->geometry.size < 1 || drv->geometry.size > 268435455) {
        drv->geometry.size = 1;
    }
    ata_image_attach(drv, drv->filename, drv->type, drv->geometry);
    SMR_B(m, &drv->error);
    SMR_B(m, &drv->features);
    SMR_B(m, &drv->sector_count);
    SMR_B(m, &drv->sector_count_internal);
    SMR_B(m, &drv->sector);
    SMR_W(m, &drv->cylinder);
    SMR_B(m, &drv->head);
    drv->dev = (drv->head >> 4) & 1;
    drv->lba = (drv->head >> 6) & 1;
    drv->legacy = drv->head & 0xa0;
    drv->head &= 0xf;
    SMR_B(m, &drv->control);
    SMR_B(m, &drv->cmd);
    SMR_B(m, &drv->power);
    SMR_BA(m, drv->packet, sizeof(drv->packet));
    if (drv->power != 0 && drv->power != 0x80) {
        drv->power = 0xff;
    }
    SMR_W_INT(m, &drv->bufp);
    if (drv->bufp < 0 || drv->bufp > drv->sector_size) {
        drv->bufp = drv->sector_size;
    }
    SMR_BA(m, drv->buffer, drv->sector_size);
    SMR_W_INT(m, &drv->cylinders);
    if (drv->cylinders < 1 || drv->cylinders > 65535) {
        drv->cylinders = 1;
    }
    SMR_B_INT(m, &drv->heads);
    if (drv->heads < 1 || drv->heads > 16) {
        drv->heads = 1;
    }
    SMR_B_INT(m, &drv->sectors);
    if (drv->sectors < 1 || drv->sectors > 63) {
        drv->sectors = 1;
    }
    SMR_DW_INT(m, &drv->pos);
    if (drv->pos < 0 || drv->pos > 268435455) {
        drv->pos = 0;
    }
    SMR_DW_INT(m, &pos);
    SMR_B_INT(m, &drv->wcache);
    if (drv->wcache) {
        drv->wcache = 1;
    }
    SMR_B_INT(m, &drv->lookahead);
    if (drv->lookahead) {
        drv->lookahead = 1;
    }
    SMR_B_INT(m, &drv->busy);
    SMR_DW(m, &spindle_clk);
    SMR_DW(m, &head_clk);
    SMR_DW(m, &standby_clk);
    SMR_DW_INT(m, &drv->standby);
    SMR_DW_INT(m, &drv->standby_max);
    drv->busy &= 0x03;
    if (drv->busy & 1) {
        alarm_set(drv->spindle_alarm, spindle_clk);
    } else {
        alarm_unset(drv->spindle_alarm);
    }
    if (drv->busy & 2) {
        alarm_set(drv->head_alarm, head_clk);
    } else {
        alarm_unset(drv->head_alarm);
    }
    if (drv->standby) {
        alarm_set(drv->standby_alarm, standby_clk);
    } else {
        alarm_unset(drv->standby_alarm);
    }

    if (drv->file) {
        fseeko(drv->file, (off_t)pos * drv->sector_size, SEEK_SET);
    }
    if (!drv->atapi) { /* atapi supports disc change events */
        drv->readonly = 1; /* make sure for ata that there's no filesystem corruption */
    }

    return snapshot_module_close(m);
}
