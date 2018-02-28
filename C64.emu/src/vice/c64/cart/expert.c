/*
 * expert.c - Cartridge handling, Expert cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Nathan Huizinga <nathan.huizinga@chess.nl>
 *  Groepaz <groepaz@gmx.net>
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

#include "archdep.h"
#define CARTRIDGE_INCLUDE_SLOT1_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT1_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "interrupt.h"
#include "lib.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "expert.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    FIXME: the following description is atleast inaccurate, if not plain wrong.

    Trilogic Expert Cartridge

    - one 8K RAM (!) bank
    - IO1 seems to be connected to a flipflop, which makes a single bit "register".
      wether it is decoded fully or mirrored over the full IO1 range is unknown
      (the software only uses $de00).
      - any access to io1 toggles the flipflop, which means enabling or disabling
        the cartridge RAM, ie ROMH (exrom) (?)
      - any access to io1 seems to disable write-access to the RAM

    the cartridge has a 3 way switch:

    PRG:

      - NMI logic and registers are disabled
      - RAM is mapped to 8000 (writeable) in 8k game mode

    ON:

    - after switching from OFF to ON
      - NMI logic is active. the "freezer" can now be activated by either
        pressing restore or the freezer button.
      - Io1 "register" logic is disabled
      - RAM not mapped

    - after reset:
      - RAM is mapped to 8000 (writeable) and E000 (read only) in ultimax mode
      - Io1 "register" logic is enabled (?)

    - on NMI (restore or freezer button pressed):
      - RAM is mapped to 8000 (writeable) and E000 (read only) in ultimax mode
      - Io1 "register" logic is enabled

    OFF:

      - NMI logic and registers are disabled
      - RAM not mapped
      - according to the documentation, the cartridge is disabled. however,
        the NMI logic of the cart still seems to interfer somehow and makes
        some program misbehave. the solution for this was to put an additional
        switch at the NMI line of the cartridge port, which then allows to
        completely disable the cartridge for real.

        this misbehavior is NOT emulated

    there also was an "upgrade" to the hardware at some point, called "EMS".
    this pretty much was no more no less than a freezer button :=)

    short instructions:

    note: the state of the switch and the order of doing things is very important,
          making a mistake here will almost certainly cause it not to work as
          expected. also, since the software tracks its state internally to act
          accordingly, saving it will not result in a runable image at any time.

    preparing the expert ram (skip this if you use a crt file)

    - switch to OFF
    - reset
    - switch to PRG
    - load and run the install software
    - switch to ON (as beeing told on screen)
    - reset. this will usually start some menu/intro screen
    - space to go to monitor

    using the freezer / monitor

    - "pp" or "p" clears memory (and will reset)
    - (if you want to save a crt file for later use, do that now)
    - switch to OFF
    - reset
    - load and run game
    - switch to ON
    - hit restore (or freezer button)
    - z "name" to save backup
    - r to restart running program
*/

/*
    notes from c64mem.c:
    - ROML is enabled at memory configs 11, 15, 27, 31 and Ultimax.
    - Allow writing at ROML at $8000-$9FFF.
    - Allow ROML being visible independent of charen, hiram & loram
    - Copy settings from "normal" operation mode into "ultimax" configuration.
*/

/*
this sequence from expert 2.10 indicates that a full ROM is available at E000
when the cartridge is ON, HIROM is selected.

it also indicates that the de00 register is a toggle

.C:038b   A9 37      LDA #$37
.C:038d   85 01      STA $01
.C:038f   AD 00 DE   LDA $DE00  toggle expert RAM on/off
.C:0392   AD BD FD   LDA $FDBD  fdbd is $00 in kernal rom
.C:0395   D0 F8      BNE $038F  do again if expert RAM not off

Reset entry from expert 2.70:

.C:9fd1   78         SEI
.C:9fd2   D8         CLD
.C:9fd3   A2 FF      LDX #$FF
.C:9fd5   9A         TXS
.C:9fd6   E8         INX
.C:9fd7   8E F2 9F   STX $9FF2  set to 0

.C:03b0   48         PHA
.C:03b1   A9 37      LDA #$37
.C:03b3   85 01      STA $01
.C:03b5   AD 00 DE   LDA $DE00  toggle expert RAM on/off
.C:03b8   AD 00 E0   LDA $E000  is $85 in kernal
.C:03bb   C9 85      CMP #$85
.C:03bd   D0 F6      BNE $03B5
.C:03bf   68         PLA
.C:03c0   60         RTS

NMI entry from expert 2.70:

.C:9c00   2C F2 9F   BIT $9FF2  get bit7 into N
.C:9c03   10 01      BPL $9C06  if N=0 branch
.C:9c05   40         RTI

.C:9c06   78         SEI
.C:9c07   8D 01 80   STA $8001
.C:9c0a   A9 FF      LDA #$FF
.C:9c0c   8D F2 9F   STA $9FF2
*/

/* #define DBGEXPERT */

#ifdef DBGEXPERT
#define DBG(x) printf x
char *expert_mode[3]={"off", "prg", "on"};
#else
#define DBG(x)
#endif

#define USEFAKEPRGMAPPING       1 /* emulate PRG mode as 8k game */

#if USEFAKEPRGMAPPING
#define EXPERT_PRG ((0 << 0) | (0 << 1)) /* 8k game */
#else
#define EXPERT_PRG ((1 << 0) | (1 << 1)) /* ultimax */
#endif
#define EXPERT_OFF ((0 << 0) | (1 << 1)) /* ram */
#define EXPERT_ON  ((1 << 0) | (1 << 1)) /* ultimax */

static int cartmode = EXPERT_MODE_DEFAULT;
static int expert_enabled = 0;
static int expert_register_enabled = 0;
static int expert_ram_writeable = 0;
static int expert_ramh_enabled = 0; /* equals EXROM ? */

/* 8 KB RAM */
static BYTE *expert_ram = NULL;

static char *expert_filename = NULL;
static int expert_filetype = 0;
static int expert_write_image = 0;

#define EXPERT_RAM_SIZE 8192

static const char STRING_EXPERT[] = CARTRIDGE_NAME_EXPERT;

static int expert_load_image(void);

/* ---------------------------------------------------------------------*/

BYTE expert_io1_read(WORD addr);
BYTE expert_io1_peek(WORD addr);
void expert_io1_store(WORD addr, BYTE value);

static io_source_t expert_io1_device = {
    CARTRIDGE_NAME_EXPERT,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xde01, 0xff,
    0, /* read is never valid */
    expert_io1_store,
    expert_io1_read,
    expert_io1_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_EXPERT,
    0,
    0
};

static const export_resource_t export_res = {
    CARTRIDGE_NAME_EXPERT, 1, 1, &expert_io1_device, NULL, CARTRIDGE_EXPERT
};

static io_source_list_t *expert_io1_list_item = NULL;

/* ---------------------------------------------------------------------*/
int expert_cart_enabled(void)
{
    if (expert_enabled) {
        return 1;
    }
    return 0;
}

static int expert_mode_changed(int mode, void *param)
{
    switch (mode) {
        case EXPERT_MODE_OFF:
        case EXPERT_MODE_PRG:
        case EXPERT_MODE_ON:
            break;
        default:
            return -1;
    }

    cartmode = mode;
    DBG(("EXPERT: expert_mode_changed cartmode: %d (%s)\n", cartmode, expert_mode[cartmode]));
    if (expert_enabled) {
        switch (mode) {
            case EXPERT_MODE_PRG:
                cart_config_changed_slot1(2, EXPERT_PRG, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
                expert_register_enabled = 1;
                expert_ramh_enabled = 0;
                expert_ram_writeable = 1;
                break;
            case EXPERT_MODE_ON:
                cart_config_changed_slot1(2, 2, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
                expert_register_enabled = 0;
                expert_ramh_enabled = 0;
                expert_ram_writeable = 0;
                break;
            case EXPERT_MODE_OFF:
                cart_config_changed_slot1(2, EXPERT_OFF, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
                expert_register_enabled = 0;
                expert_ramh_enabled = 0;
                expert_ram_writeable = 0;
                break;
        }
    }

    return 0;
}

static int expert_activate(void)
{
    if (expert_ram == NULL) {
        expert_ram = lib_malloc(EXPERT_RAM_SIZE);
    }

    if (!util_check_null_string(expert_filename)) {
        log_message(LOG_DEFAULT, "Reading Expert Cartridge image %s.", expert_filename);
        if (expert_load_image() < 0) {
            log_error(LOG_DEFAULT, "Reading Expert Cartridge image %s failed.", expert_filename);
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            expert_filetype = CARTRIDGE_FILETYPE_BIN;
            if (!util_file_exists(expert_filename)) {
                if (expert_flush_image() < 0) {
                    log_error(LOG_DEFAULT, "Creating Expert Cartridge image %s failed.", expert_filename);
                    return -1;
                }
            }
        }
    }

    return 0;
}

static int expert_deactivate(void)
{
    if (expert_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(expert_filename)) {
        if (expert_write_image) {
            log_message(LOG_DEFAULT, "Writing Expert Cartridge image %s.", expert_filename);
            if (expert_flush_image() < 0) {
                log_error(LOG_DEFAULT, "Writing Expert Cartridge image %s failed.", expert_filename);
            }
        }
    }

    lib_free(expert_ram);
    expert_ram = NULL;
    return 0;
}

static int set_expert_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("EXPERT: set enabled: %d:%d\n", expert_enabled, val));

    if (expert_enabled && !val) {
        DBG(("EXPERT: disable\n"));
        if (expert_deactivate() < 0) {
            return -1;
        }
        io_source_unregister(expert_io1_list_item);
        expert_io1_list_item = NULL;
        export_remove(&export_res);
        expert_enabled = 0;
        cart_power_off();
    } else if (!expert_enabled && val) {
        DBG(("EXPERT: enable\n"));
        if (expert_activate() < 0) {
            return -1;
        }
        expert_io1_list_item = io_source_register(&expert_io1_device);
        if (export_add(&export_res) < 0) {
            DBG(("EXPERT: set enabled: error\n"));
            io_source_unregister(expert_io1_list_item);
            expert_io1_list_item = NULL;
            expert_enabled = 0;
            return -1;
        }
        expert_enabled = 1;
        resources_set_int("ExpertCartridgeMode", cartmode);
        cart_power_off();
    }

    return 0;
}

static int set_expert_rw(int val, void *param)
{
    expert_write_image = val ? 1 : 0;

    return 0;
}

/* TODO: make sure setting filename works under all conditions */
static int set_expert_filename(const char *name, void *param)
{
    if (expert_filename != NULL && name != NULL && strcmp(name, expert_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (expert_enabled) {
        expert_deactivate();
    }
    util_string_set(&expert_filename, name);
    if (expert_enabled) {
        expert_activate();
    }

    return 0;
}

/* ---------------------------------------------------------------------*/

void expert_io1_store(WORD addr, BYTE value)
{
    DBG(("EXPERT: io1 wr %04x (%d)\n", addr, value));
    if ((cartmode == EXPERT_MODE_ON) && (expert_register_enabled == 1)) {
        cart_config_changed_slot1(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
        expert_ramh_enabled ^= 1;
        expert_ram_writeable = 0; /* =0 ? */
        DBG(("EXPERT: ON (regs: %d ramh: %d ramwrite: %d)\n", expert_register_enabled, expert_ramh_enabled, expert_ram_writeable));
    }
}

BYTE expert_io1_read(WORD addr)
{
    expert_io1_device.io_source_valid = 0;
    DBG(("EXPERT: io1 rd %04x (%d)\n", addr, expert_ramh_enabled));
    if ((cartmode == EXPERT_MODE_ON) && (expert_register_enabled == 1)) {
        cart_config_changed_slot1(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
        expert_ramh_enabled ^= 1;
        expert_ram_writeable = 0; /* =0 ? */
        DBG(("EXPERT: ON (regs: %d ramh: %d ramwrite: %d)\n", expert_register_enabled, expert_ramh_enabled, expert_ram_writeable));
    }
    return 0;
}

BYTE expert_io1_peek(WORD addr)
{
    return 0;
}

/* ---------------------------------------------------------------------*/

BYTE expert_roml_read(WORD addr)
{
/*    DBG(("EXPERT: set expert_roml_read: %x\n", addr)); */
    if (cartmode == EXPERT_MODE_PRG) {
        return expert_ram[addr & 0x1fff];
    } else if ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled) {
        return expert_ram[addr & 0x1fff];
    } else {
        return ram_read(addr);
    }
}

void expert_roml_store(WORD addr, BYTE value)
{
/*    DBG(("EXPERT: set expert_roml_store: %x\n", addr)); */
    if (expert_ram_writeable) {
        if (cartmode == EXPERT_MODE_PRG) {
            expert_ram[addr & 0x1fff] = value;
        } else if ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled) {
            expert_ram[addr & 0x1fff] = value;
        } else {
            /* mem_store_without_ultimax(addr, value); */
            ram_store(addr, value);
        }
    } else {
        ram_store(addr, value);
    }
}

BYTE expert_romh_read(WORD addr)
{
/*    DBG(("EXPERT: set expert_romh_read: %x mode %d %02x %02x\n", addr, cartmode, expert_ram[0x1ffe], expert_ram[0x1fff])); */
    if ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled) {
        return expert_ram[addr & 0x1fff];
    } else {
        return mem_read_without_ultimax(addr);
    }
}

int expert_romh_phi1_read(WORD addr, BYTE *value)
{
    if ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled) {
        *value = expert_ram[addr & 0x1fff];
        return CART_READ_VALID;
    }
    return CART_READ_C64MEM;
}

int expert_romh_phi2_read(WORD addr, BYTE *value)
{
    return expert_romh_phi1_read(addr, value);
}

int expert_peek_mem(WORD addr, BYTE *value)
{
    if (cartmode == EXPERT_MODE_PRG) {
        if ((addr >= 0x8000) && (addr <= 0x9fff)) {
            *value = expert_ram[addr & 0x1fff];
            return CART_READ_VALID;
        }
        /* return CART_READ_C64MEM; */
    } else if (cartmode == EXPERT_MODE_ON) {
        if ((addr >= 0x8000) && (addr <= 0x9fff)) {
            *value = expert_ram[addr & 0x1fff];
            return CART_READ_VALID;
        } else if (addr >= 0xe000) {
            if (expert_ramh_enabled) {
                *value = expert_ram[addr & 0x1fff];
                return CART_READ_VALID;
            }
        }
    }
    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

int expert_freeze_allowed(void)
{
    if (cartmode == EXPERT_MODE_ON) {
        return 1;
    }
    return 0;
}

void expert_freeze(void)
{
    if (cartmode == EXPERT_MODE_ON) {
        DBG(("EXPERT: freeze\n"));
        /* DBG(("ram %02x %02x\n", expert_ram[0x1ffe], expert_ram[0x1fff])); */
        cart_config_changed_slot1(2, EXPERT_ON, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
        expert_register_enabled = 1;
        expert_ram_writeable = 1;
        expert_ramh_enabled = 1;
    }
}

void expert_ack_nmi(void)
{
    if (cartmode == EXPERT_MODE_ON) {
        DBG(("EXPERT:ack nmi\n"));
        /* DBG(("ram %02x %02x\n", expert_ram[0x1ffe], expert_ram[0x1fff])); */
        cart_config_changed_slot1(2, EXPERT_ON, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
        expert_register_enabled = 1;
        expert_ram_writeable = 1;
        expert_ramh_enabled = 1;
    }
}

void expert_reset(void)
{
    DBG(("EXPERT: reset\n"));
    if (cartmode == EXPERT_MODE_ON) {
        expert_register_enabled = 1;
        expert_ram_writeable = 1;
        expert_ramh_enabled = 1;
        cart_config_changed_slot1(2, 3, CMODE_READ | CMODE_PHI2_RAM);
    } else if (cartmode == EXPERT_MODE_PRG) {
        expert_register_enabled = 1;
        expert_ram_writeable = 1;
        expert_ramh_enabled = 1;
        cart_config_changed_slot1(2, EXPERT_PRG, CMODE_READ);
    } else {
        expert_register_enabled = 0;
        expert_ram_writeable = 0;
        expert_ramh_enabled = 0;
        cart_config_changed_slot1(2, EXPERT_OFF, CMODE_READ | CMODE_PHI2_RAM);
    }
}

/* ---------------------------------------------------------------------*/

void expert_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xf000) {
        case 0xf000:
        case 0xe000:
            if ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled) {
                *base = expert_ram - 0xe000;
                *start = 0xe000;
                *limit = 0xfffd;
                return;
            }
            break;
        case 0x9000:
        case 0x8000:
            if ((cartmode == EXPERT_MODE_PRG)
                || ((cartmode == EXPERT_MODE_ON) && expert_ramh_enabled)) {
                *base = expert_ram - 0x8000;
                *start = 0x8000;
                *limit = 0x9ffd;
                return;
            }
            break;
        default:
            break;
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

void expert_config_init(void)
{
    if (expert_enabled) {
        DBG(("EXPERT: config_init cartmode: %d\n", cartmode));
        expert_reset();
        interrupt_set_nmi_trap_func(maincpu_int_status, expert_ack_nmi);
    }
}

void expert_config_setup(BYTE *rawcart)
{
    memcpy(expert_ram, rawcart, EXPERT_RAM_SIZE);
    /* DBG(("ram %02x %02x\n", expert_ram[0x1ffe], expert_ram[0x1fff])); */
}

/* ---------------------------------------------------------------------*/

const char *expert_get_file_name(void)
{
    return expert_filename;
}

static int expert_common_attach(void)
{
    DBG(("EXPERT: common attach\n"));
    if (resources_set_int("ExpertCartridgeEnabled", 1) < 0) {
        return -1;
    }
    if (expert_enabled) {
        /* Set default mode
        here we want to load a previously saved image. we use ON as
        default here.
        */
        resources_set_int("ExpertCartridgeMode", EXPERT_MODE_ON);
        DBG(("EXPERT: common attach ok\n"));
        return 0;
    }
    return -1;
}

static int expert_bin_load(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, EXPERT_RAM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    expert_filetype = CARTRIDGE_FILETYPE_BIN;
    return 0;
}

int expert_bin_attach(const char *filename, BYTE *rawcart)
{
    if (expert_bin_load(filename, rawcart) < 0) {
        return -1;
    }
    if (set_expert_filename(filename, NULL) < 0) {
        return -1;
    }
    return expert_common_attach();
}

int expert_bin_save(const char *filename)
{
    FILE *fd;

    if (expert_ram == NULL) {
        DBG(("EXPERT: ERROR expert_ram == NULL\n"));
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (fwrite(expert_ram, 1, EXPERT_RAM_SIZE, fd) != EXPERT_RAM_SIZE) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

static int expert_crt_load(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != EXPERT_RAM_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }
    expert_filetype = CARTRIDGE_FILETYPE_CRT;
    return 0;
}

int expert_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    if (expert_crt_load(fd, rawcart) < 0) {
        return -1;
    }
    if (set_expert_filename(filename, NULL) < 0) {
        return -1;
    }
    return expert_common_attach();
}

int expert_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;

    DBG(("EXPERT: save crt '%s'\n", filename));

    if (expert_ram == NULL) {
        DBG(("EXPERT: ERROR expert_ram == NULL\n"));
        return -1;
    }

    fd = crt_create(filename, CARTRIDGE_EXPERT, 1, 0, STRING_EXPERT);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;               /* Chip type. (= FlashROM?) */
    chip.bank = 0;               /* Bank nr. (= 0) */
    chip.start = 0x8000;         /* Address. (= 0x8000) */
    chip.size = EXPERT_RAM_SIZE; /* Length. (= 0x2000) */

    /* Write CHIP packet data. */
    if (crt_write_chip(expert_ram, &chip, fd)) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    DBG(("EXPERT: ERROR save crt ok\n"));

    return 0;
}

static int expert_load_image(void)
{
    int res = 0;
    FILE *fd;

    if (crt_getid(expert_filename) == CARTRIDGE_EXPERT) {
        fd = fopen(expert_filename, MODE_READ);
        res = expert_crt_load(fd, expert_ram);
        fclose(fd);
    } else {
        res = expert_bin_load(expert_filename, expert_ram);
    }
    return res;
}

int expert_flush_image(void)
{
    if (expert_filetype == CARTRIDGE_FILETYPE_BIN) {
        return expert_bin_save(expert_filename);
    } else if (expert_filetype == CARTRIDGE_FILETYPE_CRT) {
        return expert_crt_save(expert_filename);
    }
    return -1;
}

void expert_detach(void)
{
    resources_set_int("ExpertCartridgeEnabled", 0);
}

int expert_enable(void)
{
    DBG(("EXPERT: enable\n"));
    if (resources_set_int("ExpertCartridgeEnabled", 1) < 0) {
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

/* CARTEXPERT snapshot module format:

   type  | name            | description
   -------------------------------------
   BYTE  | mode            | cartridge mode
   BYTE  | register enable | register enable flag
   BYTE  | RAM writable    | RAM writable flag
   BYTE  | RAMH enable     | RAMH enable flag
   ARRAY | RAM             | 8192 BYTES of RAM data
 */

static char snap_module_name[] = "CARTEXPERT";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int expert_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)cartmode) < 0)
        || (SMW_B(m, (BYTE)expert_register_enabled) < 0)
        || (SMW_B(m, (BYTE)expert_ram_writeable) < 0)
        || (SMW_B(m, (BYTE)expert_ramh_enabled) < 0)
        || (SMW_BA(m, expert_ram, EXPERT_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int expert_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    expert_ram = lib_malloc(EXPERT_RAM_SIZE);

    if (0
        || (SMR_B_INT(m, &cartmode) < 0)
        || (SMR_B_INT(m, &expert_register_enabled) < 0)
        || (SMR_B_INT(m, &expert_ram_writeable) < 0)
        || (SMR_B_INT(m, &expert_ramh_enabled) < 0)
        || (SMR_BA(m, expert_ram, EXPERT_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(expert_ram);
        expert_ram = NULL;
        return -1;
    }

    snapshot_module_close(m);

    expert_filetype = 0;
    expert_write_image = 0;
    expert_enabled = 1;

    /* FIXME: ugly code duplication to avoid cart_config_changed calls */
    expert_io1_list_item = io_source_register(&expert_io1_device);

    if (export_add(&export_res) < 0) {
        lib_free(expert_ram);
        expert_ram = NULL;
        io_source_unregister(expert_io1_list_item);
        expert_io1_list_item = NULL;
        expert_enabled = 0;
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-expert", SET_RESOURCE, 0,
      NULL, NULL, "ExpertCartridgeEnabled", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_EXPERT_CART,
      NULL, NULL },
    { "+expert", SET_RESOURCE, 0,
      NULL, NULL, "ExpertCartridgeEnabled", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_EXPERT_CART,
      NULL, NULL },
    { "-expertimagename", SET_RESOURCE, 1,
      NULL, NULL, "Expertfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SET_EXPERT_FILENAME,
      NULL, NULL },
    { "-expertimagerw", SET_RESOURCE, 0,
      NULL, NULL, "ExpertImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_EXPERT_IMAGE,
      NULL, NULL },
    { "+expertimagerw", SET_RESOURCE, 0,
      NULL, NULL, "ExpertImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_EXPERT_IMAGE,
      NULL, NULL },
    { "-expertmode", SET_RESOURCE, 1,
      NULL, NULL, "ExpertCartridgeMode", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODE, IDCLS_SET_EXPERT_MODE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int expert_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static const resource_string_t resources_string[] = {
    { "Expertfilename", "", RES_EVENT_NO, NULL,
      &expert_filename, set_expert_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "ExpertCartridgeEnabled", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &expert_enabled, set_expert_enabled, NULL },
    { "ExpertCartridgeMode", EXPERT_MODE_DEFAULT, RES_EVENT_NO, NULL,
      &cartmode, expert_mode_changed, NULL },
    { "ExpertImageWrite", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &expert_write_image, set_expert_rw, NULL },
    RESOURCE_INT_LIST_END
};

int expert_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void expert_resources_shutdown(void)
{
    lib_free(expert_filename);
    expert_filename = NULL;
}
