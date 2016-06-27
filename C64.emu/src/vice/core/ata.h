/*
 * ata.h - ATA(PI) device emulation
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

#ifndef VICE_ATA
#define VICE_ATA

#include "types.h"

typedef enum ata_drive_type_e {
    ATA_DRIVE_NONE, ATA_DRIVE_HDD, ATA_DRIVE_FDD, ATA_DRIVE_CD, ATA_DRIVE_CF
} ata_drive_type_t;

typedef struct ata_drive_s ata_drive_t;
typedef struct ata_drive_geometry_s {
    int cylinders, heads, sectors, size;
} ata_drive_geometry_t;

extern ata_drive_t *ata_init(int drive);
extern void ata_shutdown(ata_drive_t *drv);
extern void ata_register_store(ata_drive_t *cdrive, WORD addr, WORD value);
extern WORD ata_register_read(ata_drive_t *cdrive, WORD addr, WORD bus);
extern WORD ata_register_peek(ata_drive_t *cdrive, WORD addr);
extern int ata_register_dump(ata_drive_t *cdrive);
extern void ata_image_attach(ata_drive_t *cdrive, char *filename, ata_drive_type_t type, ata_drive_geometry_t geometry);
extern void ata_image_detach(ata_drive_t *cdrive);
extern int ata_image_change(ata_drive_t *cdrive, char *filename, ata_drive_type_t type, ata_drive_geometry_t geometry);
extern void ata_reset(ata_drive_t *cdrive);
void ata_update_timing(ata_drive_t *drv, CLOCK cycles_1s);

struct snapshot_s;
extern int ata_snapshot_read_module(ata_drive_t *drv, struct snapshot_s *s);
extern int ata_snapshot_write_module(ata_drive_t *drv, struct snapshot_s *s);

#endif
