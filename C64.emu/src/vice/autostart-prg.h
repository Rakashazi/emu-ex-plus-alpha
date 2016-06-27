/*
 * autostart-prg.h - Handle autostart of program files
 *
 * Written by
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifndef VICE_AUTOSTART_PRG_H
#define VICE_AUTOSTART_PRG_H

#include "types.h"
#include "log.h"
#include "fileio.h"

struct autostart_prg_s {
    BYTE *data;
    WORD start_addr;
    DWORD size;
};
typedef struct autostart_prg_s autostart_prg_t;

#define AUTOSTART_PRG_MODE_VFS      0
#define AUTOSTART_PRG_MODE_INJECT   1
#define AUTOSTART_PRG_MODE_DISK     2
#define AUTOSTART_PRG_MODE_LAST     2
#define AUTOSTART_PRG_MODE_DEFAULT  AUTOSTART_PRG_MODE_DISK

extern void autostart_prg_init(void);
extern void autostart_prg_shutdown(void);

extern int autostart_prg_with_virtual_fs(const char *file_name,
                                         fileio_info_t *fh, log_t log);
extern int autostart_prg_with_ram_injection(const char *file_name,
                                            fileio_info_t *fh, log_t log);
extern int autostart_prg_with_disk_image(const char *file_name,
                                         fileio_info_t *fh, log_t log,
                                         const char *image_name);

extern int autostart_prg_perform_injection(log_t log);

#endif
