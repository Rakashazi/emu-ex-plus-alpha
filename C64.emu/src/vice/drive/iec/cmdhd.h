/*
 * cmdhd.h - Whole CMDHD emulation
 *
 * Written by
 *  Roberto Muscedere (muscedereroberto@gmail.com)
 *
 * Based on old code by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_CMDHD_H
#define VICE_CMDHD_H

#include "types.h"
#include "diskimage.h"
#include "rtc-72421.h"
#include "alarm.h"
#include "via.h"
#include "scsi.h"
#include "i8255a.h"

struct diskunit_context_s;
struct via_context_s;

typedef struct cmdhd_context_s {
    char *myname;
    struct diskunit_context_s *mycontext;
    struct via_context_s *via9;
    struct via_context_s *via10;
    struct scsi_context_s *scsi;
    rtc_72421_t *rtc;
    uint8_t LEDs;
    uint32_t imagesize;
    uint32_t baselba;
    alarm_t *reset_alarm;
    struct disk_image_s *image;
    i8255a_state *i8255a;
    uint8_t i8255a_i[3];
    uint8_t i8255a_o[3];
    uint8_t scsi_dir;
    uint8_t preadyff;
    uint8_t numattached;
} cmdhd_context_t;

typedef struct cmdhd_context_s cmdhd_context_t;

void cmdhd_shutdown(struct cmdhd_context_s *ctxptr);
void cmdhd_setup_context(struct diskunit_context_s *ctxptr);
void cmdhd_init(struct diskunit_context_s *ctxptr);
void cmdhd_reset(struct cmdhd_context_s *ctxptr);
void cmdhd_reset_soft(diskunit_context_t *drv);

void cmdhd_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t byte);
uint8_t cmdhd_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t cmdhd_peek(struct diskunit_context_s *ctxptr, uint16_t addr);
int cmdhd_dump(diskunit_context_t *ctxptr, uint16_t addr);
int cmdhd_attach_image(disk_image_t *image, unsigned int unit);
int cmdhd_detach_image(disk_image_t *image, unsigned int unit);
int cmdhd_update_maxsize(unsigned int size, unsigned int unit);

int cmdhd_snapshot_write_module(cmdhd_context_t *drv, struct snapshot_s *s);
int cmdhd_snapshot_read_module(cmdhd_context_t *drv, struct snapshot_s *s);

typedef struct cmdbus_s {
/* device 8 references 0 */
    uint8_t drv_bus[NUM_DISK_UNITS];
/*
   bit 7 is /PREADY
   bit 6 is /PCLK
   bit 5 is /PATN
   bit 4 is /PEXT
   bit 3..1 is 1
   bit 0 is 1 (if it is 0, then it will not contribute to the bus when
       cmdbus_update is called)
*/
    uint8_t drv_data[NUM_DISK_UNITS];
    uint8_t cpu_bus; /* same as drv_bus */
    uint8_t cpu_data;
    uint8_t bus; /* same as drv_bus */
    uint8_t data;
} cmdbus_t;

extern cmdbus_t cmdbus;

void cmdbus_init(void);
void cmdbus_update(void);
void cmdbus_patn_changed(int new, int old);

#endif
