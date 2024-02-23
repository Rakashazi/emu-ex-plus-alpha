/*
 * vic20mem.h -- VIC20 memory handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Daniel Kahlin <daniel@kahlin.net>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Memory configuration handling by
 *  Alexander Lehmann <alex@mathematik.th-darmstadt.de>
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

#ifndef VICE_VIC20MEM_H
#define VICE_VIC20MEM_H

#include "mem.h"
#include "types.h"

#define VIC20_RAM_SIZE                  0x10000 /* Kludged... */

/* VIC20 memory-related resources.  */
#define VIC_BLK0 1
#define VIC_BLK1 2
#define VIC_BLK2 4
#define VIC_BLK3 8
#define VIC_BLK5 16
#define VIC_BLK_ALL (VIC_BLK0 | VIC_BLK1 | VIC_BLK2 | VIC_BLK3 | VIC_BLK5)

/* new cart system */
#define VIC_CART_RAM123  (1 << 0)
#define VIC_CART_BLK1    (1 << 1)
#define VIC_CART_BLK2    (1 << 2)
#define VIC_CART_BLK3    (1 << 3)
#define VIC_CART_BLK5    (1 << 4)
#define VIC_CART_IO2     (1 << 5)
#define VIC_CART_IO3     (1 << 6)

int vic20_mem_init_resources(void);
int vic20_mem_init_cmdline_options(void);
int vic20_mem_disable_ram_block(int num);
int vic20_mem_enable_ram_block(int num);

/* this should go away */
void mem_attach_cartridge(int type, uint8_t *rawcart);
void mem_detach_cartridge(int type);

/* Last data read/write by the cpu, this value lingers on the C(PU)-bus and
   gets used when the CPU reads from unconnected space on the C(PU)-bus */
extern uint8_t vic20_cpu_last_data;

/* Last read data on V-bus (VD0-VD7) */
extern uint8_t vic20_v_bus_last_data;

/* Last read data on V-bus (VD8-VD11) */
extern uint8_t vic20_v_bus_last_high;

extern uint8_t vfli_ram[0x4000];

/* Update V-bus values after V-bus read ($0000-$1FFF, $8000-$9FFF) */
inline static void vic20_mem_v_bus_read(uint16_t addr)
{
    vic20_v_bus_last_data = vic20_cpu_last_data;
    vic20_v_bus_last_high = mem_ram[0x9400 + (addr & 0x3ff)];
}

/* Update V-bus values after V-bus write ($0000-$1FFF, $8000-$9FFF) */
/* TODO: same as vic20_mem_v_bus_read? */
inline static void vic20_mem_v_bus_store(uint16_t addr)
{
    vic20_v_bus_last_data = vic20_cpu_last_data;
}

#endif
