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
#define IO_DETACH_NEVER    2

#define IO_DETACH_NO_RESOURCE NULL

#define IO_COLLISION_METHOD_DETACH_ALL    0
#define IO_COLLISION_METHOD_DETACH_LAST   1
#define IO_COLLISION_METHOD_AND_WIRES     2

#define IO_PRIO_HIGH     1      /*!< override others on collisions */
#define IO_PRIO_NORMAL   0      /*!< handle collisions */
#define IO_PRIO_LOW     -1      /*!< overridden by others on collisions */

#define IO_CART_ID_NONE 0

#define IO_MIRROR_NONE  0   /*!< registered area contains no mirrors */
#define IO_MIRROR_OTHER 1   /*!< registered area contains mirrors of another registered area */
#define IO_MIRROR_MASK  2   /*!< registered area contains mirrors of itself, determined by address_mask */

uint8_t c64io_d000_read(uint16_t addr);
uint8_t c64io_d000_peek(uint16_t addr);
void c64io_d000_store(uint16_t addr, uint8_t value);
uint8_t c64io_d100_read(uint16_t addr);
uint8_t c64io_d100_peek(uint16_t addr);
void c64io_d100_store(uint16_t addr, uint8_t value);
uint8_t c64io_d200_read(uint16_t addr);
uint8_t c64io_d200_peek(uint16_t addr);
void c64io_d200_store(uint16_t addr, uint8_t value);
uint8_t c64io_d300_read(uint16_t addr);
uint8_t c64io_d300_peek(uint16_t addr);
void c64io_d300_store(uint16_t addr, uint8_t value);
uint8_t c64io_d400_read(uint16_t addr);
uint8_t c64io_d400_peek(uint16_t addr);
void c64io_d400_store(uint16_t addr, uint8_t value);
uint8_t c64io_d500_read(uint16_t addr);
uint8_t c64io_d500_peek(uint16_t addr);
void c64io_d500_store(uint16_t addr, uint8_t value);
uint8_t c64io_d600_read(uint16_t addr);
uint8_t c64io_d600_peek(uint16_t addr);
void c64io_d600_store(uint16_t addr, uint8_t value);
uint8_t c64io_d700_read(uint16_t addr);
uint8_t c64io_d700_peek(uint16_t addr);
void c64io_d700_store(uint16_t addr, uint8_t value);
uint8_t c64io_dd00_read(uint16_t addr);
uint8_t c64io_dd00_peek(uint16_t addr);
void c64io_dd00_store(uint16_t addr, uint8_t value);
uint8_t c64io_de00_read(uint16_t addr);
uint8_t c64io_de00_peek(uint16_t addr);
void c64io_de00_store(uint16_t addr, uint8_t value);
uint8_t c64io_df00_read(uint16_t addr);
uint8_t c64io_df00_peek(uint16_t addr);
void c64io_df00_store(uint16_t addr, uint8_t value);

uint8_t vic20io0_read(uint16_t addr);
uint8_t vic20io0_peek(uint16_t addr);
void vic20io0_store(uint16_t addr, uint8_t value);
uint8_t vic20io2_read(uint16_t addr);
uint8_t vic20io2_peek(uint16_t addr);
void vic20io2_store(uint16_t addr, uint8_t value);
uint8_t vic20io3_read(uint16_t addr);
uint8_t vic20io3_peek(uint16_t addr);
void vic20io3_store(uint16_t addr, uint8_t value);

uint8_t cbm2io_d800_read(uint16_t addr);
uint8_t cbm2io_d800_peek(uint16_t addr);
void cbm2io_d800_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_d900_read(uint16_t addr);
uint8_t cbm2io_d900_peek(uint16_t addr);
void cbm2io_d900_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_da00_read(uint16_t addr);
uint8_t cbm2io_da00_peek(uint16_t addr);
void cbm2io_da00_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_db00_read(uint16_t addr);
uint8_t cbm2io_db00_peek(uint16_t addr);
void cbm2io_db00_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_dc00_read(uint16_t addr);
uint8_t cbm2io_dc00_peek(uint16_t addr);
void cbm2io_dc00_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_dd00_read(uint16_t addr);
uint8_t cbm2io_dd00_peek(uint16_t addr);
void cbm2io_dd00_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_de00_read(uint16_t addr);
uint8_t cbm2io_de00_peek(uint16_t addr);
void cbm2io_de00_store(uint16_t addr, uint8_t value);
uint8_t cbm2io_df00_read(uint16_t addr);
uint8_t cbm2io_df00_peek(uint16_t addr);
void cbm2io_df00_store(uint16_t addr, uint8_t value);

uint8_t petio_8800_read(uint16_t addr);
uint8_t petio_8800_peek(uint16_t addr);
void petio_8800_store(uint16_t addr, uint8_t value);
uint8_t petio_8900_read(uint16_t addr);
uint8_t petio_8900_peek(uint16_t addr);
void petio_8900_store(uint16_t addr, uint8_t value);
uint8_t petio_8a00_read(uint16_t addr);
uint8_t petio_8a00_peek(uint16_t addr);
void petio_8a00_store(uint16_t addr, uint8_t value);
uint8_t petio_8b00_read(uint16_t addr);
uint8_t petio_8b00_peek(uint16_t addr);
void petio_8b00_store(uint16_t addr, uint8_t value);
uint8_t petio_8c00_read(uint16_t addr);
uint8_t petio_8c00_peek(uint16_t addr);
void petio_8c00_store(uint16_t addr, uint8_t value);
uint8_t petio_8d00_read(uint16_t addr);
uint8_t petio_8d00_peek(uint16_t addr);
void petio_8d00_store(uint16_t addr, uint8_t value);
uint8_t petio_8e00_read(uint16_t addr);
uint8_t petio_8e00_peek(uint16_t addr);
void petio_8e00_store(uint16_t addr, uint8_t value);
uint8_t petio_8f00_read(uint16_t addr);
uint8_t petio_8f00_peek(uint16_t addr);
void petio_8f00_store(uint16_t addr, uint8_t value);

uint8_t petio_e900_read(uint16_t addr);
uint8_t petio_e900_peek(uint16_t addr);
void petio_e900_store(uint16_t addr, uint8_t value);
uint8_t petio_ea00_read(uint16_t addr);
uint8_t petio_ea00_peek(uint16_t addr);
void petio_ea00_store(uint16_t addr, uint8_t value);
uint8_t petio_eb00_read(uint16_t addr);
uint8_t petio_eb00_peek(uint16_t addr);
void petio_eb00_store(uint16_t addr, uint8_t value);
uint8_t petio_ec00_read(uint16_t addr);
uint8_t petio_ec00_peek(uint16_t addr);
void petio_ec00_store(uint16_t addr, uint8_t value);
uint8_t petio_ed00_read(uint16_t addr);
uint8_t petio_ed00_peek(uint16_t addr);
void petio_ed00_store(uint16_t addr, uint8_t value);
uint8_t petio_ee00_read(uint16_t addr);
uint8_t petio_ee00_peek(uint16_t addr);
void petio_ee00_store(uint16_t addr, uint8_t value);
uint8_t petio_ef00_read(uint16_t addr);
uint8_t petio_ef00_peek(uint16_t addr);
void petio_ef00_store(uint16_t addr, uint8_t value);

uint8_t plus4io_fd00_read(uint16_t addr);
uint8_t plus4io_fd00_peek(uint16_t addr);
void plus4io_fd00_store(uint16_t addr, uint8_t value);
uint8_t plus4io_fe00_read(uint16_t addr);
uint8_t plus4io_fe00_peek(uint16_t addr);
void plus4io_fe00_store(uint16_t addr, uint8_t value);

struct mem_ioreg_list_s;

void io_source_ioreg_add_list(struct mem_ioreg_list_s **mem_ioreg_list);

/* The following structure is used to register the I/O address range used by a certain device/chip/cartridge.
 *
 * The address_mask determines if the defined address range is mirrored across a bigger range, the mask tells
 * the I/O read/write handler how many bits of the address being accessed are valid for an I/O read or write.
 *
 * Some examples:
 *
 * start_address | end_address | mask | primary register address | address mirrors
 * -------------------------------------------------------------------------------
 * $de00         | $de0f       | $00  | $de00                    | $de01-$de0f (mirrors of 0xde00)
 * $de00         | $de03       | $01  | $de00-$de01              | $de02-$de03 ($de02 mirrors $de00 and $de03 mirrors $de01)
 * $de00         | $deff       | $0f  | $de00-$de0f              | $de10-$deff (15 blocks of 16 bytes mirroring $de00-$de0f)
 * $de00         | $deff       | $7f  | $de00-$de7f              | $de80-$deff (1 block of 128 bytes mirroring $de00-$de7f)
 * $de80         | $deff       | $7f  | $de80-$deff              | no mirrors
 * $de00         | $de0f       | $0f  | $de00-$de0f              | no mirrors
 * $de00         | $deff       | $ff  | $de00-$deff              | no mirrors
 *
 * If the read/write functions of the device ignore the address parameter then it results in the following:
 *
 * start_address | end_address | mask | primary register address | address mirrors
 * -------------------------------------------------------------------------------
 * $de00         | $deff       | $ff  | $de00                    | $de01-$deff
 *
 * Devices can co-exist at the same addresses, but only if they are write only.
 *
 * A write to an address that is used by multiple devices just results in the data being written going to all devices.
 *
 * A read from an address that is used by multiple devices leads to problems, because all devices will respond with
 * data to the read, in which case there will be a read-collision. The default action for a read-collision is to report the
 * user with the names of the devices and the address at which the collision happened, and then detach all devices causing
 * the read-collision. This default action can be changed to detach only the last inserted device involved in the
 * read-collision, or AND the returned bytes instead of detaching devices.
 *
 * The io_source_prio defines what to do with the returned byte of a device.
 *
 * A priority of -1 (IO_PRIO_LOW) means the returned byte of the device should be ignored if other devices return a byte as well,
 * therefor a device of this priority only returns a byte if it is the only device at the address being read.
 *
 * A priority of 1 (IO_PRIO_HIGH) means the returned byte will be returned immediately and no further read-collision checks are done,
 * therefor a device of this priority will always return a byte, unless it is not the first device in the device-chain at the
 * address being read. If it is not the first device it will not return a byte, but will also not be part of a read-collision.
 *
 * A priority of 0 (IO_PRIO_NORMAL) means there needs to be a check of read-collisions, and the device byte will be used either
 * in an AND of all returned bytes, or as a device in a read-collision.
 *
 * Note: When making emulation code of new devices with their range in the I/O ranges do NOT use IO_PRIO_HIGH unless the device
 *       is a device on the system board, or a cartridge with a passthrough port, if the device doesn't work because of read
 *       collisions do NOT use IO_PRIO_HIGH as a hack to get it to work, the cause is in the emulation code, using IO_PRIO_HIGH
 *       in such a case will only hide the actual problem.
 *
 * The I/O ranges that are covered by the I/O system are as follows:
 *
 * C64/C128: $d000-$d0ff, $d100-$d1ff, $d200-$d2ff, $d300-$d3ff, $d400-$d4ff, $d500-$d5ff, $d600-$d6ff, $d700-$d7ff,
 *           $de00-$deff, $df00-$dfff
 *
 * SCPU64:   $d000-$d0ff, $d100-$d1ff, $d400-$d4ff, $d500-$d5ff, $d600-$d6ff, $d700-$d7ff, $de00-$deff, $df00-$dfff
 *
 *     CBM2: $d800-$d8ff, $d900-$d9ff, $da00-$daff, $db00-$dbff, $dc00-$d9ff, $dd00-$d9ff, $de00-$deff, $df00-$dfff
 *
 *      PET: $8800-$88ff, $8900-$89ff, $8a00-$8aff, $8b00-$8bff, $8c00-$8cff, $8d00-$8dff, $8e00-$8eff, $8f00-$8fff,
 *           $e900-$e9ff, $ea00-$eaff, $eb00-$ebff, $ec00-$ecff, $ed00-$edff, $ee00-$eeff, $ef00-$efff
 *
 *    PLUS4: $fd00-$fdff, $fe00-$feff
 *
 *    VIC20: $9000-$93ff, $9800-$9bff, $9c00-$9fff
 */

typedef struct io_source_s {
    char *name; /*!< literal name of this I/O device */
    int detach_id;
    char *resource_name;
    uint16_t start_address;
    uint16_t end_address;
    uint16_t address_mask;
    int io_source_valid;  /*!< after reading, is 1 if read was valid */
    void (*store)(uint16_t address, uint8_t data);
    void (*poke)(uint16_t address, uint8_t data); /*!< write without side effects (used by monitor) */
    uint8_t (*read)(uint16_t address);
    uint8_t (*peek)(uint16_t address); /*!< read without side effects (used by monitor) */
    int (*dump)(void); /*!< print detailed state for this i/o device (used by monitor) */
    int cart_id; /*!< id of associated cartridge */
    int io_source_prio; /*!< 0: normal, 1: higher priority (no collisions), -1: lower priority (no collisions) */
    unsigned int order; /*!< a tag to indicate the order of insertion */
    int mirror_mode; /*!< a tag to indicate the type of mirroring */
} io_source_t;

/* The I/O source list structure is a double linked list for easy insertion/removal of devices. */
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

io_source_list_t *io_source_register(io_source_t *device);
void io_source_unregister(io_source_list_t *device);

void cartio_shutdown(void);

void c64io_vicii_init(void);
void c64io_vicii_deinit(void);
void c64io_vicii_reinit(void);

int cartio_resources_init(void);
int cartio_cmdline_options_init(void);
void cartio_set_highest_order(unsigned int nr);

#endif
