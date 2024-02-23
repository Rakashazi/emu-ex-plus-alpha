/*
 * cmdhd.c - Whole CMDHD emulation
 *
 * Written by
 * Roberto Muscedere (rmusced@uwindsor.ca)
 *
 * Based on old code by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <stdint.h>

#include "archdep.h"
#include "diskimage.h"
#include "debug.h"
#include "drive.h"
#include "drivesync.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "interrupt.h"
#include "lib.h"
#include "types.h"
#include "cmdhd.h"
#include "util.h"
#include "diskimage/fsimage.h"
#include "rtc/rtc-72421.h"
#include "resources.h"

#define LOG LOG_DEFAULT
#define ERR LOG_ERR

/* #define CMDLOG */
/* #define CMDIO */
/* #define CMDBUS */

#ifdef CMDLOG
#define CLOG(_x_) log_message _x_
#else
#define CLOG(_x_)
#endif

#ifdef CMDIO
#define IDBG(_x_) log_message _x_
#else
#define IDBG(_x_)
#endif

#ifdef CMDBUS
#define BLOG(_x_) log_message _x_
#else
#define BLOG(_x_)
#endif

#define CRIT(_x_) log_message _x_

#define iecbus (viap->v_iecbus)

cmdbus_t cmdbus;

typedef struct drivevia_context_s {
    unsigned int number;
    struct drive_s *drive;
    struct iecbus_s *v_iecbus;
} drivevia_context_t;

/* Although SCSI can be a multi-master bus, CMD HD's assume they are the
masters so we won't implement all the possible varitions, just a "target"
device that will respond to all (0-6) IDs and 0-7 LUNs for each */
/* Latest boot rom (2.8) and even earlier have a bug when deleting partitions
across multiple drives */
/* Although we can support up to 55 (tested), deleting partitions corrupts
the data so we will not allow this, at least for now */
/* SCSI states are setup to reflect the register values coming out of U13 to
simplify the interface */
/* All signals asserted high here, opposite in physical implementation
   (open-collector/drain etc)

  SEL  BSY  IO  MSG  CD  U9-PB2-0  PHASE
  0    0    X   X    X   X         BUSFREE
  0    1    X   X    X   X         ARBITRATION
  1    0    X   X    X   X         SELECTION (phase 1)
  1    1    X   X    X   X         SELECTION (phase 2)
  0    1    0   0    0   0         DATA-OUT
  0    1    0   0    1   1         COMMAND
  0    1    1   0    0   4         DATA-IN
  0    1    1   0    1   5         STATUS
  0    1    0   1    1   3         MESSAGE-OUT
  0    1    1   1    1   7         MESSAGE-IN

*/

#define CMD_STATE_DATAOUT    0x00
#define CMD_STATE_COMMAND    0x01
#define CMD_STATE_DATAIN     0x04
#define CMD_STATE_STATUS     0x05
#define CMD_STATE_MESSAGEOUT 0x03
#define CMD_STATE_MESSAGEIN  0x07

# undef DEBUG_IEC_DRV_WRITE
# undef DEBUG_IEC_DRV_READ

#ifdef DEBUG

# define DEBUG_IEC_DRV_WRITE(_data) my_debug_iec_drv_write(_data)
# define DEBUG_IEC_DRV_READ(_data) my_debug_iec_drv_read(_data)

#else

# define DEBUG_IEC_DRV_WRITE(_data)
# define DEBUG_IEC_DRV_READ(_data)

#endif

#ifdef DEBUG

#include "log.h"

static void my_debug_iec_drv_write(unsigned int data)
{
    if (debug.iec) {
        uint8_t value = data;
        static uint8_t oldvalue = 0;

        if (value != oldvalue) {
            oldvalue = value;

            log_debug("$1800 store: %s %s %s",
                      value & 0x02 ? "DATA OUT" : "        ",
                      value & 0x08 ? "CLK OUT" : "       ",
                      value & 0x10 ? "ATNA   " : "       "
                      );
        }
    }
}

static void my_debug_iec_drv_read(unsigned int data)
{
    if (debug.iec) {
        uint8_t value = data;
        static uint8_t oldvalue = { 0 };
        const char * data_correct = "";

        if (value != oldvalue) {
            unsigned int atn = value & 0x80 ? 1 : 0;
            unsigned int atna = value & 0x10 ? 1 : 0;
            unsigned int ddata = value & 0x01 ? 1 : 0;

            oldvalue = value;

            if (atn && atna) {
                if (!ddata) {
                    data_correct = " ***** ERROR: ATN, ATNA & DATA! *****";
                }
            }

            log_debug("$1800 read:  %s %s %s %s %s %s%s",
                      value & 0x02 ? "DATA OUT" : "        ",
                      value & 0x08 ? "CLK OUT" : "       ",
                      value & 0x10 ? "ATNA   " : "       ",

                      value & 0x01 ? "DATA IN" : "       ",
                      value & 0x04 ? "CLK IN" : "       ",
                      value & 0x80 ? "ATN" : "   ",
                      data_correct
                      );
        }
    }
}
#endif

/* copy some functions here so we don't make them external */
static uint8_t drive_read_rom(diskunit_context_t *drv, uint16_t address)
{
    return drv->rom[address & 0x7fff];
}

static uint8_t drive_read_ram(diskunit_context_t *drv, uint16_t address)
{
    return drv->drive_ram[address];
}

static void drive_store_ram(diskunit_context_t *drv, uint16_t address, uint8_t value)
{
    drv->drive_ram[address] = value;
}

static void reset_alarm_handler(CLOCK offset, void *data)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)data;

    CLOG((LOG, "CMDHD: alarm triggered at %lu; releasing buttons",
        *(hd->mycontext->clk_ptr)));
    /* stop pressing WP, SWAP8, and SWAP9 buttons */
    hd->i8255a_i[1] |= (0x08 | 0x04 | 0x02);
    /* update in drive context too */
    hd->mycontext->button = 0;

    alarm_unset(hd->reset_alarm);
}

/* returns 0 if block has CMCHD signature at end */
static int cmdhd_has_sig(unsigned char *buf)
{
    unsigned char hdmagic[16]={0x43, 0x4d, 0x44, 0x20, 0x48, 0x44, 0x20, 0x20,
        0x8d, 0x03, 0x88, 0x8e, 0x02, 0x88, 0xea, 0x60};
    return memcmp(&(buf[0xf0]), hdmagic, 16);
}

static void cmdhd_scsiread(struct scsi_context_s *scsi)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(scsi->p);
    drive_t *dc = (drive_t *)(hd->mycontext->drives[0]);
    int unit = hd->mycontext->mynumber + 8;
    int track;

    /* update track info on status bar; never 100 or above */
    track = (scsi->address * 200) / (hd->imagesize + 1);
    if (track >= 200) {
        track = 199;
    }
    dc->current_half_track = track;

    /* leave if we are not the first disk */
    if (scsi->target !=0 || scsi->lun != 0 ) {
        return;
    }

    /* correct device number on the fly since it is stored on HD not by switch or EEPROM */
    if (hd->baselba != UINT32_MAX && scsi->address == hd->baselba + 2) {
        /* make sure it has the CMD signature first */
        if (!cmdhd_has_sig(&(scsi->data_buf[256]))) {
            /* if it isn't what was defined by vice, set it to it */
            if ( (scsi->data_buf[0x1e1] != unit) ||
                (scsi->data_buf[0x1e4] != unit) ) {
                /* tell the user it is happening */
                CLOG((LOG, "CMDHD: drive number is now %d; was %d in config block",
                    unit, (int)scsi->data_buf[0x1e1]));
                scsi->data_buf[0x1e1] = unit;
                scsi->data_buf[0x1e4] = unit;
            }
        } else {
            /* block we had on record no longer has signature, invalidate it */
            hd->baselba = UINT32_MAX;
        }
    }
}

static void cmdhd_scsiwrite(struct scsi_context_s *scsi)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(scsi->p);
    drive_t *dc = (drive_t *)(hd->mycontext->drives[0]);
    uint32_t temp;

    /* keep track of the maximum lba written to */
    temp = scsi->address + 1;
    if (temp > hd->imagesize) {
        hd->imagesize = temp;
    }

    /* update track info on status bar */
    dc->current_half_track = (scsi->address * 200) / (hd->imagesize + 1);
}

/* We don't actually format the disk, we just remove the 16 byte CMD signature */
static void cmdhd_scsiformat(struct scsi_context_s *scsi)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(scsi->p);
    int i;

    /* leave if we are not the first disk */
    if (scsi->target !=0 || scsi->lun != 0 ) {
        return;
    }

    /* correct device number on the fly since it is stored on HD not by switch or EEPROM */
    /* figure out where to start looking */
    if (hd->baselba != UINT32_MAX) {
        /* use what we already have if we found it before */
        if (hd->baselba < hd->imagesize) {
            scsi->address = hd->baselba + 2;
        } else {
        /* other wise, start from scratch */
            hd->baselba = UINT32_MAX;
            scsi->address = 2;
        }
    } else {
        scsi->address = 2;
    }
    /* start searching, every 128 LBAs starting from 2 or known base */
    while (scsi->address < hd->imagesize) {
        /* stop if we hit the end of the file */
        if (scsi_image_read(scsi) < 0) {
            break;
        }
        /* check for the CMD sig */
        if (!cmdhd_has_sig(&(scsi->data_buf[256]))) {
            hd->baselba = scsi->address - 2;
            /* we found it, zero it out */
            for (i = 0; i < 16; i++) {
                scsi->data_buf[0x1f0 + i] = 0;
            }
            /* write it back */
            scsi_image_write(scsi);
            break;
        }
        /* otherwise, keep looking */
        scsi->address += 128;
    }
}

/* Set all the inputs high, like the pull-up resistors do */
void cmdbus_init(void)
{
    int i;

    cmdbus.cpu_data = 0xff;
    cmdbus.cpu_bus = 0xff;
    cmdbus.data = 0xff;
    cmdbus.bus = 0xff;
    for (i = 0; i < NUM_DISK_UNITS; i++) {
        cmdbus.drv_data[i] = 0xff;
        cmdbus.drv_bus[i] = 0xff;
    }
}

/* Calculate the data/bus values */
void cmdbus_update(void)
{
    int i;

    /* only allow devices to impact the bus if bit 0 is set on bus part */
    if (cmdbus.cpu_bus & 1) {
        cmdbus.bus = cmdbus.cpu_bus;
        cmdbus.data = cmdbus.cpu_data;
    } else {
        cmdbus.bus = 0xff;
        cmdbus.data = 0xff;
    }
    for (i = 0; i < NUM_DISK_UNITS; i++) {
        /* only allow devices to impact the bus if bit 0 is set on bus part */
        /* and if the parallel cable is not set to none */
        if ((cmdbus.drv_bus[i] & 1) && (diskunit_context[i]->parallel_cable)) {
            cmdbus.bus &= cmdbus.drv_bus[i];
            cmdbus.data &= cmdbus.drv_data[i];
        }
    }

    BLOG((LOG, "CMDBUS PREADY=%s PCLK=%s PATN=%s", cmdbus.bus & 0x80 ? "LOW " : "HIGH",
        cmdbus.bus & 0x40 ? "LOW " : "HIGH", cmdbus.bus & 0x20 ? "LOW " : "HIGH"));
}

/* U11 or i8255a interfacing */
/* Port A is for CMD Parallel bus, input/output, data only */
static void set_pa(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(ctx->p);

    cmdbus.drv_data[hd->mycontext->mynumber] = byte;
    cmdbus_update();
}

static uint8_t get_pa(struct _i8255a_state *ctx, int8_t reg)
{
    uint8_t data = 0xff;

    if (reg == 0) {
        /* if reg is 0, it is an actual read from the register */
        data = cmdbus.data;
    } else {
        /* otherwise it is a bus change; physically it is pulled up */
        data = 0xff;
    }

    return data;
}

/* Port B is for CMD Parallel bus the buttons (input only):
   PB7 is PATN
   PB6 is PCLK
   PB5 is not referenced
   PB4 is not referenced
   PB3 is WP (active low)
   PB2 is SWAP9 (active low)
   PB1 is SWAP8 (active low)
   PB0 is PREADY
*/
static void set_pb(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(ctx->p);

    hd->i8255a_o[1] = byte;
}

static uint8_t get_pb(struct _i8255a_state *ctx, int8_t reg)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(ctx->p);
    uint8_t data = 0xff;

    data = ((~cmdbus.bus << 2) & 0x80) | /* PATN */
           (~cmdbus.bus & 0x40) |        /* PCLK */
           ((~cmdbus.bus >> 7) & 0x01) | /* PREADY */
           (hd->i8255a_i[1] & 0x3e);     /* Everything else */

    return data;
}

/* Called to deal with how PATN changes PREADY */
/* "new" and "old", 0 or 1, are PATN, not /PATN */
/* CMDHD's have a circuit which drives /PREADY */
/* It has a FF where PC7 controls /CLR and PATN is the CLK */
/* The output is NANDed with PATN which drives /PREADY */
/* /PREADY is also connected to a buffer (OC) driven by PC7 */
static int cmdhd_patn_changed(unsigned int unit, int new, int old)
{
    cmdhd_context_t *hd;
    int t;

    /* standard unit range check */
    if (unit < 8 || unit > 8 + NUM_DISK_UNITS) {
        return -1;
    }

    /* check context */
    if (!diskunit_context[unit - 8]) {
        return -1;
    }

    if (diskunit_context[unit - 8]->type != DRIVE_TYPE_CMDHD) {
        return -1;
    }

    /* get context */
    hd = diskunit_context[unit - 8]->cmdhd;

    /* leave if no HD contxt provided */
    if (!hd) {
        return -1;
    }

    /* The drive type is a CMDHD by this point */

    if ((hd->i8255a_o[2] & 0x80) == 0) {
        /* when PC7 is 0, PREADYFF becomes 0 */
        hd->preadyff = 0;
    } else if (old == 0 && new == 1) {
        /* on rising edge of PATN, PREADYFF = 1 */
        hd->preadyff = 1;
    }
    /* /PREADY = !(NEWATN & PREADYFF) */
    t = !(new & hd->preadyff);
    /* The OC buffer */
    if ((hd->i8255a_o[2] & 0x80) == 0) {
        t = 0;
    }

    cmdbus.drv_bus[unit - 8] = (cmdbus.drv_bus[unit - 8] & 0x7f) | (t << 7);

    return 0;
}

/* Update ALL TDE units when PATN changed */
/* called from ramlink */
void cmdbus_patn_changed(int new, int old)
{
    int unit;

    for (unit = 8; unit < 8 + NUM_DISK_UNITS; unit++ ) {
        cmdhd_patn_changed(unit, new, old);
    }
}

/* Port C is for CMD Parallel bus, SCSI, and memory control (output only):
   PC7 is for used for driving PREADY
   PC6 is /PCLK
   PC5 is /PEXT
   PC4 is SCSI BSY
   PC3 is SCSI RST
   PC2 is SCSI ATN
   PC1 is RAM mapping
   PC0 is ROM control
*/
static void set_pc(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(ctx->p);
    scsi_context_t *scsi = (scsi_context_t*)(hd->scsi);
    int t;
    int mynumber = hd->mycontext->mynumber;

    hd->i8255a_o[2] = byte;
    scsi->atn = ((hd->i8255a_o[2] & 4)!=0);
    scsi->rst = ((hd->i8255a_o[2] & 8)!=0);
    scsi->bsyi = ((hd->i8255a_o[2] & 16)!=0);
    scsi_process_noack(scsi);

    /* get the PATN state (from /PATN) */
    t = (cmdbus.bus) & 0x20 ? 0 : 1;

    /* adjust /PREADY */
    cmdhd_patn_changed(mynumber + 8, t, t);

    /* update bus */
    cmdbus.drv_bus[mynumber] =
        (cmdbus.drv_bus[mynumber] & 0xa0)  | /* old /PREADY and /PATN */
        (hd->i8255a_o[2] & 0x40)           | /* /PCLK */
        ((hd->i8255a_o[2] & 0x20)>>1)      | /* /PEXT */
        0x0f;                                /* everything else */
    cmdbus_update();
}

static uint8_t get_pc(struct _i8255a_state *ctx, int8_t reg)
{
    cmdhd_context_t *hd = (cmdhd_context_t*)(ctx->p);
    uint8_t data = 0xff;

    data = hd->i8255a_i[2];

    return data;
}

static void updateleds(diskunit_context_t *ctxptr)
{
    ctxptr->drives[0]->led_status = (ctxptr->cmdhd->LEDs & 0x02) ? 0 : 2;
    ctxptr->drives[0]->led_status |= (ctxptr->cmdhd->LEDs & 0x01) ? 0 : 1;
}

void cmdhd_store(diskunit_context_t *ctxptr, uint16_t addr, uint8_t data)
{
#ifdef CMDIO
    static uint8_t oldd;
    static uint16_t olda;
    static CLOCK oldc = 0;
    drivecpu_context_t *cpu = ctxptr->cpu;
#define storedebug() \
if (olda != addr || olds != data) { \
    IDBG((LOG, "CMDHD: IO write %02x to %04x PC=%04x CYCLE=%u", data, addr, \
        cpu->cpu_R65C02_regs.pc, *(ctxptr->clk_ptr)-oldc); \
    old=data; \
    olda=addr; \
    oldc=*(ctxptr->clk_ptr); \
}
#else
#define storedebug()
#endif
    ctxptr->cpu->cpu_last_data = data;
    /* Decode bits 15-12 */
    switch ( (addr >> 12) & 15 ) {
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        /* Since the ROM wants to read/write from the RAM from 0xC000-0xFFFF,
            when 0x8802.1 = 0, RAM from 0xC000-0xFFFF maps to 0x4000-0x7FFF */
        if (ctxptr->cmdhd->i8255a_o[2]&2) {
            drive_store_ram(ctxptr, (addr & 0x3fff) | 0x4000, data );
        } else {
            drive_store_ram(ctxptr, (addr & 0x3fff) | 0xC000, data );
        }
        break;
    case 0x8:
        /* Decode bits 11-8 */
        /* Since the kernel is loaded into RAM from the HD on startup, if
            0x8F00.5=0 then the memory (not IO)  above 0x8000 is protected from
            being written to */
        switch ((addr >> 8) & 15) {
        case 0x0: /* 0x80xx U10 */
        case 0x1: /* 0x81xx U10 */
            storedebug()
            viacore_store(ctxptr->cmdhd->via10, addr & 15, data);
            break;
        case 0x4: /* 0x84xx U9 */
        case 0x5: /* 0x85xx U9 */
            storedebug()
            viacore_store(ctxptr->cmdhd->via9, addr & 15, data);
            break;
        case 0x8: /* 0x88xx U11 */
        case 0x9: /* 0x89xx U11 */
            storedebug()
            i8255a_store(ctxptr->cmdhd->i8255a, addr & 3, data);
            break;
        case 0xc: /* 0x8cxx */
        case 0xd: /* 0x8dxx */
            storedebug()
            rtc72421_write(ctxptr->cmdhd->rtc, addr & 15, data);
            break;
        case 0xf: /* 0x8fxx U20 */
            /* Although page 0x8F is RAM, all writes go to U20 */
            /* OS often reads from 0x8f00 to get past values to OR/AND them */
            storedebug()
            ctxptr->cmdhd->LEDs = data;
            drive_store_ram(ctxptr, (addr & 255) | 0x8f00, data);
            updateleds(ctxptr);
            break;
        case 0xe: /* 0x8exx unprotected RAM */
            drive_store_ram(ctxptr, (addr & 255) | 0x8e00, data);
            break;
        default:
            /* Everything else writes to RAM if switch is on */
            if (ctxptr->cmdhd->LEDs&32) {
                drive_store_ram(ctxptr, addr, data);
            }
            break;
        }
        break;
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        if (ctxptr->cmdhd->LEDs&32) {
            drive_store_ram(ctxptr, addr, data);
        }
        break;
    }
    return;
}

uint8_t cmdhd_read(diskunit_context_t *ctxptr, uint16_t addr)
{
#ifdef CMDIO
    static CLOCK oldc = 0;
    drivecpu_context_t *cpu = ctxptr->cpu;
#define readdebug() \
    IDBG((LOG, "CMDHD: IO read %02x from %04x PC=%04x CYCLE=%u", data, addr, \
        cpu->cpu_R65C02_regs.pc, *(ctxptr->clk_ptr)-oldc); \
    oldc=*(ctxptr->clk_ptr);
#else
#define readdebug()
#endif
    uint8_t data;

    /* Decode bits 15-12 */
    switch ( (addr >> 12) & 15 ) {
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        /* Since the ROM wants to read/write from the RAM from 0xC000-0xFFFF,
            when 0x8802.1 = 0, RAM from 0xC000-0xFFFF maps to 0x4000-0x7FFF */
        if (ctxptr->cmdhd->i8255a_o[2]&2) {
            return ctxptr->cpu->cpu_last_data = drive_read_ram(ctxptr, (addr & 0x3fff) | 0x4000 );
        } else {
            return ctxptr->cpu->cpu_last_data = drive_read_ram(ctxptr, (addr & 0x3fff) | 0xC000 );
        }
        break;
    case 0x8:
        /* Decode bits 11-8 */
        switch ((addr >> 8) & 15) {
        case 0x0: /* 0x80xx U10 */
        case 0x1: /* 0x81xx U10 */
            data = viacore_read(ctxptr->cmdhd->via10, addr & 15);
            readdebug()
            return ctxptr->cpu->cpu_last_data = data;
            break;
        case 0x4: /* 0x84xx U9 */
        case 0x5: /* 0x85xx U9 */
            data = viacore_read(ctxptr->cmdhd->via9, addr & 15);
            readdebug()
            return ctxptr->cpu->cpu_last_data = data;
            break;
        case 0x8: /* 0x88xx U11 */
        case 0x9: /* 0x89xx U11 */
            data = i8255a_read(ctxptr->cmdhd->i8255a, addr & 3);
            readdebug()
            return ctxptr->cpu->cpu_last_data = data;
            break;
        case 0xc: /* 0x8cxx */
        case 0xd: /* 0x8dxx */
            data = rtc72421_read(ctxptr->cmdhd->rtc, addr & 15);
            readdebug()
            return ctxptr->cpu->cpu_last_data = data;
            break;
        default:
            return ctxptr->cpu->cpu_last_data = drive_read_ram(ctxptr, addr);
            break;
        }
        break;
    case 0x9:
    case 0xa:
    case 0xb:
        return ctxptr->cpu->cpu_last_data = drive_read_ram(ctxptr, addr);
        break;
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        /* ROM is enabled when 0x8802.0 is 1, else RAM */
        if (ctxptr->cmdhd->i8255a_o[2]&1) {
            return ctxptr->cpu->cpu_last_data = drive_read_rom(ctxptr, addr & 0x3fff );
        } else {
            return ctxptr->cpu->cpu_last_data = drive_read_ram(ctxptr, addr);
        }
        break;
    }
    return 0;
}

uint8_t cmdhd_peek(diskunit_context_t *ctxptr, uint16_t addr)
{
    return 0;
}

int cmdhd_dump(diskunit_context_t *ctxptr, uint16_t addr)
{
    return 0;
}

static void set_ca2(via_context_t *via_context, int state)
{
}

static void set_cb2(via_context_t *via_context, int state, int offset)
{
}

static void set_int(via_context_t *via_context, unsigned int int_num,
                    int value, CLOCK rclk)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    diskunit_context_t *dc = (diskunit_context_t *)(hd->mycontext);

    interrupt_set_irq(dc->cpu->int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num,
                        int value)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    diskunit_context_t *dc = (diskunit_context_t *)(hd->mycontext);

    interrupt_restore_irq(dc->cpu->int_status, int_num, value);
}

static void undump_pra(via_context_t *via_context, uint8_t byte)
{
}

static void undump_prb10(via_context_t *via_context, uint8_t byte)
{
    drivevia_context_t *viap = (drivevia_context_t *)(via_context->prv);

    if (iecbus != NULL) {
        uint8_t *drive_bus, *drive_data;
        unsigned int unit;

        drive_bus = &(iecbus->drv_bus[viap->number + 8]);
        drive_data = &(iecbus->drv_data[viap->number + 8]);

        *drive_data = ~byte;
        *drive_bus = ((((*drive_data) << 3) & 0x40)
                      | (((*drive_data) << 6)
                         & (((*drive_data) | iecbus->cpu_bus) << 3) & 0x80));

        iecbus->cpu_port = iecbus->cpu_bus;
        for (unit = 4; unit < 8 + NUM_DISK_UNITS; unit++) {
            iecbus->cpu_port &= iecbus->drv_bus[unit];
        }

        iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                            | (iecbus->cpu_port >> 7)
                            | ((iecbus->cpu_bus << 3) & 0x80));
    } else {
        iec_drive_write((uint8_t)(~byte), viap->number);
    }
}

static void store_pra9(via_context_t *via_context, uint8_t byte, uint8_t oldpa,
                      uint16_t addr)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    scsi_context_t *scsi = (scsi_context_t*)(hd->scsi);
    scsi_set_bus(scsi, byte);

    if (scsi->state!=SCSI_STATE_BUSFREE && ((addr & 0xf) == VIA_PRA) ) {
        scsi_process_ack(scsi);
    } else {
        scsi_process_noack(scsi);
    }
}

static uint8_t read_pra9(via_context_t *via_context, uint16_t addr)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    scsi_context_t *scsi = (scsi_context_t*)(hd->scsi);
    uint8_t byte;

    byte = scsi_get_bus(scsi);
    if (scsi->state!=SCSI_STATE_BUSFREE && ((addr & 0xf) == VIA_PRA) ) {
        scsi_process_ack(scsi);
    } else {
        scsi_process_noack(scsi);
    }
    return byte;
}

static void store_prb9(via_context_t *via_context, uint8_t byte, uint8_t p_oldpb,
                      uint16_t addr)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    scsi_context_t *scsi = (scsi_context_t*)(hd->scsi);

    scsi->sel = (byte & 0x10) ? 1 : 0;
    hd->scsi_dir = (byte & 0x08) ? 1 : 0;
    scsi_process_noack(scsi);
    IDBG((LOG, "CMDHD: sprb9: SEL=%d BSY=%d DATA=%02x RST=%d",
        scsi->sel, scsi->bsyo, byte, scsi->rst));
}

static uint8_t read_prb9(via_context_t *via_context)
{
    cmdhd_context_t *hd = (cmdhd_context_t *)via_context->context;
    scsi_context_t *scsi = (scsi_context_t*)(hd->scsi);
    uint8_t temp, state;

    scsi_process_noack(scsi);
    IDBG((LOG, "CMDHD: rprb9: SEL=%d BSY=%d REQ=%d", scsi->sel, scsi->bsyo, scsi->req));
    if ( (via_context->via[VIA_PCR] & 0xf0) == 0xf0 ) {
        temp=(scsi->sel) & (scsi->bsyo);
    } else {
        temp=(!scsi->sel) & (scsi->bsyo);
    }

    /* mask scsi state to cmd specific pld value */
    switch (scsi->state)
    {
        case SCSI_STATE_DATAOUT:
        state = CMD_STATE_DATAOUT;
        break;
        case SCSI_STATE_DATAIN:
        state = CMD_STATE_DATAIN;
        break;
        case SCSI_STATE_COMMAND:
        state = CMD_STATE_COMMAND;
        break;
        case SCSI_STATE_STATUS:
        state = CMD_STATE_STATUS;
        break;
        case SCSI_STATE_MESSAGEOUT:
        state = CMD_STATE_MESSAGEOUT;
        break;
        case SCSI_STATE_MESSAGEIN:
        state = CMD_STATE_MESSAGEIN;
        break;
        default:
        state = CMD_STATE_STATUS;
    }

    return (scsi->req << 7) | (scsi->ack << 6) | (temp << 5) | (scsi->sel << 4) |
        (hd->scsi_dir << 3) | (state & 7);
}

static uint8_t read_prb10(via_context_t *via_context)
{
    uint8_t byte;
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    if (iecbus != NULL) {
        byte = (((via_context->via[VIA_PRB] & 0x1a)
                 | iecbus->drv_port) ^ 0x85);
    } else {
        byte = (((via_context->via[VIA_PRB] & 0x1a)
                 | iec_drive_read(viap->number)) ^ 0x85);
    }

    DEBUG_IEC_DRV_READ(byte);

    DEBUG_IEC_BUS_READ(byte);

    return byte;
}

static void store_prb10(via_context_t *via_context, uint8_t byte, uint8_t oldpb,
                      uint16_t addr)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    if (byte != oldpb) {
        DEBUG_IEC_DRV_WRITE(byte);

        if (iecbus != NULL) {
            uint8_t *drive_data, *drive_bus;
            unsigned int unit;

            drive_bus = &(iecbus->drv_bus[viap->number + 8]);
            drive_data = &(iecbus->drv_data[viap->number + 8]);

            *drive_data = ~byte;
            *drive_bus = ((((*drive_data) << 3) & 0x40)
                          | (((*drive_data) << 6)
                             & (((*drive_data) | iecbus->cpu_bus) << 3) & 0x80));

            iecbus->cpu_port = iecbus->cpu_bus;
            for (unit = 4; unit < 8 + NUM_DISK_UNITS; unit++) {
                iecbus->cpu_port &= iecbus->drv_bus[unit];
            }

            iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                                | (iecbus->cpu_port >> 7)
                                | ((iecbus->cpu_bus << 3) & 0x80));

            DEBUG_IEC_BUS_WRITE(iecbus->drv_port);
        } else {
            iec_drive_write((uint8_t)(~byte), viap->number);
            DEBUG_IEC_BUS_WRITE(~byte);
        }

        iec_fast_drive_direction(byte & 0x20, viap->number);
    }
}

static void undump_prb(via_context_t *via_context, uint8_t byte)
{
}

static void store_pra10(via_context_t *via_context, uint8_t byte, uint8_t p_oldpb,
                      uint16_t addr)
{
}

static void undump_pcr(via_context_t *via_context, uint8_t byte)
{
}

static uint8_t store_pcr(via_context_t *via_context, uint8_t byte, uint16_t addr)
{
    return byte;
}

static void undump_acr(via_context_t *via_context, uint8_t byte)
{
}

static void store_acr(via_context_t *via_context, uint8_t byte)
{
}

static void store_sr(via_context_t *via_context, uint8_t byte)
{
}

static void store_sr10(via_context_t *via_context, uint8_t byte)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    iec_fast_drive_write(byte, viap->number);
}

static void store_t2l(via_context_t *via_context, uint8_t byte)
{
}

static void reset(via_context_t *via_context)
{
}

static uint8_t read_pra10(via_context_t *via_context, uint16_t addr)
{
    return 255;
}

void cmdhd_setup_context(diskunit_context_t *ctxptr)
{
    drivevia_context_t *via10p;
    via_context_t *via10, *via9;
    scsi_context_t *scsi;
    i8255a_state *i8255a;
    char *name = NULL;

    CLOG((LOG, "CMDHD: setup_context"));

    ctxptr->drives[0]->side = 0;

    ctxptr->cmdhd = lib_calloc(1, sizeof(cmdhd_context_t));
    ctxptr->cmdhd->myname = lib_msprintf("CMDHD%u", ctxptr->mynumber);
    ctxptr->cmdhd->mycontext = ctxptr;

    ctxptr->cmdhd->image = NULL;

    /* Clear struct as snapshot code may write uninitialized values.  */
    ctxptr->cmdhd->via10 = lib_calloc(1, sizeof(via_context_t));
    via10 = ctxptr->cmdhd->via10;

    via10->prv = lib_malloc(sizeof(drivevia_context_t));
    via10p = (drivevia_context_t *)(via10->prv);
    via10p->number = ctxptr->mynumber;

    via10->context = (void *)ctxptr->cmdhd;

    via10->rmw_flag = &(ctxptr->cpu->rmw_flag);
    via10->clk_ptr = ctxptr->clk_ptr;

    via10->myname = lib_msprintf("CMDHD%uVIA10", ctxptr->mynumber);
    via10->my_module_name = lib_msprintf("CMDHD%uVIA10", ctxptr->mynumber);

    viacore_setup_context(via10);

    via10->my_module_name_alt1 = lib_msprintf("CMDHDVIA10-%u", ctxptr->mynumber);
    via10->my_module_name_alt2 = lib_msprintf("CMDHDVIA10");

    via10->irq_line = IK_IRQ;

    via10p->drive = ctxptr->drives[0];
    via10p->v_iecbus = iecbus_drive_port();

    via10->undump_pra = undump_pra;
    via10->undump_prb = undump_prb10;
    via10->undump_pcr = undump_pcr;
    via10->undump_acr = undump_acr;
    via10->store_pra = store_pra10;
    via10->store_prb = store_prb10;
    via10->store_pcr = store_pcr;
    via10->store_acr = store_acr;
    via10->store_sr = store_sr10;
    via10->store_t2l = store_t2l;
    via10->read_pra = read_pra10;
    via10->read_prb = read_prb10;
    via10->set_int = set_int;
    via10->restore_int = restore_int;
    via10->set_ca2 = set_ca2;
    via10->set_cb2 = set_cb2;
    via10->reset = reset;

    ctxptr->cmdhd->via9 = lib_calloc(1, sizeof(via_context_t));
    via9 = ctxptr->cmdhd->via9;

    via9->context = (void *)ctxptr->cmdhd;

    via9->rmw_flag = &(ctxptr->cpu->rmw_flag);
    via9->clk_ptr = ctxptr->clk_ptr;

    via9->myname = lib_msprintf("CMDHD%uVIA9", ctxptr->mynumber);
    via9->my_module_name = lib_msprintf("CMDHD%uVIA9", ctxptr->mynumber);

    viacore_setup_context(via9);

    via9->my_module_name_alt1 = lib_msprintf("CMDHDVIA9-%u", ctxptr->mynumber);
    via9->my_module_name_alt2 = lib_msprintf("CMDHDVIA9");

    via9->irq_line = IK_IRQ;

    via9->undump_pra = undump_pra;
    via9->undump_prb = undump_prb;
    via9->undump_pcr = undump_pcr;
    via9->undump_acr = undump_acr;
    via9->store_pra = store_pra9;
    via9->store_prb = store_prb9;
    via9->store_pcr = store_pcr;
    via9->store_acr = store_acr;
    via9->store_sr = store_sr;
    via9->store_t2l = store_t2l;
    via9->read_pra = read_pra9;
    via9->read_prb = read_prb9;
    via9->set_int = set_int;
    via9->restore_int = restore_int;
    via9->set_ca2 = set_ca2;
    via9->set_cb2 = set_cb2;
    via9->reset = reset;

    ctxptr->cmdhd->scsi = lib_calloc(1, sizeof(scsi_context_t));
    scsi = ctxptr->cmdhd->scsi;
    scsi->p = ctxptr->cmdhd;
    scsi->myname = lib_msprintf("CMDHD%uSCSI", ctxptr->mynumber);

    ctxptr->cmdhd->i8255a = lib_calloc(1, sizeof(i8255a_state));
    i8255a = ctxptr->cmdhd->i8255a;
    i8255a->p = ctxptr->cmdhd;
    i8255a->set_pa = set_pa;
    i8255a->set_pb = set_pb;
    i8255a->set_pc = set_pc;
    i8255a->get_pa = get_pa;
    i8255a->get_pb = get_pb;
    i8255a->get_pc = get_pc;

    name = lib_msprintf("CMDHD%uRTC", ctxptr->mynumber);
    ctxptr->cmdhd->rtc = rtc72421_init(name);
    lib_free(name);

    ctxptr->cmdhd->rtc->stop = 0;

    name = lib_msprintf("%sEXEC", ctxptr->cmdhd->myname);
    ctxptr->cmdhd->reset_alarm = alarm_new(ctxptr->cpu->alarm_context, name,
        reset_alarm_handler, ctxptr->cmdhd);
    lib_free(name);

    /* reset attachment counter for warning messages */
    ctxptr->cmdhd->numattached = 0;
}

void cmdhd_init(diskunit_context_t *ctxptr)
{
    scsi_context_t *scsi = (scsi_context_t*)(ctxptr->cmdhd->scsi);

    CLOG((LOG, "CMDHD: init"));

    /* init via cores */
    viacore_init(ctxptr->cmdhd->via9, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status);
    viacore_init(ctxptr->cmdhd->via10, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status);

    /* reset scsi system */
    scsi_reset(scsi);
    /* change any defaults */
    /* grab value from unit structure as it is likely it is initialized
        before the drive init is run */
    scsi->max_imagesize = diskunit_context[ctxptr->mynumber]->fixed_size;
    /* don't allow more than a 24-bit value -2 to be returned on disk query
        as this will cause problems for CMDHD tools */
    scsi->limit_imagesize = 0xfffffe;
    /* CMDHD can handle 56 drives, but the latest boot rom has a bug which
        corrupts data when deleting partitions that span across disk
        boundaries. However, there are a number of users who want multiple
        device and LUN support, so we will make it adaptive. */
    /* Setup SCSI user functions */
    scsi->user_format = cmdhd_scsiformat;
    scsi->user_read = cmdhd_scsiread;
    scsi->user_write = cmdhd_scsiwrite;

    ctxptr->cmdhd->preadyff = 1;
}

void cmdhd_shutdown(cmdhd_context_t *hd)
{
    CLOG((LOG, "CMDHD: shutdown"));

    /* leave if no context provided */
    if (!hd) {
        return;
    }

    rtc72421_destroy(hd->rtc, hd->mycontext->rtc_save);
    /* Don't know if we need this, so why is it even available? */
/*    alarm_destroy(hd->reset_alarm); */
    viacore_shutdown(hd->via9);
    viacore_shutdown(hd->via10);
    lib_free(hd->scsi->myname);
    lib_free(hd->scsi);
    lib_free(hd->i8255a);
    lib_free(hd->myname);
    lib_free(hd);
}

/* Function to find the baselba of the CMD partition. We need this so we
    can modify the device number of the drive as it is stored on disk. */
static void cmdhd_findbaselba(cmdhd_context_t *hd)
{
    uint32_t  i;
    disk_addr_t dadr;
    unsigned char buf[256];
    int rlpresent;

    CLOG((LOG, "CMDHD: findbaselba"));

    /* leave if no context provided */
    if (!hd) {
        return;
    }

    /* reset value to invalid */
    hd->baselba = UINT32_MAX;

    /* leave if no image provided */
    if (!hd->image) {
        return;
    }

    /* look for configuration block */
    i = 2;
    /* start at LBA 2 as in 512-byte blocks */
    while (i < hd->imagesize) {
        /* translate to T/S for DHD images, ie. 65536 for each */
        dadr.track = (i / 32768) + 1;
        dadr.sector = (i % 32768) * 2 + 1;
        /* read the sector */
        if (disk_image_read_sector(hd->image, buf, &dadr) < 0) {
            /* hit the end of file */
            break;
        }
        /* otherwise check the CMD sig */
        if (!cmdhd_has_sig(buf)) {
            /* if it has it, update the offset */
            hd->baselba = i - 2;
            /* quit */
            break;
        }
        /* try next 128 sectors */
        i += 128;
    }

    CLOG((LOG, "CMDHD: findbaselba=%u", hd->baselba));

    /* check if RAMLINK is enabled */
    rlpresent = 0;
    resources_get_int("RAMLINK", &rlpresent);

    if (!hd->mycontext->parallel_cable && rlpresent) {
        hd->mycontext->parallel_cable = 1;
        CRIT((ERR, "CMDHD: RAMLink detected. Drive %u 'parallel cable' set to 'standard'.",
            hd->mycontext->mynumber + 8));
    }
}

void cmdhd_reset(cmdhd_context_t *hd)
{
    CLOCK c;
    size_t i;
    int unit;

    CLOG((LOG, "CMDHD: reset"));

    /* leave if no context provided */
    if (!hd) {
        return;
    }

    /* reset vias */
    viacore_reset(hd->via9);
    viacore_reset(hd->via10);

    /* setup default inputs to U11 (pullups/downs) */
    hd->i8255a_i[0] = 0xff;
    hd->i8255a_i[1] = 0x7f;
    hd->i8255a_i[2] = 0xe3;
    hd->scsi_dir = 0;

    /* check RAM for CMC signature */
    /* The HD ROM does a series of hardware checks on reset if it doesn't
        find a signature in memory. Otherwise it skips to the boot loader.
        If the user holds down a button on reset, we need to set the alarm
        to remove the button press based on this. So on a cold reset, wait
        about 8M cycles, where as if it is a soft reset, wait 500K cycles. */
    c = *(hd->mycontext->clk_ptr) +
        cmdhd_has_sig(&(hd->mycontext->drive_ram[0x9000])) ? 8000000 : 500000;
    CLOG((LOG, "CMDHD: alarm set for %lu from %lu", c, *(hd->mycontext->clk_ptr)));
    alarm_set(hd->reset_alarm, c);

    /* look for base lba as it may have changed on reset */
    cmdhd_findbaselba(hd);

    /* check if write protect button is pressed */
    if (hd->mycontext->button & 1) {
        hd->i8255a_i[1] &= 0xf7;
        CLOG((LOG, "CMDHD: WP pressed down"));
    }
    /* check if swap8 button is pressed */
    if (hd->mycontext->button & 2) {
        hd->i8255a_i[1] &= 0xfd;
        CLOG((LOG, "CMDHD: SWAP8 pressed down"));
    }
    /* check if swap9 button is pressed */
    if (hd->mycontext->button & 4) {
        hd->i8255a_i[1] &= 0xfb;
        CLOG((LOG, "CMDHD: SWAP9 pressed down"));
    }

    /* count the number of connected drives */
    unit = 0;
    for (i = 0; i < 56; i++) {
        if (hd->scsi->file[i]) {
            unit++;
        }
    }

    /* if the image size is too small, put the drive in installation mode */
    /* but if there is more than one drive connect, go to normal mode */
    if (hd->imagesize < 144) {
        if (unit == 1) {
            hd->i8255a_i[1] &= 0xf9;
            CRIT((ERR, "CMDHD: Image size too small, starting up in installation mode."));
            if (hd->mycontext->parallel_cable) {
                hd->mycontext->parallel_cable = 0;
                CRIT((ERR, "CMDHD: Drive %u 'parallel cable' set to none. Set it back to 'standard' when",
                    hd->mycontext->mynumber + 8));
                CRIT((ERR, "CMDHD: HDDOS installation is complete."));
            }
        } else {
            /* remove scsi ID 0 */
            hd->scsi->file[0] = NULL;
        }
    }

    /* make sure the cmdbus isn't held down */
    unit = hd->mycontext->mynumber;
    cmdbus.drv_data[unit] = 0xff;
    cmdbus.drv_bus[unit] = 0xff;

    /* propogate inputs to output */
    i8255a_reset(hd->i8255a);

    /* reset attachment counter for warning messages */
    hd->numattached = 1;
}

int cmdhd_attach_image(disk_image_t *image, unsigned int unit)
{
    cmdhd_context_t *hd;
    char *basename, *testname;
    size_t i, j;
    FILE *test;
    off_t filelength;

    CLOG((LOG, "CMDHD: attach_image"));

    /* standard unit range check */
    if (unit < 8 || unit > 8 + NUM_DISK_UNITS) {
        return -1;
    }

    /* make sure this is a DHD image */
    switch (image->type) {
        case DISK_IMAGE_TYPE_DHD:
            disk_image_attach_log(image, LOG, unit, 0);
            break;
        default:
            return -1;
    }

    /* get context */
    hd = diskunit_context[unit - 8]->cmdhd;

    /* leave if no context provided */
    if (!hd) {
        return -1;
    }

    /* record passed values */
    hd->image = image;
    hd->imagesize = (uint32_t)(disk_image_size(image) >> 9);

    /* leave if there is a problem getting the image size */
    if (hd->imagesize == UINT32_MAX) {
        return -1;
    }

    /* copy file FD to the scsi module */
    hd->scsi->file[0] = image->media.fsimage->fd;

    /* find the base lba */
    cmdhd_findbaselba(hd);

    /* look to see if there are more files with the same base
       name, but different extensions: s10, s11, ..., s20, ..., s67
       s<ID><LUN> */
    /* copy the file name */
    basename = lib_strdup(image->media.fsimage->name);

    /* make sure it ends in some form of DHD */
    i = strlen(basename);
    if (i > 0 &&
        (basename[i - 1] == 'd' || basename[i - 1] == 'D') &&
        (basename[i - 2] == 'h' || basename[i - 2] == 'H') &&
        (basename[i - 3] == 'd' || basename[i - 3] == 'D')) {
        /* strip off the last 2 letters */
        basename[i - 2] = 0;
        /* change the first D to an S */
        basename[i - 3] = (basename[i - 3] & ~31) | 'S';
        /* cycle through all of them S01-S67 */
        for (i = 0; i < 7; i++) {
            /* skip first disk as it has a DHD extension */
            for (j = (i == 0); j < 8; j++) {
               /* generate the name */
               testname = lib_msprintf("%s%" PRI_SIZE_T "%1" PRI_SIZE_T,
                                       basename, i, j);
               /* open the file */
               test = fopen(testname, "rb+");
               if (test) {
                   /* if it is there, check the length */
                   filelength = archdep_file_size(test);
                   /* must be multiple of 512 */
                   if ((filelength % 512) == 0) {
                       /* set the FILE pointer */
                       hd->scsi->file[(i << 3) | j] = test;
                   } else {
                       /* otherwise make sure it is zero */
                       hd->scsi->file[(i << 3) | j] = NULL;
                       fclose(test);
                   }
               }
               /* release any memory */
               lib_free(testname);
            }
        }
    } else {
        /* otherwise clear out SCSI resources just in case */
        for (i = 1; i < 56; i++) {
            hd->scsi->file[i] = NULL;
        }
    }

    /* release any more memory */
    lib_free(basename);

    /* don't do this yet as a lot of 3rd party CMD tools don't expect this */
#if 0
    /* attaching a new disk requires a device reset */
    drive_cpu_trigger_reset(unit - 8);
#endif

    /* process attachment counter for warning messages */
    hd->numattached++;

    if (hd->numattached > 1) {
        CRIT((ERR, "CMDHD: Attaching a new DHD normally requires the drive to be manually reset as"));
        CRIT((ERR, "CMDHD: the HDDOS is not designed to detect this. Exceptions are when handling"));
        CRIT((ERR, "CMDHD: removable media on SCSI IDs other than 0 (ie. changing .S?? files)."));
    }

    return 0;
}

int cmdhd_detach_image(disk_image_t *image, unsigned int unit)
{
    cmdhd_context_t *hd;
    int32_t i;

    CLOG((LOG, "CMDHD: detach_image"));

    /* standard unit range check */
    if (image == NULL || unit < 8 || unit > 8 + NUM_DISK_UNITS) {
        return -1;
    }

    /* make sure this is a DHD image */
    switch (image->type) {
        case DISK_IMAGE_TYPE_DHD:
            disk_image_detach_log(image, LOG, unit, 0);
            break;
        default:
            return -1;
    }

    /* get context */
    hd = diskunit_context[unit - 8]->cmdhd;

    /* leave if no context provided */
    if (!hd) {
        return -1;
    }

    /* remove all image settings in this context */
    hd->image = NULL;
    hd->imagesize = 0;
    hd->baselba = UINT32_MAX;
    hd->scsi->file[0] = NULL;

    /* close all additional SCSI ID files */
    for (i = 1; i < 56; i++) {
        /* if it isn't NULL, it must be a file */
        if (hd->scsi->file[i]) {
            /* close it and set to NULL */
            fclose(hd->scsi->file[i]);
            hd->scsi->file[i] = NULL;
        }
    }

    /* make sure the cmdbus isn't held down */
    cmdbus.drv_data[unit - 8] = 0xff;
    cmdbus.drv_bus[unit - 8] = 0xff;

    return 0;
}

int cmdhd_update_maxsize(unsigned int size, unsigned int unit)
{
    cmdhd_context_t *hd;

    /* standard unit range check */
    if (unit < 8 || unit > 8 + NUM_DISK_UNITS) {
        return -1;
    }

    /* get context */
    hd = diskunit_context[unit - 8]->cmdhd;

    /* leave if no context provided */
    if (!hd) {
        return -1;
    }

    hd->scsi->max_imagesize = size;
    return 0;
}

#define CMDHD_SNAP_MAJOR 1
#define CMDHD_SNAP_MINOR 0

int cmdhd_snapshot_write_module(cmdhd_context_t *drv, struct snapshot_s *s)
{
    snapshot_module_t *m;

    CLOG((LOG, "CMDHD: snapshot_write_module"));

    m = snapshot_module_create(s, drv->myname, CMDHD_SNAP_MAJOR, CMDHD_SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, drv->LEDs) < 0
        || SMW_BA(m, drv->i8255a_i, 3) < 0
        || SMW_BA(m, drv->i8255a_o, 3) < 0
        || SMW_B(m, drv->scsi_dir) < 0
        || SMW_B(m, drv->preadyff) < 0 ) {
        snapshot_module_close(m);
        return -1;
    }

    if (i8255a_snapshot_write_data(drv->i8255a, m) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    if (viacore_snapshot_write_module(drv->via9, s) < 0) {
        return -1;
    }

    if (viacore_snapshot_write_module(drv->via10, s) < 0) {
        return -1;
    }

    if (scsi_snapshot_write_module(drv->scsi, s) < 0) {
        return -1;
    }

    if (rtc72421_write_snapshot(drv->rtc, s) < 0) {
        return -1;
    }

    return 0;
}

int cmdhd_snapshot_read_module(cmdhd_context_t *drv, struct snapshot_s *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    CLOG((LOG, "CMDHD: snapshot_read_module"));

    m = snapshot_module_open(s, drv->myname, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (snapshot_version_is_bigger(vmajor, vminor, CMDHD_SNAP_MAJOR, CMDHD_SNAP_MAJOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || SMR_B(m, &drv->LEDs) < 0
        || SMR_BA(m, drv->i8255a_i, 3) < 0
        || SMR_BA(m, drv->i8255a_o, 3) < 0
        || SMR_B(m, &drv->scsi_dir) < 0
        || SMR_B(m, &drv->preadyff) < 0 ) {
        snapshot_module_close(m);
        return -1;
    }

    if (i8255a_snapshot_read_data(drv->i8255a, m) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    alarm_unset(drv->reset_alarm);

    if (viacore_snapshot_read_module(drv->via9, s) < 0) {
        return -1;
    }

    if (viacore_snapshot_read_module(drv->via10, s) < 0) {
        return -1;
    }

    if (scsi_snapshot_read_module(drv->scsi, s) < 0) {
        return -1;
    }

    if (rtc72421_read_snapshot(drv->rtc, s) < 0) {
        return -1;
    }

    return 0;
}

