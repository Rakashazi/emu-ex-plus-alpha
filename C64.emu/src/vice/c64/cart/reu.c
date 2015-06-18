/*! \file reu.c \n
 *  \author Andreas Boose, Spiro Trikaliotis, Jouko Valta, Richard Hable, Ettore Perazzoli\n
 *  \brief   REU emulation.
 *
 * reu.c - REU emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 *
 * Additions upon extensive REU hardware testing:
 *  Wolfgang Moser <http://d81.de>
 *
 * Based on old code by
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Richard Hable <K3027E7@edvz.uni-linz.ac.at>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "reu.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

#if 0
#define REU_DEBUG 1 /*!< define this if you want to get debugging output for the REU. */
#endif

/*! \brief the debug levels to use when REU_DEBUG is defined */
typedef enum {
    DEBUG_LEVEL_NONE = 0,              /*!< do not output debugging information */
    DEBUG_LEVEL_REGISTER,              /*!< output debugging information concerning the REU registers */
    DEBUG_LEVEL_REGISTER2,             /*!< more register stuff */
    DEBUG_LEVEL_TRANSFER_HIGH_LEVEL,   /*!< output debugging information on transfers, on a high-level (per operation) */
    DEBUG_LEVEL_NO_DRAM,               /*!< output debugging information whenever an address is accessed where no DRAM is available  */
    DEBUG_LEVEL_TRANSFER_LOW_LEVEL     /*!< output debugging information on transfers, on a low-level (per single byte) */
} debug_level_e;

#ifdef REU_DEBUG
/*! \brief dynamically define the debugging level */
static debug_level_e DEBUG_LEVEL = DEBUG_LEVEL_TRANSFER_LOW_LEVEL;

/*! \brief output debugging information
  \param _level
     The debugging level on which this data appears

  \param _x
    The complete log_message parameter, including the braces
*/
    #define DEBUG_LOG( _level, _x ) do { if (_level <= (DEBUG_LEVEL)) { log_message _x; } \
} \
    while (0)

#else

/*! \brief output debugging information
  \param _level
     The debugging level on which this data appears

  \param _x
    The complete log_message parameter, including the braces

  \remark
    This implementation is the dummy if debugging output is disabled
*/
    #define DEBUG_LOG( _level, _x )

#endif

/*! \brief Shortcut to check for masked bits being all set */
#define BITS_ARE_ALL_SET(_where, _bits) ((((_where) & (_bits)) == (_bits)))

/*! \brief Shortcut to check for masked bits being all cleared */
#define BITS_ARE_ALL_UNSET(_where, _bits) ((((_where) & (_bits)) == 0))

/*
 * Status and Command Registers
 * bit  7       6       5       4       3       2       1       0
 * 00   Int     EOB     Fault   RamSize ________ Version ________
 * 01   Exec    0       Load    Delayed 0       0          Mode
 */

/*! \brief Offsets of the different REU registers */
enum {
    REU_REG_R_STATUS         = 0x00, /*!< the REU status register */
    REU_REG_RW_COMMAND       = 0x01, /*!< the REU command register */
    REU_REG_RW_BASEADDR_LOW  = 0x02, /*!< the REU base low address register (computer side) */
    REU_REG_RW_BASEADDR_HIGH = 0x03, /*!< the REU base high address register (computer side) */
    REU_REG_RW_RAMADDR_LOW   = 0x04, /*!< the REU RAM low address register (expansion side) */
    REU_REG_RW_RAMADDR_HIGH  = 0x05, /*!< the REU RAM high address register (expansion side) */
    REU_REG_RW_BANK          = 0x06, /*!< the REU RAM bank address register (expansion side) */
    REU_REG_RW_BLOCKLEN_LOW  = 0x07, /*!< the REU transfer length low register */
    REU_REG_RW_BLOCKLEN_HIGH = 0x08, /*!< the REU transfer length high register */
    REU_REG_RW_INTERRUPT     = 0x09, /*!< the REU interrupt register */
    REU_REG_RW_ADDR_CONTROL  = 0x0A, /*!< the REU address register */
    REU_REG_RW_UNUSED        = 0x0B, /*!< the first unused REU address. The unused area fills up to REU_REG_LAST_REG */
    REU_REG_LAST_REG         = 0x1F  /*!< the last register of the REU */
};

/*! \brief bit definitions for the REU status register at offset REU_REG_R_STATUS */
enum {
    REU_REG_R_STATUS_CHIPVERSION_MASK  = 0x0F, /*!< bit mask the extract the chip version no. */
    REU_REG_R_STATUS_256K_CHIPS        = 0x10, /*!< set if 256K DRAMs (256Kx1) are used (1764, 1750), if unset, 64K DRAMs (64Kx1) are used (1700) */
    REU_REG_R_STATUS_VERIFY_ERROR      = 0x20, /*!< set if an verify error occurred. Cleared on read. */
    REU_REG_R_STATUS_END_OF_BLOCK      = 0x40, /*!< set of the operation ended. Cleared on read. */
    REU_REG_R_STATUS_INTERRUPT_PENDING = 0x80  /*!< set if an interrupt is pending. Cleared on read. */
};

/*! \brief bit definitions for the REU command register at offset REU_REG_RW_COMMAND */
enum {
    REU_REG_RW_COMMAND_TRANSFER_TYPE_MASK    = 0x03,    /*!< bit mask to extract the transfer type */
    REU_REG_RW_COMMAND_TRANSFER_TYPE_TO_REU     = 0x00, /*!< transfer type is C64 -> REU */
    REU_REG_RW_COMMAND_TRANSFER_TYPE_FROM_REU   = 0x01, /*!< transfer type is REU -> C64 */
    REU_REG_RW_COMMAND_TRANSFER_TYPE_SWAP       = 0x02, /*!< transfer type is swap between C64 and REU */
    REU_REG_RW_COMMAND_TRANSFER_TYPE_VERIFY     = 0x03, /*!< transfer type is verify between C64 and REU */
    REU_REG_RW_COMMAND_RESERVED_MASK         = 0x4C,    /*!< the bits defined here are writeable, but unused */
    REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED = 0x10,    /*!< if set, $FF00 trigger is disabled. */
    REU_REG_RW_COMMAND_AUTOLOAD              = 0x20,    /*!< if set, the address registers should be autoloaded after an operation */
    REU_REG_RW_COMMAND_EXECUTE               = 0x80     /*!< is set, the specified operation should start. */
};

/*! \brief bit definitions for the REU bank register at offset REU_REG_RW_BANK */
enum {
    REU_REG_RW_BANK_UNUSED = 0xF8  /*!< these bits are unused and always read as 1 */
};

/*! \brief bit definitions for the REU interrupt register at offset REU_REG_RW_INTERRUPT */
enum {
    REU_REG_RW_INTERRUPT_UNUSED_MASK          = 0x1F, /*!< these bits are unused and always read as 1 */
    REU_REG_RW_INTERRUPT_VERIFY_ENABLED       = 0x20, /*!< if set (and REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED is set, too), generate an interrupt if verify fails */
    REU_REG_RW_INTERRUPT_END_OF_BLOCK_ENABLED = 0x40, /*!< if set (and REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED is set, too), generate an interrupt if operation finished */
    REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED   = 0x80  /*!< is set, the REU can generate an interrupt. If unset, no interrupts can be generated */
};

/*! \brief bit definitions for the REU address control register at offset REU_REG_RW_ADDR_CONTROL */
enum {
    REU_REG_RW_ADDR_CONTROL_UNUSED_MASK = 0x3f, /*!< these bits are unused and always read as 1 */
    REU_REG_RW_ADDR_CONTROL_FIX_REC     = 0x40, /*!< if set, the REU address is fixed, it does not increment */
    REU_REG_RW_ADDR_CONTROL_FIX_C64     = 0x80  /*!< if set, the C64 address is fixed, it does not increment */
};

/* REU registers */

/*! \brief define a complete set of REC registers */
struct rec_s {
    BYTE status;              /*!< status register at offset REU_REG_R_STATUS */
    BYTE command;             /*!< command register at offset REU_REG_RW_COMMAND */

    WORD base_computer;       /*!< C64 base address as defined at offsets REU_REG_RW_BASEADDR_LOW and REU_REG_RW_BASEADDR_HIGH */
    WORD base_reu;            /*!< REU base address as defined at offsets REU_REG_RW_RAMADDR_LOW and REU_REG_RW_RAMADDR_HIGH */
    BYTE bank_reu;            /*!< REU bank address as defined at offset REU_REG_RW_BANK */
    WORD transfer_length;     /*!< transfer length as defined at offsets REU_REG_RW_BLOCKLEN_LOW and REU_REG_RW_BLOCKLEN_HIGH */

    BYTE int_mask_reg;        /*! interrupt mask register as defined at offset REU_REG_RW_INTERRUPT */
    BYTE address_control_reg; /*! address control register as defined at offset REU_REG_RW_ADDR_CONTROL */

    /* shadow registers for implementing the "Half-Autoload-Bug" */

    WORD base_computer_shadow;   /*!< shadow register of base_computer */
    WORD base_reu_shadow;        /*!< shadow register of base_reu */
    BYTE bank_reu_shadow;        /*!< shadow register of bank_reu */
    WORD transfer_length_shadow; /*!< shadow register of transfer_length */
};

/*! \brief a complete REC description */
static struct rec_s rec;

/*! \brief some rec options which define special behaviour */
struct rec_options_s {
    unsigned int wrap_around;                   /*!< address where the REU has a wrap around (usually 512k, 1700 is special) */
    unsigned int dram_wrap_around;              /*!< address where the dram address space has a wrap around */
    unsigned int not_backedup_addresses;        /*!< beginning from this address up to wrap_around, there is no DRAM at all */
    unsigned int wrap_around_mask_when_storing; /*!< mask for the wrap around of REU address when putting result back in base_reu and bank_reu */
    BYTE reg_bank_unused;                       /*!< the unused bits (stuck at 1) of REU_REG_RW_BANK; for original REU, it is REU_REG_RW_BANK_UNUSED */
    BYTE status_preset;                         /*!< preset value for the status (can be 0 or REU_REG_R_STATUS_256K_CHIPS) */
};

#define REU_REG_FIRST_UNUSED REU_REG_RW_UNUSED  /*!< the highest address the used REU register occupy */

/*! \brief a complete REC options description */
static struct rec_options_s rec_options;

/*! \brief flag for DMA active */
static int reu_dma_active = 0;

/*! \brief pointer to a buffer which holds the REU image.  */
static BYTE *reu_ram = NULL;
/*! \brief the old ram size of reu_ram. Used to determine if and how much of the
    buffer has to cleared when resizing the REU. */
static unsigned int old_reu_ram_size = 0;

static log_t reu_log = LOG_ERR; /*!< the log output for the REU */

static int reu_activate(void);
static int reu_deactivate(void);

static unsigned int reu_int_num;

/*! \brief interface for BA interaction with CPU & VICII, used for x64sc */
struct reu_ba_s {
    reu_ba_check_callback_t *check;
    reu_ba_steal_callback_t *steal;
    int *cpu_ba;
    int cpu_ba_mask;
    int enabled;
};

static struct reu_ba_s reu_ba = {
    NULL, NULL, NULL, 0, 0
};

static int reu_write_image = 0;

/* ------------------------------------------------------------------------- */

/* some prototypes are needed */
static void reu_io2_store(WORD addr, BYTE byte);
static BYTE reu_io2_read(WORD addr);
static BYTE reu_io2_peek(WORD addr);

static io_source_t reu_io2_device = {
    CARTRIDGE_NAME_REU,
    IO_DETACH_RESOURCE,
    "REU",
    0xdf00, 0xdfff, REU_REG_LAST_REG,
    0,
    reu_io2_store,
    reu_io2_read,
    reu_io2_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_REU,
    IO_PRIO_HIGH, /* high priority so it will work together with cartridges like RR and SSV5 */
    0
};

static io_source_list_t *reu_list_item = NULL;

static const c64export_resource_t export_res_reu = {
    CARTRIDGE_NAME_REU, 0, 0, NULL, &reu_io2_device, CARTRIDGE_REU
};

/* ------------------------------------------------------------------------- */

/*! \brief Flag: Is the external REU enabled?  */
static int reu_enabled = 0;

/*! \brief Size of the REU.  */
static unsigned int reu_size = 0;

/*! \brief Size of the REU in KB.  */
static int reu_size_kb = 0;

/*! \brief Filename of the REU image.  */
static char *reu_filename = NULL;

int reu_cart_enabled(void)
{
    return reu_enabled;
}

/*! \internal \brief set the reu to the enabled or disabled state

 \param val
   if 0, disable the REU; else, enable it.

 \param param
   unused

 \return
   0 on success. else -1.
*/
static int set_reu_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((!val) && (reu_enabled)) {
        if (reu_deactivate() < 0) {
            return -1;
        }
        c64export_remove(&export_res_reu);
        io_source_unregister(reu_list_item);
        reu_list_item = NULL;
        reu_enabled = 0;
    } else if ((val) && (!reu_enabled)) {
        if (reu_activate() < 0) {
            return -1;
        }
        if (c64export_add(&export_res_reu) < 0) {
            return -1;
        }
        reu_list_item = io_source_register(&reu_io2_device);
        reu_enabled = 1;
    }
    return 0;
}

/*! \internal \brief set the size of the reu

 \param val
   the size of the REU, in KB

 \param param
   unused

 \return
   0 on success, else -1.

 \remark
   val must be one of 128, 256, 512, 1024, 2048, 4096, 8192, or 16384.
*/
static int set_reu_size(int val, void *param)
{
    if (val == reu_size_kb) {
        return 0;
    }

    switch (val) {
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
        case 8192:
        case 16384:
            break;
        default:
            log_message(reu_log, "Unknown REU size %d.", val);
            return -1;
    }

    if (reu_enabled) {
        reu_deactivate();
    }

    reu_size_kb = val;
    reu_size = reu_size_kb << 10;

    rec_options.wrap_around = 0x80000; /* always 512k, except 1700 */
    rec_options.dram_wrap_around = rec_options.wrap_around;
    rec_options.not_backedup_addresses = reu_size;
    rec_options.wrap_around_mask_when_storing = rec_options.wrap_around - 1;

    rec_options.reg_bank_unused = REU_REG_RW_BANK_UNUSED;
    rec_options.status_preset = REU_REG_R_STATUS_256K_CHIPS;

    switch (val) {
        case 128: /* Commodore 1700 */
            rec_options.status_preset = 0; /* we do not have 256K chips, but only 64K chips */
            /* the 1700 has a special wrap around, mimic that one */
            rec_options.wrap_around = 0x20000;
            rec_options.dram_wrap_around = 0x20000;
            break;
        case 256: /* Commodore 1764 */
        case 512: /* Commodore 1750 */
            break;
        /*
            note: the only real REU > 512kb that existed was the CMD 1750XL, which
                  shows the "wraparound bug" behaviour.
                  also the 1541U implements this starting with fw2.0, and the same
                  is true for the chameleon.
            "hacked REU" > 512k generally works like this.
            - the upper 5 bits of the banking register are added (in form of a latch)
              and are directly connected to the upper adresslines of the dram. that
              means these bits are never affected by what happens inside the REC chip.
            - the lower bits go the REC chip just like in a smaller REU
        */
        case 1024:
        case 2048: /* CMD 1750XL */
        case 4096:
        case 8192:
        case 16384:
            rec_options.reg_bank_unused = 0;
            rec_options.wrap_around_mask_when_storing = 0x00ffffff;
            rec_options.dram_wrap_around = 0x01000000;
            break;
        default:
            log_message(reu_log, "Unknown REU size %d.", val);
            return -1;
    }

    if (reu_enabled) {
        reu_activate();
    }

    return 0;
}

/*! \internal \brief set the file name of the REU data

 \param name
   pointer to a buffer which holds the file name.
   If NULL, the REU data will not be backed on the disk.

 \param param
   unused

 \return
   0 on success, else -1.

 \remark
   The file name of the REU data is the name of the file which is
   used to store the REU data onto disk.
*/
static int set_reu_filename(const char *name, void *param)
{
    if (reu_filename != NULL && name != NULL && strcmp(name, reu_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (reu_enabled) {
        reu_deactivate();
    }
    util_string_set(&reu_filename, name);

    if (reu_enabled) {
        reu_activate();
    }

    return 0;
}

static int set_reu_image_write(int val, void *param)
{
    reu_write_image = val ? 1 : 0;

    return 0;
}

/*! \brief string resources used by the REU module */
static const resource_string_t resources_string[] = {
    { "REUfilename", "", RES_EVENT_NO, NULL,
      &reu_filename, set_reu_filename, NULL },
    { NULL }
};

/*! \brief integer resources used by the REU module */
static const resource_int_t resources_int[] = {
    { "REUImageWrite", 0, RES_EVENT_NO, NULL,
      &reu_write_image, set_reu_image_write, NULL },
    { "REUsize", 512, RES_EVENT_NO, NULL,
      &reu_size_kb, set_reu_size, NULL },
    /* keeping "enable" resource last prevents unnecessary (re)init when loading config file */
    { "REU", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &reu_enabled, set_reu_enabled, NULL },
    { NULL }
};

/*! \brief initialize the reu resources
 \return
   0 on success, else -1.

 \remark
   Registers the string and the integer resources
*/
int reu_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*! \brief uninitialize the reu resources */
void reu_resources_shutdown(void)
{
    lib_free(reu_filename);
    reu_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-reu", SET_RESOURCE, 0,
      NULL, NULL, "REU", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_REU,
      NULL, NULL },
    { "+reu", SET_RESOURCE, 0,
      NULL, NULL, "REU", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_REU,
      NULL, NULL },
    { "-reusize", SET_RESOURCE, 1,
      NULL, NULL, "REUsize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_SIZE_IN_KB, IDCLS_REU_SIZE,
      NULL, NULL },
    { "-reuimage", SET_RESOURCE, 1,
      NULL, NULL, "REUfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_REU_NAME,
      NULL, NULL },
    { "-reuimagerw", SET_RESOURCE, 0,
      NULL, NULL, "REUImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_REU_IMAGE,
      NULL, NULL },
    { "+reuimagerw", SET_RESOURCE, 0,
      NULL, NULL, "REUImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_REU_IMAGE,
      NULL, NULL },
    { NULL }
};

/*! \brief initialize the command-line options'
 \return
   0 on success, else -1.

 \remark
   Registers the command-line options
*/
int reu_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

const char *reu_get_file_name(void)
{
    return reu_filename;
}

/*! \brief initialize the REU */
void reu_init(void)
{
    reu_log = log_open("REU");

    reu_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "REU");
}

void reu_config_setup(BYTE *rawcart)
{
    if (reu_size > 0) {
        memcpy(reu_ram, rawcart, reu_size); /* FIXME */
    }
}

/*! \brief register the BA low interface */
void reu_ba_register(reu_ba_check_callback_t *ba_check,
                     reu_ba_steal_callback_t *ba_steal,
                     int *ba_var, int ba_mask)
{
    reu_ba.check = ba_check;
    reu_ba.steal = ba_steal;
    reu_ba.cpu_ba = ba_var;
    reu_ba.cpu_ba_mask = ba_mask;
    reu_ba.enabled = 1;
}

/*! \brief reset the REU */
void reu_reset(void)
{
    memset(&rec, 0, sizeof rec);

    rec.status = (rec.status & ~REU_REG_R_STATUS_256K_CHIPS) | rec_options.status_preset;

    rec.command = REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED;

    rec.transfer_length = rec.transfer_length_shadow = 0xffff;

    rec.bank_reu = rec.bank_reu_shadow = rec_options.reg_bank_unused;

    rec.int_mask_reg = REU_REG_RW_INTERRUPT_UNUSED_MASK;

    rec.address_control_reg = REU_REG_RW_ADDR_CONTROL_UNUSED_MASK;
}

static int reu_activate(void)
{
    if (!reu_size) {
        return 0;
    }

    reu_ram = lib_realloc(reu_ram, reu_size);

    /* Clear newly allocated RAM.  */
    if (reu_size > old_reu_ram_size) {
        memset(reu_ram, 0, (size_t)(reu_size - old_reu_ram_size));
    }

    old_reu_ram_size = reu_size;

    log_message(reu_log, "%dKB unit installed.", reu_size >> 10);

    if (!util_check_null_string(reu_filename)) {
        if (util_file_load(reu_filename, reu_ram, (size_t)reu_size, UTIL_FILE_LOAD_RAW) < 0) {
            log_error(reu_log, "Reading REU image %s failed.", reu_filename);
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            if (!util_file_exists(reu_filename)) {
                if (util_file_save(reu_filename, reu_ram, reu_size) < 0) {
                    log_error(reu_log, "Creating REU image %s failed.", reu_filename);
                    return -1;
                }
                log_message(reu_log, "Creating REU image %s.", reu_filename);
            }
            return 0;
        }
        log_message(reu_log, "Reading REU image %s.", reu_filename);
    }

    reu_reset();
    return 0;
}

static int reu_deactivate(void)
{
    if (reu_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(reu_filename)) {
        if (reu_write_image) {
            log_message(reu_log, "Writing REU image %s.", reu_filename);
            if (reu_flush_image() < 0) {
                log_error(reu_log, "Writing REU image %s failed.", reu_filename);
            }
        }
    }

    lib_free(reu_ram);
    reu_ram = NULL;
    old_reu_ram_size = 0;

    return 0;
}

/* detach the reu from the cartridge port */
void reu_detach(void)
{
    set_reu_enabled(0, NULL);
    reu_deactivate();
}

int reu_enable(void)
{
    return set_reu_enabled(1, NULL);
}

int reu_bin_attach(const char *filename, BYTE *rawcart)
{
    FILE *fd;
    int size;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    size = util_file_length(fd);
    fclose(fd);

    if (set_reu_size(size / 1024, NULL) < 0) {
        return -1;
    }

    if (set_reu_filename(filename, NULL) < 0) {
        return -1;
    }

    if (util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return reu_enable();
}

int reu_bin_save(const char *filename)
{
    if (reu_ram == NULL) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (util_file_save(filename, reu_ram, reu_size) < 0) {
        return -1;
    }

    return 0;
}

int reu_flush_image(void)
{
    if (reu_bin_save(reu_filename) < 0) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* helper functions */

/*! \brief clock handling for x64 */
inline static void reu_clk_inc_pre(void)
{
    if (!reu_ba.enabled) {
        maincpu_clk++;
    }
}

/*! \brief clock handling for x64sc */
inline static void reu_clk_inc_post(void)
{
    if (reu_ba.enabled) {
        maincpu_clk++;
        if (reu_ba.check()) {
            reu_ba.steal();
        }
    }
}

/*! \brief read the REU register values without side effects
  This function reads the REU values, so they can be accessed like
  an array of bytes. No side-effects that would be performed if a real
  read access would occur are executed.

  \param addr
    The address of the REC register to read

  \return
    The value the register has

  \remark
    address must be in the valid range 0..0x1f
*/
static BYTE reu_read_without_sideeffects(WORD addr)
{
    BYTE retval = 0xff;

    switch (addr) {
        case REU_REG_R_STATUS:
            retval = rec.status;
            break;
        case REU_REG_RW_COMMAND:
            retval = rec.command;
            break;
        case REU_REG_RW_BASEADDR_LOW:
            retval = rec.base_computer & 0xff;
            break;
        case REU_REG_RW_BASEADDR_HIGH:
            retval = (rec.base_computer >> 8) & 0xff;
            break;
        case REU_REG_RW_RAMADDR_LOW:
            retval = rec.base_reu & 0xff;
            break;
        case REU_REG_RW_RAMADDR_HIGH:
            retval = (rec.base_reu >> 8) & 0xff;
            break;
        case REU_REG_RW_BANK:
            retval = rec.bank_reu | rec_options.reg_bank_unused;
            break;
        case REU_REG_RW_BLOCKLEN_LOW:
            retval = rec.transfer_length & 0xff;
            break;
        case REU_REG_RW_BLOCKLEN_HIGH:
            retval = (rec.transfer_length >> 8) & 0xff;
            break;
        case REU_REG_RW_INTERRUPT:
            assert(BITS_ARE_ALL_SET(rec.int_mask_reg, REU_REG_RW_INTERRUPT_UNUSED_MASK));
            retval = rec.int_mask_reg;
            break;
        case REU_REG_RW_ADDR_CONTROL:
            assert(BITS_ARE_ALL_SET(rec.address_control_reg, REU_REG_RW_ADDR_CONTROL_UNUSED_MASK));
            retval = rec.address_control_reg;
            break;
    }

    return retval;
}

/*! \brief write the REU register values without side effects
  This function writes the REU values, so they can be accessed like
  an array of bytes. No side-effects that would be performed if a real
  write access would occur are executed.

  \param addr
    The address of the REC register to write

  \param byte
    The value to set the register to

  \remark
    address must be in the valid range 0..0x1f
*/
static void reu_store_without_sideeffects(WORD addr, BYTE byte)
{
    switch (addr) {
        case REU_REG_R_STATUS:
            /* REC status register is Read Only */
            break;
        case REU_REG_RW_COMMAND:
            rec.command = byte;
            break;
        case REU_REG_RW_BASEADDR_LOW:
            rec.base_computer = rec.base_computer_shadow = (rec.base_computer_shadow & 0xff00) | byte;
            break;
        case REU_REG_RW_BASEADDR_HIGH:
            rec.base_computer = rec.base_computer_shadow = (rec.base_computer_shadow & 0xff) | (byte << 8);
            break;
        case REU_REG_RW_RAMADDR_LOW:
            rec.base_reu = rec.base_reu_shadow = (rec.base_reu_shadow & 0xff00) | byte;
            break;
        case REU_REG_RW_RAMADDR_HIGH:
            rec.base_reu = rec.base_reu_shadow = (rec.base_reu_shadow & 0xff) | (byte << 8);
            break;
        case REU_REG_RW_BANK:
            rec.bank_reu = rec.bank_reu_shadow = byte & ~rec_options.reg_bank_unused;
            break;
        case REU_REG_RW_BLOCKLEN_LOW:
            rec.transfer_length = rec.transfer_length_shadow = (rec.transfer_length_shadow & 0xff00) | byte;
            break;
        case REU_REG_RW_BLOCKLEN_HIGH:
            rec.transfer_length = rec.transfer_length_shadow = (rec.transfer_length_shadow & 0xff) | (byte << 8);
            break;
        case REU_REG_RW_INTERRUPT:
            rec.int_mask_reg = byte | REU_REG_RW_INTERRUPT_UNUSED_MASK;
            break;
        case REU_REG_RW_ADDR_CONTROL:
            rec.address_control_reg = byte | REU_REG_RW_ADDR_CONTROL_UNUSED_MASK;
            break;
        default:
            break;
    }
}

/* ------------------------------------------------------------------------- */

/*! \brief read the REU register values
  This function is used to read the REU values from the computer.
  All side-effects are executed.

  \param addr
    The address of the REC register to read

  \return
    The value the register has
*/
static BYTE reu_io2_read(WORD addr)
{
    BYTE retval = 0xff;

    if (reu_dma_active) {
        reu_io2_device.io_source_valid = 0;
        return 0;
    } else {
        reu_io2_device.io_source_valid = 1;
    }

    if (addr < REU_REG_FIRST_UNUSED) {
        retval = reu_read_without_sideeffects(addr);

        switch (addr) {
            case REU_REG_R_STATUS:
                /* Bits 7-5 are cleared when register is read, and pending IRQs are
                   removed. */
                rec.status &= ~(REU_REG_R_STATUS_VERIFY_ERROR | REU_REG_R_STATUS_END_OF_BLOCK | REU_REG_R_STATUS_INTERRUPT_PENDING);

                maincpu_set_irq(reu_int_num, 0);
                break;
            default:
                break;
        }

        DEBUG_LOG(DEBUG_LEVEL_REGISTER, (reu_log, "read [$%02X] => $%02X.", addr, retval));
    }

    return retval;
}

static BYTE reu_io2_peek(WORD addr)
{
    BYTE retval = 0xff;
    if (addr < REU_REG_FIRST_UNUSED) {
        retval = reu_read_without_sideeffects(addr);
    }
    return retval;
}

/*! \brief write the REU register values
  This function is used to write the REU values from the computer.

  \param addr
    The address of the REC register to write

  \param byte
    The value to set the register to
*/
static void reu_io2_store(WORD addr, BYTE byte)
{
    if (!reu_dma_active && (addr < REU_REG_FIRST_UNUSED)) {
        reu_store_without_sideeffects(addr, byte);

        DEBUG_LOG( DEBUG_LEVEL_REGISTER, (reu_log, "store [$%02X] <= $%02X.", addr, (int)byte));

        switch (addr) {
            case REU_REG_RW_COMMAND:
                /* write REC command register
                 * DMA only if execution bit (7) set  - RH */
                if (BITS_ARE_ALL_SET(rec.command, REU_REG_RW_COMMAND_EXECUTE)) {
                    reu_dma(rec.command & REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED);
                }
                break;
            case REU_REG_RW_INTERRUPT:
                if (BITS_ARE_ALL_SET(rec.int_mask_reg, REU_REG_RW_INTERRUPT_END_OF_BLOCK_ENABLED | REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED) &&
                    BITS_ARE_ALL_SET(rec.status, REU_REG_R_STATUS_END_OF_BLOCK)) {
                    DEBUG_LOG( DEBUG_LEVEL_REGISTER, (reu_log, "End-of-transfer interrupt pending"));
                    rec.status |= REU_REG_R_STATUS_INTERRUPT_PENDING;
                    maincpu_set_irq(reu_int_num, 1);
                }
                if (BITS_ARE_ALL_SET(rec.int_mask_reg, REU_REG_RW_INTERRUPT_VERIFY_ENABLED | REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED) &&
                    BITS_ARE_ALL_SET(rec.status, REU_REG_R_STATUS_VERIFY_ERROR)) {
                    DEBUG_LOG( DEBUG_LEVEL_REGISTER, (reu_log, "Verify interrupt pending"));
                    rec.status |= REU_REG_R_STATUS_INTERRUPT_PENDING;
                    maincpu_set_irq(reu_int_num, 1);
                }
                break;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------------------- */

/*! \brief increment the reu address, taking wrap around into account
  This function increments the lower 19bits of the reu address (ie the range
  adressed by the REC chip) by the specified step.
  If a wrap around should occur (usually at 512k), perform it, too.

  \param reu_addr
     The address to be incremented

  \param reu_step
     The increment. Must be either 0 or 1. If 0, reu_addr is not changed at all.

  \return
     The incremented reu_addr, taking into account the wrap-around
*/
inline static unsigned int increment_reu_with_wrap_around(unsigned int reu_addr, unsigned int reu_step)
{
    unsigned int next = (reu_addr & 0x0007ffff) + reu_step;
    assert(((reu_step == 0) || (reu_step == 1)));

    if (next == rec_options.wrap_around) {
        next = 0;
    }

    return (reu_addr & 0x00f80000) | next;
}

/*! \brief store a value into the REU
  This function stores a byte value into the specified location of the REU.
  It takes into account addresses of the REU not backed up by DRAM.

  \param reu_addr
     The REU address where to store the value

  \param value
     The value to write into the REU.

  \remark
     If the location reu_addr is not backed up by DRAM, the store is simply
     ignored.
*/
inline static void store_to_reu(unsigned int reu_addr, BYTE value)
{
    reu_addr &= rec_options.dram_wrap_around - 1;
    if (reu_addr < rec_options.not_backedup_addresses) {
        assert(reu_addr < reu_size);
        reu_ram[reu_addr] = value;
    } else {
        DEBUG_LOG(DEBUG_LEVEL_NO_DRAM, (reu_log, "--> writing to REU address %05X, but no DRAM!", reu_addr));
    }
}

/*! \brief read a value from the REU
  This function reads a byte value from the specified location of the REU.
  It takes into account addresses of the REU not backed up by DRAM.

  \param reu_addr
     The REU address where to read the value from

  \remark value
     The value read from the REU.

  \remark
     If the location reu_addr is not backed up by DRAM, a dummy
     value is returned.

  \todo
     Check the values a real 17xx returns.
*/
inline static BYTE read_from_reu(unsigned int reu_addr)
{
    BYTE value = 0xff; /* dummy value to return if not DRAM is available */

    reu_addr &= rec_options.dram_wrap_around - 1;
    if (reu_addr < rec_options.not_backedup_addresses) {
        assert(reu_addr < reu_size);
        value = reu_ram[reu_addr];
    } else {
        DEBUG_LOG(DEBUG_LEVEL_NO_DRAM, (reu_log, "--> read from REU address %05X, but no DRAM!", reu_addr));
    }

    return value;
}

/* ------------------------------------------------------------------------- */

/*! \brief update the REU registers after a DMA operation

  \param host_addr
    The host (computer) address the operation stopped at

  \param reu_addr
    The REU address the operation stopped at

  \param len
    The transfer length the operation stopped at

  \param new_status_or_mask
    A mask which is used to set bits in the status

  \remark
    if autoload is enabled, the shadow registers are written back
    to the REU registers.
*/
static void reu_dma_update_regs(WORD host_addr, unsigned int reu_addr, int len, BYTE new_status_or_mask)
{
    assert(len >= 1);
    assert(new_status_or_mask != 0);

    reu_addr &= rec_options.wrap_around_mask_when_storing;

    rec.status |= new_status_or_mask;

    if (!(rec.command & REU_REG_RW_COMMAND_AUTOLOAD)) {
        /* not autoload
         * incr. of addr. disabled, as already pointing to correct addr.
         * address changes only if not fixed, correct reu base registers  -RH
         */
        DEBUG_LOG(DEBUG_LEVEL_REGISTER2, (reu_log, "No autoload."));
        if (BITS_ARE_ALL_UNSET(rec.address_control_reg, REU_REG_RW_ADDR_CONTROL_FIX_C64)) {
            rec.base_computer = host_addr;
        }

        if (BITS_ARE_ALL_UNSET(rec.address_control_reg, REU_REG_RW_ADDR_CONTROL_FIX_REC)) {
            rec.base_reu = reu_addr & 0xffff;
            rec.bank_reu = (reu_addr >> 16) & 0xff;
        }

        rec.transfer_length = len & 0xFFFF;
    } else {
        rec.base_computer = rec.base_computer_shadow;
        rec.base_reu = rec.base_reu_shadow;
        rec.bank_reu = rec.bank_reu_shadow;
        rec.transfer_length = rec.transfer_length_shadow;

        DEBUG_LOG(DEBUG_LEVEL_REGISTER, (reu_log, "Autoload."));
    }

    if (BITS_ARE_ALL_SET(new_status_or_mask, REU_REG_R_STATUS_END_OF_BLOCK)) {
        /* only check for interrupt, when the transfer ended correctly (no verify error) */
        if (BITS_ARE_ALL_SET(rec.int_mask_reg, REU_REG_RW_INTERRUPT_END_OF_BLOCK_ENABLED | REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED)) {
            DEBUG_LOG(DEBUG_LEVEL_REGISTER, (reu_log, "Interrupt pending"));
            rec.status |= REU_REG_R_STATUS_INTERRUPT_PENDING;
            maincpu_set_irq(reu_int_num, 1);
        }
    }

    if (BITS_ARE_ALL_SET(new_status_or_mask, REU_REG_R_STATUS_VERIFY_ERROR)) {
        if (BITS_ARE_ALL_SET(rec.int_mask_reg, REU_REG_RW_INTERRUPT_VERIFY_ENABLED | REU_REG_RW_INTERRUPT_INTERRUPTS_ENABLED)) {
            DEBUG_LOG(DEBUG_LEVEL_REGISTER, (reu_log, "Verify Interrupt pending"));
            rec.status |= REU_REG_R_STATUS_INTERRUPT_PENDING;
            maincpu_set_irq(reu_int_num, 1);
        }
    }
}

/*! \brief DMA operation writing from the host to the REU

  \param host_addr
    The host (computer) address where the operation starts

  \param reu_addr
    The REU address where the operation starts

  \param host_step
    The increment to use for the host address; must be either 0 or 1

  \param reu_step
    The increment to use for the REU address; must be either 0 or 1

  \param len
    The transfer length of the operation
*/
static void reu_dma_host_to_reu(WORD host_addr, unsigned int reu_addr, int host_step, int reu_step, int len)
{
    BYTE value;
    DEBUG_LOG(DEBUG_LEVEL_TRANSFER_HIGH_LEVEL, (reu_log, "copy ext $%05X %s<= main $%04X%s, $%04X (%d) bytes.",
                                                reu_addr, reu_step ? "" : "(fixed) ", host_addr, host_step ? "" : " (fixed)", len, len));

    assert(((host_step == 0) || (host_step == 1)));
    assert(((reu_step == 0) || (reu_step == 1)));
    assert(len >= 1);

    while (len) {
        reu_clk_inc_pre();
        machine_handle_pending_alarms(0);
        value = mem_read(host_addr);
        reu_clk_inc_post();
        DEBUG_LOG(DEBUG_LEVEL_TRANSFER_LOW_LEVEL, (reu_log, "Transferring byte: %x from main $%04X to ext $%05X.", value, host_addr, reu_addr));

        store_to_reu(reu_addr, value);
        host_addr = (host_addr + host_step) & 0xffff;
        reu_addr = increment_reu_with_wrap_around(reu_addr, reu_step);
        len--;
    }
    DEBUG_LOG(DEBUG_LEVEL_REGISTER2, (reu_log, "END OF BLOCK"));
    reu_dma_update_regs(host_addr, reu_addr, ++len, REU_REG_R_STATUS_END_OF_BLOCK);
}

/*! \brief DMA operation writing from the REU to the host

  \param host_addr
    The host (computer) address where the operation starts

  \param reu_addr
    The REU address where the operation starts

  \param host_step
    The increment to use for the host address; must be either 0 or 1

  \param reu_step
    The increment to use for the REU address; must be either 0 or 1

  \param len
    The transfer length of the operation
*/
static void reu_dma_reu_to_host(WORD host_addr, unsigned int reu_addr, int host_step, int reu_step, int len)
{
    BYTE value;
    DEBUG_LOG(DEBUG_LEVEL_TRANSFER_HIGH_LEVEL, (reu_log, "copy ext $%05X %s=> main $%04X%s, $%04X (%d) bytes.",
                                                reu_addr, reu_step ? "" : "(fixed) ", host_addr, host_step ? "" : " (fixed)", len, len));

    assert(((host_step == 0) || (host_step == 1)));
    assert(((reu_step == 0) || (reu_step == 1)));
    assert(len >= 1);

    while (len) {
        DEBUG_LOG(DEBUG_LEVEL_TRANSFER_LOW_LEVEL, (reu_log, "Transferring byte: %x from ext $%05X to main $%04X.", reu_ram[reu_addr % reu_size], reu_addr, host_addr));
        reu_clk_inc_pre();
        value = read_from_reu(reu_addr);
        mem_store(host_addr, value);
        reu_clk_inc_post();
        machine_handle_pending_alarms(0);
        host_addr = (host_addr + host_step) & 0xffff;
        reu_addr = increment_reu_with_wrap_around(reu_addr, reu_step);
        len--;
    }
    DEBUG_LOG(DEBUG_LEVEL_REGISTER2, (reu_log, "END OF BLOCK"));
    reu_dma_update_regs(host_addr, reu_addr, ++len, REU_REG_R_STATUS_END_OF_BLOCK);
}

/*! \brief DMA operation swaping data between host and REU

  \param host_addr
    The host (computer) address where the operation starts

  \param reu_addr
    The REU address where the operation starts

  \param host_step
    The increment to use for the host address; must be either 0 or 1

  \param reu_step
    The increment to use for the REU address; must be either 0 or 1

  \param len
    The transfer length of the operation
*/
static void reu_dma_swap(WORD host_addr, unsigned int reu_addr, int host_step, int reu_step, int len)
{
    BYTE value_from_reu;
    BYTE value_from_c64;
    DEBUG_LOG(DEBUG_LEVEL_TRANSFER_HIGH_LEVEL, (reu_log, "swap ext $%05X %s<=> main $%04X%s, $%04X (%d) bytes.",
                                                reu_addr, reu_step ? "" : "(fixed) ", host_addr, host_step ? "" : " (fixed)", len, len));

    assert(((host_step == 0) || (host_step == 1)));
    assert(((reu_step == 0) || (reu_step == 1)));
    assert(len >= 1);

    while (len) {
        value_from_reu = read_from_reu(reu_addr);
        reu_clk_inc_pre();
        machine_handle_pending_alarms(0);
        value_from_c64 = mem_read(host_addr);
        reu_clk_inc_post();
        DEBUG_LOG(DEBUG_LEVEL_TRANSFER_LOW_LEVEL, (reu_log, "Exchanging bytes: %x from main $%04X with %x from ext $%05X.", value_from_c64, host_addr, value_from_reu, reu_addr));
        store_to_reu(reu_addr, value_from_c64);
        mem_store(host_addr, value_from_reu);
        reu_clk_inc_pre();
        reu_clk_inc_post();
        machine_handle_pending_alarms(0);
        host_addr = (host_addr + host_step) & 0xffff;
        reu_addr = increment_reu_with_wrap_around(reu_addr, reu_step);
        len--;
    }
    DEBUG_LOG(DEBUG_LEVEL_REGISTER2, (reu_log, "END OF BLOCK"));
    reu_dma_update_regs(host_addr, reu_addr, ++len, REU_REG_R_STATUS_END_OF_BLOCK);
}

/*! \brief DMA operation comparing data between host and REU

  \param host_addr
    The host (computer) address where the operation starts

  \param reu_addr
    The REU address where the operation starts

  \param host_step
    The increment to use for the host address; must be either 0 or 1

  \param reu_step
    The increment to use for the REU address; must be either 0 or 1

  \param len
    The transfer length of the operation
*/
static void reu_dma_compare(WORD host_addr, unsigned int reu_addr, int host_step, int reu_step, int len)
{
    BYTE value_from_reu;
    BYTE value_from_c64;

    BYTE new_status_or_mask = 0;

    DEBUG_LOG(DEBUG_LEVEL_TRANSFER_HIGH_LEVEL, (reu_log, "compare ext $%05X %s<=> main $%04X%s, $%04X (%d) bytes.",
                                                reu_addr, reu_step ? "" : "(fixed) ", host_addr, host_step ? "" : " (fixed)", len, len));

    assert(((host_step == 0) || (host_step == 1)));
    assert(((reu_step == 0) || (reu_step == 1)));
    assert(len >= 1);

    /* the real 17xx does not clear these bits on compare;
     * thus, we do not clear them, either! */

    /* rec.status &= ~ (REU_REG_R_STATUS_VERIFY_ERROR | REU_REG_R_STATUS_END_OF_BLOCK); */

    while (len) {
        reu_clk_inc_pre();
        machine_handle_pending_alarms(0);
        value_from_reu = read_from_reu(reu_addr);
        value_from_c64 = mem_read(host_addr);
        reu_clk_inc_post();
        DEBUG_LOG(DEBUG_LEVEL_TRANSFER_LOW_LEVEL, (reu_log, "Comparing bytes: %x from main $%04X with %x from ext $%05X.", value_from_c64, host_addr, value_from_reu, reu_addr));
        reu_addr = increment_reu_with_wrap_around(reu_addr, reu_step);
        host_addr = (host_addr + host_step) & 0xffff;
        len--;

        if (value_from_reu != value_from_c64) {
            DEBUG_LOG(DEBUG_LEVEL_REGISTER, (reu_log, "VERIFY ERROR"));
            new_status_or_mask |= REU_REG_R_STATUS_VERIFY_ERROR;

            /* weird behaviour no. 1 of the 17xx:
             * failed verify operations consume one extra cycle, except if
             * the failed comparison happened on the last byte of the buffer.
             */
            if (len >= 1) {
                reu_clk_inc_pre();
                machine_handle_pending_alarms(0);
                reu_clk_inc_post();
            }
            break;
        }
    }

    /* the length was decremented once too much, correct this */
    if (len == 0) {
        ++len;

        /* weird behaviour no. 2 of the 17xx:
         * If the last byte failed, the "end of block transfer" bit is set, too
         */
        /* all bytes are equal, mark End Of Block */
        new_status_or_mask |= REU_REG_R_STATUS_END_OF_BLOCK;
    } else if (len == 1) {
        /* weird behaviour no. 3 of the 17xx:
         * If the next-to-last byte failed, the "end of block transfer" bit is
         * set, but only if the last byte compares equal
         */

        value_from_reu = read_from_reu(reu_addr);
        value_from_c64 = mem_read(host_addr);
        DEBUG_LOG(DEBUG_LEVEL_TRANSFER_LOW_LEVEL, (reu_log, "Comparing bytes after verify error: %x from main $%04X with %x from ext $%05X.",
                                                   value_from_c64, host_addr, value_from_reu, reu_addr));
        if (value_from_reu == value_from_c64) {
            new_status_or_mask |= REU_REG_R_STATUS_END_OF_BLOCK;
        }
    }

    assert(len >= 1);
    reu_dma_update_regs(host_addr, reu_addr, len, new_status_or_mask);
}

/* ------------------------------------------------------------------------- */

/*! \brief perform REU DMA

 This function is called when a write to REC command register or memory
 location FF00 is detected.

 \param immediate
   If 0, the DMA should not started immediately. It is only prepared, so it
   can be executed when the next call to reu_dma() occurs with something different
   than immediate == 0.

 \remark
   If the REC command register is written and
   REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED is *not* set, this function is called
   with immediate == 0. In this case, this function is armed for an execution of
   the DMA as soon as it is called with immediate == -1.\n
   If the REC command register is written and
   REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED *is* set, this function is called with
   immediate == 1. In this case, the DMA is executed immediately.\n
   If a write to $FF00 is encountered, this function is called with immediate == -1.
   If it has been previously armed (with immediate == 0), then the DMA operation is
   executed.
*/
void reu_dma(int immediate)
{
    static int delay = 0;

    if (!reu_enabled) {
        return;
    }

    if (!immediate) {
        delay = 1;
        return;
    } else {
        if (!delay && immediate < 0) {
            return;
        }
        delay = 0;
    }

    if (reu_ba.enabled) {
        /* signal CPU that BA is pulled low */
        *(reu_ba.cpu_ba) |= reu_ba.cpu_ba_mask;
    } else {
        /* start the operation right away */
        reu_dma_start();
    }
}

void reu_dma_start(void)
{
    int len;
    int reu_step, host_step;
    WORD host_addr;
    unsigned int reu_addr;

    /* wrong address of bank register & calculations corrected  - RH */
    host_addr = rec.base_computer;
    reu_addr = rec.base_reu | (rec.bank_reu << 16);
    len = rec.transfer_length ? rec.transfer_length : 0x10000;

    /* Fixed addresses implemented -- [EP] 04-16-97. */
    host_step = rec.address_control_reg & REU_REG_RW_ADDR_CONTROL_FIX_C64 ? 0 : 1;
    reu_step = rec.address_control_reg & REU_REG_RW_ADDR_CONTROL_FIX_REC ? 0 : 1;

    reu_dma_active = 1;

    switch (rec.command & REU_REG_RW_COMMAND_TRANSFER_TYPE_MASK) {
        case REU_REG_RW_COMMAND_TRANSFER_TYPE_TO_REU:
            reu_dma_host_to_reu(host_addr, reu_addr, host_step, reu_step, len);
            break;
        case REU_REG_RW_COMMAND_TRANSFER_TYPE_FROM_REU:
            reu_dma_reu_to_host(host_addr, reu_addr, host_step, reu_step, len);
            break;
        case REU_REG_RW_COMMAND_TRANSFER_TYPE_SWAP:
            reu_dma_swap(host_addr, reu_addr, host_step, reu_step, len);
            break;
        case REU_REG_RW_COMMAND_TRANSFER_TYPE_VERIFY:
            reu_dma_compare(host_addr, reu_addr, host_step, reu_step, len);
            break;
    }

    reu_dma_active = 0;
    rec.command = (rec.command & ~REU_REG_RW_COMMAND_EXECUTE) | REU_REG_RW_COMMAND_FF00_TRIGGER_DISABLED;
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "REU1764"; /*!< the name of the module for the snapshot */
#define SNAP_MAJOR 0 /*!< version number for this module, major number */
#define SNAP_MINOR 0 /*!< version number for this module, minor number */

/*! \brief type for the REU data as being stored in the snapshot.
 \remark
   Here, 16 byte are used (instead of only 11, which would be enough) to be
   compatible with the original implementation. Otherwise, we would have to
   change the version number. This way, it is much simpler.
 */
typedef BYTE reu_as_stored_in_snapshot_t[16];

/*! \brief write the REU module data to the snapshot
 \param s
    The snapshot data where to add the information for this module.

 \return
    0 on success, else -1.
*/
int reu_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    reu_as_stored_in_snapshot_t reu;
    WORD reu_address;

    memset(reu, 0xff, sizeof reu);

    for (reu_address = 0; reu_address < sizeof(reu); reu_address++) {
        reu[reu_address] = reu_read_without_sideeffects(reu_address);
    }

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_DW(m, (reu_size >> 10)) < 0 || SMW_BA(m, reu, sizeof(reu)) < 0 || SMW_BA(m, reu_ram, reu_size) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

/*! \brief read the REU module data from the snapshot
 \param s
    The snapshot data from where to read the information for this module.

 \return
    0 on success, else -1.
 */
int reu_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    DWORD size;

    reu_as_stored_in_snapshot_t reu;
    WORD reu_address;

    memset(reu, 0xff, sizeof reu);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version != SNAP_MAJOR) {
        log_error(reu_log, "Major version %d not valid; should be %d.", major_version, SNAP_MAJOR);
        goto fail;
    }

    /* Read RAM size.  */
    if (SMR_DW(m, &size) < 0) {
        goto fail;
    }

    if (size > 16384) {
        log_error(reu_log, "Size %d in snapshot not supported.", (int)size);
        goto fail;
    }

    set_reu_size((int)size, NULL);

    if (!reu_enabled) {
        set_reu_enabled(1, NULL);
    }

    if (SMR_BA(m, reu, sizeof(reu)) < 0 || SMR_BA(m, reu_ram, reu_size) < 0) {
        goto fail;
    }

    if (reu[REU_REG_R_STATUS] & 0x80) {
        interrupt_restore_irq(maincpu_int_status, reu_int_num, 1);
    } else {
        interrupt_restore_irq(maincpu_int_status, reu_int_num, 0);
    }

    for (reu_address = 0; reu_address < sizeof(reu); reu_address++) {
        reu_store_without_sideeffects(reu_address, reu[reu_address]);
    }


    snapshot_module_close(m);
    reu_enabled = 1;
    return 0;

fail:
    snapshot_module_close(m);
    reu_enabled = 0;
    return -1;
}
