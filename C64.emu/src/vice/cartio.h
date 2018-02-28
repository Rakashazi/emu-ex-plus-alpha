/*
 * cartio.h -- C64/C128/VIC20/CBM2/PET/PLUS4 I/O handling.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_CARTIO_H
#define VICE_CARTIO_H

#include "types.h"

#define IO_DETACH_CART     0
#define IO_DETACH_RESOURCE 1

#define IO_COLLISION_METHOD_DETACH_ALL    0
#define IO_COLLISION_METHOD_DETACH_LAST   1
#define IO_COLLISION_METHOD_AND_WIRES     2

#define IO_PRIO_HIGH     1      /*!< override others on collisions */
#define IO_PRIO_NORMAL   0      /*!< handle collisions */
#define IO_PRIO_LOW     -1      /*!< overridden by others on collisions */

extern BYTE c64io_d000_read(WORD addr);
extern BYTE c64io_d000_peek(WORD addr);
extern void c64io_d000_store(WORD addr, BYTE value);
extern BYTE c64io_d100_read(WORD addr);
extern BYTE c64io_d100_peek(WORD addr);
extern void c64io_d100_store(WORD addr, BYTE value);
extern BYTE c64io_d200_read(WORD addr);
extern BYTE c64io_d200_peek(WORD addr);
extern void c64io_d200_store(WORD addr, BYTE value);
extern BYTE c64io_d300_read(WORD addr);
extern BYTE c64io_d300_peek(WORD addr);
extern void c64io_d300_store(WORD addr, BYTE value);
extern BYTE c64io_d400_read(WORD addr);
extern BYTE c64io_d400_peek(WORD addr);
extern void c64io_d400_store(WORD addr, BYTE value);
extern BYTE c64io_d500_read(WORD addr);
extern BYTE c64io_d500_peek(WORD addr);
extern void c64io_d500_store(WORD addr, BYTE value);
extern BYTE c64io_d600_read(WORD addr);
extern BYTE c64io_d600_peek(WORD addr);
extern void c64io_d600_store(WORD addr, BYTE value);
extern BYTE c64io_d700_read(WORD addr);
extern BYTE c64io_d700_peek(WORD addr);
extern void c64io_d700_store(WORD addr, BYTE value);
extern BYTE c64io_de00_read(WORD addr);
extern BYTE c64io_de00_peek(WORD addr);
extern void c64io_de00_store(WORD addr, BYTE value);
extern BYTE c64io_df00_read(WORD addr);
extern BYTE c64io_df00_peek(WORD addr);
extern void c64io_df00_store(WORD addr, BYTE value);

extern BYTE vic20io0_read(WORD addr);
extern BYTE vic20io0_peek(WORD addr);
extern void vic20io0_store(WORD addr, BYTE value);
extern BYTE vic20io2_read(WORD addr);
extern BYTE vic20io2_peek(WORD addr);
extern void vic20io2_store(WORD addr, BYTE value);
extern BYTE vic20io3_read(WORD addr);
extern BYTE vic20io3_peek(WORD addr);
extern void vic20io3_store(WORD addr, BYTE value);

extern BYTE cbm2io_d800_read(WORD addr);
extern BYTE cbm2io_d800_peek(WORD addr);
extern void cbm2io_d800_store(WORD addr, BYTE value);
extern BYTE cbm2io_d900_read(WORD addr);
extern BYTE cbm2io_d900_peek(WORD addr);
extern void cbm2io_d900_store(WORD addr, BYTE value);
extern BYTE cbm2io_da00_read(WORD addr);
extern BYTE cbm2io_da00_peek(WORD addr);
extern void cbm2io_da00_store(WORD addr, BYTE value);
extern BYTE cbm2io_db00_read(WORD addr);
extern BYTE cbm2io_db00_peek(WORD addr);
extern void cbm2io_db00_store(WORD addr, BYTE value);
extern BYTE cbm2io_dc00_read(WORD addr);
extern BYTE cbm2io_dc00_peek(WORD addr);
extern void cbm2io_dc00_store(WORD addr, BYTE value);
extern BYTE cbm2io_dd00_read(WORD addr);
extern BYTE cbm2io_dd00_peek(WORD addr);
extern void cbm2io_dd00_store(WORD addr, BYTE value);
extern BYTE cbm2io_de00_read(WORD addr);
extern BYTE cbm2io_de00_peek(WORD addr);
extern void cbm2io_de00_store(WORD addr, BYTE value);
extern BYTE cbm2io_df00_read(WORD addr);
extern BYTE cbm2io_df00_peek(WORD addr);
extern void cbm2io_df00_store(WORD addr, BYTE value);

extern BYTE petio_8800_read(WORD addr);
extern BYTE petio_8800_peek(WORD addr);
extern void petio_8800_store(WORD addr, BYTE value);
extern BYTE petio_8900_read(WORD addr);
extern BYTE petio_8900_peek(WORD addr);
extern void petio_8900_store(WORD addr, BYTE value);
extern BYTE petio_8a00_read(WORD addr);
extern BYTE petio_8a00_peek(WORD addr);
extern void petio_8a00_store(WORD addr, BYTE value);
extern BYTE petio_8b00_read(WORD addr);
extern BYTE petio_8b00_peek(WORD addr);
extern void petio_8b00_store(WORD addr, BYTE value);
extern BYTE petio_8c00_read(WORD addr);
extern BYTE petio_8c00_peek(WORD addr);
extern void petio_8c00_store(WORD addr, BYTE value);
extern BYTE petio_8d00_read(WORD addr);
extern BYTE petio_8d00_peek(WORD addr);
extern void petio_8d00_store(WORD addr, BYTE value);
extern BYTE petio_8e00_read(WORD addr);
extern BYTE petio_8e00_peek(WORD addr);
extern void petio_8e00_store(WORD addr, BYTE value);
extern BYTE petio_8f00_read(WORD addr);
extern BYTE petio_8f00_peek(WORD addr);
extern void petio_8f00_store(WORD addr, BYTE value);

extern BYTE petio_e900_read(WORD addr);
extern BYTE petio_e900_peek(WORD addr);
extern void petio_e900_store(WORD addr, BYTE value);
extern BYTE petio_ea00_read(WORD addr);
extern BYTE petio_ea00_peek(WORD addr);
extern void petio_ea00_store(WORD addr, BYTE value);
extern BYTE petio_eb00_read(WORD addr);
extern BYTE petio_eb00_peek(WORD addr);
extern void petio_eb00_store(WORD addr, BYTE value);
extern BYTE petio_ec00_read(WORD addr);
extern BYTE petio_ec00_peek(WORD addr);
extern void petio_ec00_store(WORD addr, BYTE value);
extern BYTE petio_ed00_read(WORD addr);
extern BYTE petio_ed00_peek(WORD addr);
extern void petio_ed00_store(WORD addr, BYTE value);
extern BYTE petio_ee00_read(WORD addr);
extern BYTE petio_ee00_peek(WORD addr);
extern void petio_ee00_store(WORD addr, BYTE value);
extern BYTE petio_ef00_read(WORD addr);
extern BYTE petio_ef00_peek(WORD addr);
extern void petio_ef00_store(WORD addr, BYTE value);

extern BYTE plus4io_fd00_read(WORD addr);
extern BYTE plus4io_fd00_peek(WORD addr);
extern void plus4io_fd00_store(WORD addr, BYTE value);
extern BYTE plus4io_fe00_read(WORD addr);
extern BYTE plus4io_fe00_peek(WORD addr);
extern void plus4io_fe00_store(WORD addr, BYTE value);

struct mem_ioreg_list_s;
extern void io_source_ioreg_add_list(struct mem_ioreg_list_s **mem_ioreg_list);

typedef struct io_source_s {
    char *name; /*!< literal name of this I/O device */
    int detach_id;
    char *resource_name;
    WORD start_address;
    WORD end_address;
    WORD address_mask;
    int io_source_valid;  /*!< after reading, is 1 if read was valid */
    void (*store)(WORD address, BYTE data);
    BYTE (*read)(WORD address);
    BYTE (*peek)(WORD address); /*!< read without side effects (used by monitor) */
    int (*dump)(void); /*!< print detailed state for this i/o device (used by monitor) */
    int cart_id; /*!< id of associated cartridge */
    int io_source_prio; /*!< 0: normal, 1: higher priority (no collisions), -1: lower priority (no collisions) */
    unsigned int order; /*!< a tag to indicate the order of insertion */
} io_source_t;

typedef struct io_source_list_s {
    struct io_source_list_s *previous;
    io_source_t *device;
    struct io_source_list_s *next;
} io_source_list_t;

typedef struct io_source_detach_s {
    int det_id;
    char *det_devname;
    char *det_name;
    int det_cartid;
    unsigned int order;
} io_source_detach_t;

extern io_source_list_t *io_source_register(io_source_t *device);
extern void io_source_unregister(io_source_list_t *device);

extern void cartio_shutdown(void);

extern void c64io_vicii_init(void);
extern void c64io_vicii_deinit(void);

extern int cartio_resources_init(void);
extern int cartio_cmdline_options_init(void);
extern void cartio_set_highest_order(unsigned int nr);

#endif
