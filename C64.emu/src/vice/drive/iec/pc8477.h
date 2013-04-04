/*
 * pc8477.h - dp8473/pc8477 emulation for the 4000 disk drive.
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

#ifndef VICE_PC8477_H
#define VICE_PC8477_H

#include "types.h"

struct disk_image_s;
struct drive_context_s;
typedef void (*pc8477_motor_on_callback_t)(void *data, int signal);
typedef struct pc8477_s pc8477_t;

/* FIXME: whats the deal with the different prefixes? */
extern void pc8477d_init(struct drive_context_s *drv);
extern void pc8477_shutdown(pc8477_t *drv);

extern void pc8477_setup_context(struct drive_context_s *drv);
extern void pc8477d_store(struct drive_context_s *drv, WORD addr, BYTE byte);
extern BYTE pc8477d_read(struct drive_context_s *drv, WORD addr);
extern void pc8477_reset(pc8477_t *drv, int is8477);
extern inline int pc8477_irq(pc8477_t *drv);

extern int pc8477_attach_image(struct disk_image_s *image, unsigned int unit);
extern int pc8477_detach_image(struct disk_image_s *image, unsigned int unit);

#endif
