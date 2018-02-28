/*
 * mmcreplay.c - Cartridge handling, MMCReplay cart.
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64pla.h"
#include "cartio.h"
#include "cartridge.h"
#include "clockport.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "flash040.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "ser-eeprom.h"
#include "snapshot.h"
#include "spi-sdcard.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "mmcreplay.h"
#include "reu.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API


#define MMCREPLAY_FLASHROM_SIZE (1024 * 512)
#define MMCREPLAY_RAM_SIZE (1024 * 512)

/* #define MMCRDEBUG */
/* #define DEBUG_LOGBANKS */    /* log access to banked rom/ram */
/* #define DEBUG_IOBANKS */     /* log access to banked rom/ram in io */
/* #define LOG_MAPPER */        /* log memory map changes */
/* #define LOG_CLOCKPORT */     /* log clockport i/o */

/* #define LOG_READ_DE00 */
/* #define LOG_WRITE_DE00 */
/* #define LOG_WRITE_DE01 */

/* #define LOG_READ_IO1_RAM */
/* #define LOG_READ_IO1_ROM */
/* #define LOG_READ_IO1_DISABLED */
/* #define LOG_WRITE_IO1_RAM */
/* #define LOG_WRITE_IO1_ROM */
/* #define LOG_WRITE_IO1_DISABLED */

/* #define LOG_READ_DF10 */
/* #define LOG_READ_DF11 */
/* #define LOG_READ_DF12 */
/* #define LOG_READ_DF13 */

/* #define LOG_WRITE_DF10 */
/* #define LOG_WRITE_DF11 */
/* #define LOG_WRITE_DF12 */
/* #define LOG_WRITE_DF13 */

/* #define LOG_READ_IO2_RAM */
/* #define LOG_READ_IO2_ROM */
/* #define LOG_READ_IO2_DISABLED */
/* #define LOG_WRITE_IO2_RAM */
/* #define LOG_WRITE_IO2_ROM */
/* #define LOG_WRITE_IO2_DISABLED */

/* #define TEST_AR_MAPPER */
/* #define TEST_RR_MAPPER */
/* #define TEST_NORDIC_MAPPER */
/* #define TEST_SUPER_8KCRT */
/* #define TEST_SUPER_16KCRT */
/* #define TEST_RESCUE_MODE */

#ifdef MMCRDEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

static int mmcr_enabled = 0;
static int enable_rescue_mode = 0;
static char *mmcr_card_filename = NULL;
static char *mmcr_eeprom_filename = NULL;
static int mmcr_card_rw = 0;
static int mmcr_eeprom_rw = 0;
static int mmcr_sd_type = 0;

static char *mmcr_filename = NULL;
static int mmcr_filetype = 0;

static int mmcr_write_image = 0;

static const char STRING_MMC_REPLAY[] = CARTRIDGE_NAME_MMC_REPLAY;

/*
Features

512KB FLASH rom
512KB SRAM
1KB Serial EEPROM for configuration storage
SPI Interface to access MMC / Secure Digital cards
Clockport to connect additional hardware
Freezer functionality (Action Replay / Retro Replay compatible)
Reset button
*/

/* the 29F040 statemachine */
static flash040_context_t *flashrom_state = NULL;

static BYTE active_mode_phi1 = 0;
static BYTE active_mode_phi2 = 0;

/* RAM */
static BYTE *mmcr_ram = NULL;

/* RAM banking.*/
static unsigned int raml_bank = 0, ramh_bank = 0;
static unsigned int enable_raml = 0;
static unsigned int enable_ramh = 0;

static int ramA000_bank = 0;
static unsigned int enable_ramA000 = 0;

static int romA000_bank = 0;

/* IO banking.*/
static unsigned int io1_bank = 0, io2_bank = 0;
static unsigned int io1_ram_bank = 0, io2_ram_bank = 0;
static unsigned int enable_io1 = 0;
static unsigned int enable_io2 = 0;
static unsigned int enable_io1_ram = 0;
static unsigned int enable_io2_ram = 0;

static unsigned int ultimax_mapping_hack = 0;

/*** $DE00:     RR control register write */
/* bit 0:       GAME line (W) */
static unsigned int enable_game = 0;    /* (w) status of game line */
static unsigned int enable_flash_write = 1;     /* (r) */
/* bit 1:       EXROM line (W) */
static unsigned int enable_exrom = 0;   /* status of exrom line */
/* bit 2:       1 = disable RR (W) */
/*              (*) bit can be reset by setting bit 6 of $df12 */
static unsigned int rr_active_bit = 0;  /* (w) Cart is activated., also used in Nordic Power mode  */
static unsigned int rr_active = 0;      /* (w) Cart is activated.  */
static unsigned int freeze_pressed = 0; /* (r) freeze button pressed.  */
/* ^ bit 3:     bank address 13 (W) */
/* ^ bit 4:     bank address 14 (W) */
/* bit 5:       0 = rom enable, 1 = ram enable (W) */
static unsigned int enable_ram_io = 0;
/* bit 6:       1 = exit freeze mode (W) */
static unsigned int enable_freeze_exit = 0;
/* bit 7:       bank address 15 (W) */
static unsigned int bank_address_13_15 = 0;

/*** $DE01:     extended RR control register write */
/* bit 0:       0 = disable clockport, 1 = enable clockport (W) */
int mmcr_clockport_enabled = 0;         /* used globally, eg in c64io.c */
/* bit 1:       0 = disable io ram banking, 1 = enable io ram banking (W) */
static unsigned int allow_bank = 1;     /* RAM bank switching allowed.  */
/* bit 2:       0 = enable freeze, 1 = disable freeze (W) */
static unsigned int no_freeze = 0;      /* Freeze is disallowed.  */
/* ^ bit 3:     bank address 13 (mirror of $DE00) (W) */
/* ^ bit 4:     bank address 14 (mirror of $DE00) (W) */
/* bit 5:       0 = enable MMC registers, 1 = disable MMC registers (W) */
/*              (*) Can only be written when bit 6 of $DF12 is 1. Register becomes effective
                when bit 0 of $DF11 is 1.*/
static unsigned int enable_mmc_regs = 0;
static unsigned int enable_mmc_regs_pending = 0;
/* bit 6:       0 = ram/rom @ DFxx, 1 = ram/rom @ $DExx (W) */
static unsigned int enable_ram_io1 = 0; /* REU compatibility mapping.  */
/* ^ bit 7:     bank address 15 (mirror of $DE00) (W) */

/* Only one write access is allowed.  */
static unsigned int write_once;

/* ^ $DE02 - $DE0F: Clockport memory area (when enabled) */

/* ^ $DF10:     MMC SPI transfer register */
/*              byte written is sent to the card & response from the card is read here */

/**** $DF11:    MMC control register */
/* bit 0:       0 = MMC BIOS enabled, 1 = MMC BIOS disabled (R/W) (*) */
/*              (*) Enabling MMC Bios sets ROM banking to the last 64K bank */
static unsigned int disable_mmc_bios = 0;
/* ^ bit 1:     0 = card selected, 1 = card not selected (R/W) (**) */
/*              (**) This bit also controls the green activity LED */
/* ^ bit 2:     0 = 250khz transfer, 1 = 8mhz transfer (R/W) */
/* ^ bit 3:     ** ALWAYS 0 ** */
/* ^ bit 4:     ** ALWAYS 0 ** */

/* bit 5:       (in RR-Mode:) 0 = allow RR rom when MMC BIOS disabled , 1 = disable RR ROM (R/W) (***) */
/*              (***) When in mmcreplay bios mode, bit 5 controls RAM banking (0 = $e000 - $ffff, 1 = $8000 - $9fff)
                When in 16K mode, bit 5 enables RAM at $a000 - $bfff */
static unsigned int disable_rr_rom = 0;

/* ^ bit 6:     0 = SPI write trigger mode, 1 = SPI read trigger mode (R/W) */
/* ^ bit 7:     ** ALWAYS 0 ** */

/***** $DF12:   MMC status register */
/* bit 0:       0 = SPI ready, 1 = SPI busy (R) */
/*              1 = forbid ROM write accesses (W) (*) */
/*              (*) Setting this bit will disable writes to ROM until next reset */
static unsigned int disable_rom_write = 0;      /* (w) disables rom write until reset */

/* ^ bit 1:     feedback of $DE00 bit 0 (GAME) (R) */
/* ^ bit 2:     feedback of $DE00 bit 1 (EXROM) (R) */
/* ^ bit 3:     0 = card inserted, 1 = no card inserted (R) */
/* ^ bit 4:     0 = card write enabled, 1 = card write disabled (R) */

/* ^ bit 5:     EEPROM DATA line / DDR Register (R/W) (**) */
/*              (**) Setting DATA to "1" enables reading data bit from EEPROM at this position. */
/* bit 6:       0 = RR compatibility mode, 1 = Extended mode (W) (***) */
/*              (***) Selecting RR compatibility mode limits RAM to 32K and
                disables writes to extended banking register.
                Selecting Extended mode enables full RAM banking and enables
                Nordic Power mode in RR mode. */
static unsigned int enable_extended_mode = 0;   /* bit 6 */
/* ^ bit 7:     EEPROM CLK line (W) */


/***** $DF13:   Extended banking register (*) */
/*              (*) Can only be read/written to when bit 6 of $DF12 is 1 */
/* ^ bit 0:     bank address 16 (R/W) */
/* ^ bit 1:     bank address 17 (R/W) */
/* bit 2:       bank address 18 (R/W) */
static unsigned int bank_address_16_18 = 7;

/* ^ bit 3:     ** ALWAYS 0 ** */
/* ^ bit 4:     ** ALWAYS 0 ** */
/* bit 5:       16K rom mapping (R/W) */
static unsigned int enable_16k_mapping = 0;
/* bit 6:       1 = enable RR register */
/*              Disabling RR Register disables ALL ROM/RAM banking too. */
static unsigned int enable_rr_regs = 1;
/* ^ bit 7:     ** ALWAYS 0 ** */

/* Current clockport device */
static int clockport_device_id = CLOCKPORT_DEVICE_NONE;
static clockport_device_t *clockport_device = NULL;

static char *clockport_device_names = NULL;

/********************************************************************************************************************/

/* some prototypes are needed */
static BYTE mmcreplay_io1_read(WORD addr);
static void mmcreplay_io1_store(WORD addr, BYTE value);
static BYTE mmcreplay_io2_read(WORD addr);
static void mmcreplay_io2_store(WORD addr, BYTE value);
static int mmcreplay_dump(void);

static BYTE mmcreplay_clockport_read(WORD io_address);
static BYTE mmcreplay_clockport_peek(WORD io_address);
static void mmcreplay_clockport_store(WORD io_address, BYTE byte);

static io_source_t mmcreplay_io1_device = {
    CARTRIDGE_NAME_MMC_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    mmcreplay_io1_store,
    mmcreplay_io1_read,
    NULL, /* TODO: peek */
    mmcreplay_dump,
    CARTRIDGE_MMC_REPLAY,
    0,
    0
};

static io_source_t mmcreplay_io2_device = {
    CARTRIDGE_NAME_MMC_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    mmcreplay_io2_store,
    mmcreplay_io2_read,
    NULL, /* TODO: peek */
    mmcreplay_dump,
    CARTRIDGE_MMC_REPLAY,
    0,
    0
};

static io_source_t mmcreplay_clockport_device = {
    CARTRIDGE_NAME_MMC_REPLAY " Clockport",
    IO_DETACH_RESOURCE,
    "MMCRClockPort",
    0xde02, 0xde0f, 0x0f,
    0,
    mmcreplay_clockport_store,
    mmcreplay_clockport_read,
    mmcreplay_clockport_peek,
    mmcreplay_dump,
    CARTRIDGE_MMC_REPLAY,
    0,
    0
};

static io_source_list_t *mmcreplay_io1_list_item = NULL;
static io_source_list_t *mmcreplay_io2_list_item = NULL;
static io_source_list_t *mmcreplay_clockport_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MMC_REPLAY, 1, 1, &mmcreplay_io1_device, &mmcreplay_io2_device, CARTRIDGE_MMC_REPLAY
};

/********************************************************************************************************************/

#ifdef LOG_MAPPER
void mmcreplay_dump_cfg(void)
{
    static char dumpstr1[0x100];
    static char dumpstr2[0x100];
    static char ndumpstr[0x100];
    char *str_mapper[4] = { "MMCBIOS", "RR", "SUPER", "NORMAL" };
    char *str_config[4] = { "off", "ultimax", "8K Game", "16K Game" };
    int mapper, config;

    sprintf(ndumpstr,
            "MMCREPLAY: cart active: %d disable rr rom/opt.mapping:%d rr regs active:%d  extended mode:%d  16k mapping:%d RAM IO1 Mapping:%d RAM Banking:%d",
            rr_active, disable_rr_rom, enable_rr_regs, enable_extended_mode,
            enable_16k_mapping, enable_ram_io1, allow_bank);
    if (strcmp(dumpstr1, ndumpstr) != 0) {
        strcpy(dumpstr1, ndumpstr);
        LOG(("%s", dumpstr1));
    }

    config = active_mode_phi2 & 3;

    if (disable_mmc_bios == 0) {
        /* mmc bios mapper */
        mapper = 0;
    } else if (enable_16k_mapping) {
        /* super mapper */
        mapper = 2;
    } else if (disable_rr_rom) {
        /* normal mapper */
        mapper = 3;
    } else {
        /* rr mapper */
        mapper = 1;
    }


/*
  mode: (exrom<<1)|game
        0          0
        0          1            ultimax   roml ($8000),romh ($e000)
        1          0            8k game   roml ($8000)
        1          1            16k game  roml ($8000),romh ($a000)
*/
    {
        char str[0x100];
        char romlbank[0x100];
        char romhbank[0x100];
        char ramlbank[0x100];
        char ramhbank[0x100];
        char ramA000bank[0x100];
        char romA000bank[0x100];
        char io1bank[0x100];
        char io2bank[0x100];
        char io1rambank[0x100];
        char io2rambank[0x100];

        sprintf(romlbank, "(%d:%d)", roml_bank >> 3, roml_bank & 7);
        sprintf(romhbank, "(%d:%d)", romh_bank >> 3, romh_bank & 7);
        sprintf(ramlbank, "(%d:%d)", raml_bank >> 3, raml_bank & 7);
        sprintf(ramhbank, "(%d:%d)", ramh_bank >> 3, ramh_bank & 7);
        /* a000- bfff */
        sprintf(ramA000bank, "(%d:%d)", ramA000_bank >> 3, ramA000_bank & 7);
        sprintf(romA000bank, "(%d:%d)", romA000_bank >> 3, romA000_bank & 7);  /* FIXME: always = raml bank? */

        sprintf(io1bank, "(%d:%d)", io1_bank >> 3, io1_bank & 7);
        sprintf(io2bank, "(%d:%d)", io2_bank >> 3, io2_bank & 7);
        sprintf(io1rambank, "(%d:%d)", io1_ram_bank >> 3, io1_ram_bank & 7);
        sprintf(io2rambank, "(%d:%d)", io2_ram_bank >> 3, io2_ram_bank & 7);
        str[0] = 0;

        switch (config) {
            case 0:            /* off */
                strcat(str, "---");
                break;
            case 1:            /* ultimax */
                if (enable_raml) {
                    strcat(str, "RAML");
                    strcat(str, ramlbank);
                } else {
                    strcat(str, "ROML");
                    strcat(str, romlbank);
                }
                strcat(str, "($8000)");
                if (enable_ramA000) {
                    strcat(str, "RAMA000");
                    strcat(str, ramA000bank);
                } else {
                    strcat(str, "ROMA000");
                    strcat(str, romA000bank);
                }
                strcat(str, "($a000)");
                if (enable_ramh) {
                    strcat(str, "RAMH");
                    strcat(str, ramhbank);
                } else {
                    strcat(str, "ROMH");
                    strcat(str, romhbank);
                }
                strcat(str, "($e000)");
                break;
            case 2:
                if (enable_raml) {
                    strcat(str, "RAML");
                    strcat(str, ramlbank);
                } else {
                    strcat(str, "ROML");
                    strcat(str, romlbank);
                }
                strcat(str, "($8000)");
                break;
            case 3:
                if (enable_raml) {
                    strcat(str, "RAML");
                    strcat(str, ramlbank);
                } else {
                    strcat(str, "ROML");
                    strcat(str, romlbank);
                }
                strcat(str, "($8000)");
                if (enable_ramh) {
                    strcat(str, "RAMH");
                    strcat(str, ramhbank);
                } else {
                    strcat(str, "ROMH");
                    strcat(str, romhbank);
                }
                strcat(str, "($a000)");
                break;
        }

        strcat(str, " IO1: ");

        if (enable_io1) {
            if (enable_ram_io) {
                strcat(str, "RAM ");
                strcat(str, io1rambank);
            } else {
                strcat(str, "ROM ");
                strcat(str, io1bank);
            }
        } else {
            strcat(str, " off ");
        }

        strcat(str, " IO2: ");

        if (enable_io2) {
            if (enable_ram_io) {
                strcat(str, "RAM ");
                strcat(str, io2rambank);
            } else {
                strcat(str, "ROM ");
                strcat(str, io2bank);
            }
        } else {
            strcat(str, " off ");
        }

        sprintf(ndumpstr, "[%s] %s %s", str_mapper[mapper], str_config[config], str);
#if 1
        if (strcmp(dumpstr2, ndumpstr) != 0) {
            strcpy(dumpstr2, ndumpstr);
            LOG(("%s", dumpstr2));
        }
#else
        LOG(("MMCREPLAY: %s", ndumpstr));
#endif
    }
}
#endif

/*
basic operation modes:

$df11:0 (enable_mmc_bios)
$df11:5 (disable_rr_rom)
$df13:5 (enable_16k_mapping)

                       $df11:0 5  $df13:5
C64 standard mode            1 ?        0             (*1)
MMC Replay Bios mode         0 X        X             (*2)
Retro Replay mode            1 0        0             (*3)
Super Mapper mode            1 m        1             (*4)

(*1) In this mode, GAME and EXROM are disabled. However, IO Ram/Rom banking is still active.
(*2) MMC-Replay BIOS Mode. This mode is only intended for official Bios development
(*3) Retro Replay compatible mode. This mode is designed to work exactly like a Retro Replay
     which is plugged into a MMC64. That means GAME/EXROM is active, and RAM/ROM banking works
     normally. Extended banking registers at $df13 are active
(*4) 16k Super Mapper Mode. This mode is designed to define an enhanced Retro Replay mode, in
     which you have 16k ROM/RAM banking at $8000-$bfff and 512 byte Ram/Rom banking at $de00-$dfff.
     GAME/EXROM works like in Retro Replay mode. Banking bit A13 is disabled, since only 16K banks
     are selected in this mode. This mode also defines 2 different RAM modes.

banking, Super Mapper mode:

Lets assume we have RAM enabled (bit 5 of $de00 = 1):

$df11: bit 5 = 0 (16k RAM/ROM mixed mode)
-----------------------------------------
$8000 ----------------------------------
                8K RAM bank lo
$a000 ----------------------------------
                8K FLASH ROM bank hi
$c000 ----------------------------------

$df11: bit 5 = 1 (16k RAM mode)
-----------------------------------------
$8000 ----------------------------------
                8K RAM bank lo
$a000 ----------------------------------
                8K RAM bank hi
$c000 ----------------------------------

Allowbank masks only banking bits A13-A15 in Super Mapper mode.
*/

void mmcreplay_ramhbank_set(unsigned int bank)
{
    ramh_bank = (int)bank;
}

void mmcreplay_ramlbank_set(unsigned int bank)
{
    raml_bank = (int)bank;
}

void mmcreplay_io1bank_set(unsigned int bank, unsigned int rambank)
{
    io1_bank = (int)bank;
    io1_ram_bank = (int)rambank;
}

void mmcreplay_io2bank_set(unsigned int bank, unsigned int rambank)
{
    io2_bank = (int)bank;
    io2_ram_bank = (int)rambank;
}

#ifdef MMCRDEBUG
static int last_mainmode = 0;
static int last_biosmode = 0;
static int last_biosmode444 = 0;
#endif

/* FIXME: mapping in freeze mode */
static void mmcreplay_update_mapper_nolog(unsigned int wflag, int release_freeze)
{
    unsigned int cartbankl = 0, cartbankh = 0;
    unsigned int rambankl = 0, rambankh = 0;
    unsigned int io1bank = 0, io2bank = 0;
    unsigned int io1bank_ram = 0, io2bank_ram = 0;
    int mapped_game = -1, mapped_exrom = -1;

    ultimax_mapping_hack = 0;

    mapped_exrom = enable_exrom;
    mapped_game = enable_game;

    enable_io1_ram = enable_ram_io;
    enable_io2_ram = enable_ram_io;

    enable_ramA000 = 0;

    /* df11:0  0 = mmcreplay bios mode */
    if (disable_mmc_bios == 0) {
        /*
         * in rescue mode GAME and EXROM are tri-stated if bit0 of DF11 is 0
         */
        if (enable_rescue_mode) {
            mapped_exrom = 0;
            mapped_game = 0;
            LOG(("main mode: rescue"));
        } else {
#ifdef MMCRDEBUG
            if (last_mainmode != 111) {
                LOG(("main mode: mmc bios"));
                last_mainmode = 111;
            }
#endif
            /**************************************************************************************************
             * mmc bios mapper
             ***************************************************************************************************/

            /*
             * "In MMC Replay Bios Mode, the ROM bank is normaly fixed to bank 7 when
             * RAM is disabled. When enabling RAM, one can select betwen 2 different
             * RAM modes usiing bit#5 of $DF11. Note that RAM enabled at $8000 is
             * read only while RAM mapped at $E000 is both read and write enabled.
             * GAME & EXROM bits have no effect, the serial EEPROM can be accessed."
             */
            cartbankl = ((7 << 3) | (bank_address_13_15)) & (0x3f);     /* always last 64k bank in mmc bios mode */
            cartbankh = cartbankl;

            rambankl = ((7 << 3) | bank_address_13_15) & (0x3f);
            rambankh = rambankl;
            io1bank = ((7 << 3) | bank_address_13_15) & (0x3f);
            io2bank = io1bank;
            /* FIXME */
            io1bank_ram = io1bank;
            io2bank_ram = io2bank;

            /* FIXME */
            mapped_game = 1;
            mapped_exrom = 1;

            enable_raml = 0;
            enable_ramh = 0;

            /* df11:5 When in mmcreplay bios mode, bit 5 controls RAM banking
             * 0 = $e000 - $ffff    (512K read/write window)
             * 1 = $8000 - $9fff    (512K read only  window)
             */
            if (disable_rr_rom) { /* 1 = $8000 - $9fff (512K read only window) */
                /* 16k game mode,  16k game mode */
                mapped_game = 1;
                mapped_exrom = 1;

                if (enable_ram_io1) {
#ifdef MMCRDEBUG
                    if (last_biosmode != 111) {
                        LOG(("bios mode: 111"));
                        last_biosmode = 111;
                    }
#endif
                    /*
                     * disable_rr_rom=1 (opt.banking)
                     * enable_ram_io1=1  (use io1+2)
                     *
                     * extended_mode=0   (1= toggle IO2 in cfg $22)
                     * allow_bank (active for ram in IO)
                     *
                     * enable_ram_io=0
                     * 00 0Xro0Xro 0Xro---- kern
                     * 00 0Xro0Xro 0Xro---- kern
                     * 00 0Xro0Xro 0Xro---- kern
                     * 00 0Xro0Xro 0Xro---- kern
                     *
                     * $8000 - $9fff      bios rom
                     * $a000 - $bfff      bios rom (mirror)
                     *
                     * enable_ram_io=1
                     * 20 0Xra0Xro 00ra00ra kern
                     * 21 0Xra0Xro 00ra00ra kern
                     * 22 0Xra0Xro 00ra00ra kern
                     * 23 0Xra0Xro 00ra00ra kern
                     *
                     * $8000 - $9fff      512K read only  ram window
                     * $a000 - $bfff      bios rom (mirror)
                     */
                    if (enable_ram_io) {
                        enable_raml = 1;
                        enable_io1 = 1;
                        enable_io2 = 1;

                        if (allow_bank) {
                            io1bank_ram = (7 << 3) | io1bank_ram;
                            io2bank_ram = (7 << 3) | io2bank_ram;
                        } else {
                            io1bank_ram = (0 << 3) | 0;
                            io2bank_ram = (0 << 3) | 0;
                        }
                    } else {
                        /* enable_raml=0; */
                        enable_io1 = 1;
                        enable_io2 = 0;
                    }
                } else {
#ifdef MMCRDEBUG
                    if (last_biosmode != 222) {
                        LOG(("bios mode: 222"));
                        last_biosmode = 222;
                    }
#endif
                    /*
                     * disable_rr_rom=1 (opt.banking)
                     * enable_ram_io1=0  (use io2 only)
                     *
                     * extended_mode=0   (1= toggle IO2 in cfg $22)
                     * allow_bank (active for ram in IO)
                     *
                     * enable_ram_io=0
                     * 00 0Xro0Xro----0Xrokern
                     * 00 0Xro0Xro----0Xr0kern
                     * 00 0Xro0Xro----0Xrokern
                     * 00 0Xro0Xro----0Xrokern
                     *
                     * $8000 - $9fff      bios rom
                     * $a000 - $bfff      bios rom (mirror)
                     *
                     * enable_ram_io=1
                     * 20 0Xra0Xro----00rakern
                     * 21 0Xra0Xro----00rakern
                     * 22 0Xra0Xro----00rakern (*)
                     * 23 0Xra0Xro----00rakern
                     *
                     * $8000 - $9fff        (512K read only  ram window)
                     * $a000 - $bfff      bios rom (mirror)
                     */
                    if (enable_ram_io) {
                        enable_raml = 1;
                        enable_io1 = 0;
                        enable_io2 = 1;

                        if (allow_bank) {
                            io1bank_ram = (7 << 3) | io1bank_ram;
                            io2bank_ram = (7 << 3) | io2bank_ram;
                        } else {
                            io1bank_ram = (0 << 3) | 0;
                            io2bank_ram = (0 << 3) | 0;
                        }

                        if (enable_extended_mode) {
                            if (((enable_game) | (enable_exrom << 1)) == 2) {
                                /* ??? ROM in io? */
                                enable_io1_ram = 0;
                                enable_io2_ram = 0;
                            }
                        }
                    } else {
                        enable_io1 = 0;
                        enable_io2 = 1;
                    }
                }
            } else { /* disable_rr_rom=0   $e000 - $ffff    (512K read/write window) */
                /* 16k game mode,  ultimax */
                /* ultimax, ram at $e000, rom at $8000, rom at $a000 */
                mapped_game = 1;
                mapped_exrom = 1;
                ultimax_mapping_hack = 1;

                if (enable_ram_io1) {
#ifdef MMCRDEBUG
                    if (last_biosmode != 333) {
                        LOG(("bios mode: 333"));
                        last_biosmode = 333;
                    }
#endif
                    /*
                     * disable_rr_rom=0 (opt.banking)
                     * enable_ram_io1=1  (use io1+2)
                     *
                     * extended_mode=0   (not active ?)
                     * allow_bank (active for ram in both IO)
                     *
                     * enable_ram_io=0
                     * 00 0Xro0Xro 0Xr0---- kern
                     * 00 0Xro0Xro 0Xr0---- kern
                     * 00 0Xro0Xro 0Xr0---- kern
                     * 00 0Xro0Xro 0Xr0---- kern
                     *
                     * $8000 - $9fff      bios rom
                     * $a000 - $bfff      bios rom (mirror)
                     *
                     * enable_ram_io=1
                     * 20 0Xro0Xro 0Zra0Zra 0Xra
                     * 21 0Xro0Xro 0Zra0Zra 0Xra
                     * 22 0Xro0Xro 0Zra0Zra 0Xra
                     * 23 0Xro0Xro 0Zra0Zra 0Xra
                     *
                     * $8000 - $9fff      bios rom
                     * $a000 - $bfff      bios rom (mirror)
                     * $e000 - $ffff           512K read/write ram window
                     */
                    if (enable_ram_io) { /* ultimax */
                        /* ultimax, ram at $e000, rom at $8000, rom at $a000 */
                        enable_ramh = 1;
                        enable_io1 = 1;
                        enable_io2 = 1;
                        mapped_game = 1;
                        mapped_exrom = 0;       /* ultimax */

                        if (allow_bank) {
                            io1bank_ram = (7 << 3) | io1bank_ram;
                            io2bank_ram = (7 << 3) | io2bank_ram;
                        } else {
                            io1bank_ram = (0 << 3) | 0;
                            io2bank_ram = (0 << 3) | 0;
                        }
                    } else {       /* 16k game */
                        enable_io1 = 1;
                        enable_io2 = 0;
                        mapped_game = 1;
                        mapped_exrom = 1;       /* 16k game */
                    }
                } else {
#ifdef MMCRDEBUG
                    if (last_biosmode != 444) {
                        LOG(("bios mode: 444"));
                        last_biosmode = 444;
                    }
#endif
                    /*
                     * disable_rr_rom=0 (opt.banking)
                     * enable_ram_io1=0  (use io2 only)
                     *
                     * extended_mode=0   (1= toggle IO2 in cfg $22)
                     * allow_bank (active for ram in IO)
                     *
                     * enable_ram_io=0
                     * 00 0Wro0Wro ----0Wr0 kern
                     * 01 0Wro0Wro ----0Wr0 kern
                     * 02 0Wro0Wro ----0Wr0 kern
                     * 03 0Wro0Wro ----0Wr0 kern
                     *
                     * enable_ram_io=1
                     * 20 0Wro0Wro ----0Zra 0Zra
                     * 21 0Wro0Wro ----0Zra 0Zra
                     * 22 0Wro0Wro ----0Yra 0Zra (*)
                     * 23 0Wro0Wro ----0Zra 0Zra
                     *
                     * $8000 - $9fff      bios rom
                     * $a000 - $bfff      bios rom (mirror)
                     * $e000 - $ffff           512K read/write ram window
                     *
                     * X=normal bank
                     *
                     * Y changes to ? if extended mode
                     * else ==Z
                     * W changes to 2 if extended mode and high bank=7
                     * full banking if extended mode and high bank!=7
                     * else ==X
                     *
                     * Z bank 00 if allow_bank=0
                     * full banking in extended mode
                     * high bank=7 if not extended mode
                     */
                    if (enable_ram_io) { /* ultimax */
#ifdef MMCRDEBUG
                        if (last_biosmode444 != 111) {
                            LOG(("bios mode 444: ram enabled"));
                            last_biosmode444 = 111;
                        }
#endif
                        /* ultimax, ram at $e000, rom at $8000, rom at $a000 */
                        enable_ramh = 1;
                        enable_io1 = 0;
                        enable_io2 = 1;

                        mapped_game = 1;
                        mapped_exrom = 0;       /* ultimax */


                        /* if (enable_extended_mode) */
                        {
                            /* full rom banking */
                            cartbankl =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            cartbankh = cartbankl;
                            rambankl =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            rambankh = rambankl;
                            io1bank =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            io2bank = io1bank;

                            /* if (bank_address_16_18==7) */
                            {
                                switch (bank_address_13_15) {
                                    case 6:
                                    case 7:
                                        cartbankl = (7 << 3) | 0;
                                        cartbankh = (7 << 3) | 0;
                                        break;
                                    default:
                                        cartbankl =
                                            (7 << 3) | bank_address_13_15;
                                        cartbankh =
                                            (7 << 3) | bank_address_13_15;
                                        break;
                                }
                            }

                            if (allow_bank) {
                                /* full ram banking in extended mode */
                                io1bank_ram =
                                    ((bank_address_16_18 << 3) |
                                     bank_address_13_15) & (0x3f);
                                io2bank_ram = io1bank;
                            } else {
                                io1bank_ram = (0 << 3) | 0;
                                io2bank_ram = (0 << 3) | 0;
                            }
                        }

                        romA000_bank = cartbankl;
                        ramA000_bank = cartbankl;
                        rambankh = rambankl;
                    } else {       /* 16k game */
#ifdef MMCRDEBUG
                        if (last_biosmode444 != 222) {
                            LOG(("bios mode 444: ram disabled"));
                            last_biosmode444 = 222;
                        }
#endif
                        enable_io1 = 0;
                        enable_io2 = 1;
                        mapped_game = 1;
                        mapped_exrom = 1;       /* 16k game */

                        cartbankl = ((7 << 3) | bank_address_13_15) & (0x3f);
                        cartbankh = cartbankl;
                        rambankl = ((7 << 3) | bank_address_13_15) & (0x3f);
                        rambankh = rambankl;
                        io1bank = ((7 << 3) | bank_address_13_15) & (0x3f);
                        io2bank = io1bank;

                        romA000_bank = cartbankl;

                        if (enable_extended_mode) {
#if 0
                            /* full rom banking */
                            cartbankl =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            cartbankh = cartbankl;
                            rambankl =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            rambankh = rambankl;
                            io1bank =
                                ((bank_address_16_18 << 3) | bank_address_13_15)
                                & (0x3f);
                            io2bank = io1bank;

                            romA000_bank = cartbankl;
#endif
                        }
                    }
                }
            }
        }
    } else if (enable_16k_mapping) {
#ifdef MMCRDEBUG
        if (last_mainmode != 222) {
            LOG(("main mode: super mapper"));
            last_mainmode = 222;
        }
#endif
        /*************************************************************************************************
         * super mapper
         *************************************************************************************************/
        cartbankl = ((bank_address_16_18 << 3) | bank_address_13_15) & (0x3e);
        cartbankh = cartbankl + 1;
        rambankl = ((bank_address_16_18 << 3) | bank_address_13_15) & (0x3e);
        rambankh = rambankl + 1;
        io1bank = ((bank_address_16_18 << 3) | bank_address_13_15) & (0x3e);
        io2bank = io1bank;

        romA000_bank = cartbankh;

#if 0
        if (!allow_bank) {
            rambankl &= 0x38;   /* FIXME: clear last 3 bits ? */
            rambankh &= 0x38;   /* FIXME: clear last 3 bits ? */
            io1bank &= 0x38;    /* FIXME: clear last 3 bits ? */
            io2bank &= 0x38;    /* FIXME: clear last 3 bits ? */
        }
#endif
        /* FIXME */
        io1bank_ram = io1bank;
        io2bank_ram = io2bank;

        enable_io1 = 1;
        enable_io2 = 1;

        enable_raml = 0;
        enable_ramh = 0;

        if (disable_rr_rom) {    /* When in 16K mode, bit 5 enables RAM at $a000 - $bfff */
            /*
             * disable_rr_rom=1 (opt.banking)
             *
             * allow_bank        (inactive?)
             * extended_mode     (inactive?)
             * enable_ram_io1=0  (use io2 only)
             *
             * enable_ram_io=0
             * 00 0Xrobasi0Xro0Xrokern
             * 00 0Xro0Yra0Xro0Xr0kern
             * 00  rambasi0Xro0Xrokern
             * 00 0Xro----0Xro0Xro0Yra
             * enable_ram_io=1
             * 00 0Xrabasi0Xra0Xrakern
             * 00 0Xra0Yra0Xra0Xrakern
             * 00  rambasi0Xra0Xrakern
             * 00 0Xra----0Xra0Xra0Yra
             *
             * X=bankl Y=bankl+1
             */
            enable_ramh = 1;

            if (enable_ram_io) {
                enable_raml = 1;
            }
        } else {
            /*
             * disable_rr_rom=0 (opt.banking)
             *
             * allow_bank        (inactive?)
             * extended_mode     (inactive?)
             * enable_ram_io1=0  (use io2 only)
             *
             * enable_ram_io=0
             * 00 0Xrobasi0Xro0Xrokern
             * 00 0Xro0Yro0Xro0Xr0kern
             * 00  rambasi0Xro0Xrokern
             * 00 0Xro----0Xro0Xro0Yro
             * enable_ram_io=1
             * 00 0Xrabasi0Xra0Xrakern
             * 00 0Xra0Yro0Xra0Xrakern
             * 00  rambasi0Xra0Xrakern
             * 00 0Xra----0Xra0Xra0Yro
             *
             * X=bankl Y=bankl+1
             */
            if (enable_ram_io) {
                enable_raml = 1;
                enable_ramh = 0;
            }
        }
        LOG(("raml:%d ramh:%d", enable_raml, enable_ramh));
    } else if (disable_rr_rom) {
        /**************************************************************************************************
         * normal mapper
         **************************************************************************************************/
#ifdef MMCRDEBUG
        if (last_mainmode != 333) {
            LOG(("main mode: normal mapper"));
            last_mainmode = 333;
        }
#endif
        cartbankl = (bank_address_16_18 << 3) | (bank_address_13_15);
        cartbankh = cartbankl;
        rambankl = ((bank_address_16_18 << 3) | bank_address_13_15);
        rambankh = rambankl;
        io1bank = ((bank_address_16_18 << 3) | bank_address_13_15);
        io2bank = io1bank;

        romA000_bank = cartbankl;

        /* FIXME */
        io1bank_ram = io1bank;
        io2bank_ram = io2bank;

        enable_raml = 0;
        enable_ramh = 0;

        if (enable_extended_mode) {
            /* FIXME */
            enable_io1 = 1;
            enable_io2 = 1;
        }
    } else {
        /**************************************************************************************************
         * retro replay mapper
         **************************************************************************************************/
#ifdef MMCRDEBUG
        if (last_mainmode != 444) {
            LOG(("MMCREPLAY: *** main mode: [rr mapper]"));
            last_mainmode = 444;
        }
#endif
        cartbankl = (bank_address_16_18 << 3) | (bank_address_13_15);
        cartbankh = cartbankl;
        rambankl = ((bank_address_16_18 << 3) | bank_address_13_15);
        rambankh = rambankl;
        io1bank = ((bank_address_16_18 << 3) | bank_address_13_15);
        io2bank = io1bank;

        if (allow_bank) {
        } else {
#if 0
            if (!enable_extended_mode) {
                io1bank &= 0x38;        /* FIXME: clear last 3 bits ? */
                io2bank &= 0x38;        /* FIXME: clear last 3 bits ? */
            }
#endif
        }

        /* FIXME */
        io1bank_ram = io1bank;
        io2bank_ram = io2bank;

        enable_raml = 0;
        enable_ramh = 0;
        /*
         * Selecting Extended mode enables full RAM banking and enables
         * Nordic Power mode in RR mode.
         */
        /* FIXME: add 64kb-rombank offset also for ram ? */
        if (enable_extended_mode) {
            /* extended RR Mode */
            BYTE value = (enable_game) |     /* bit 0 */
                         ((enable_exrom ^ 1) << 1) | /* bit 1 FIXME: is the bit inverted in the register ?! */
                         (rr_active_bit << 2) | /* bit 2 */
                         ((bank_address_13_15 & 3) << 3) | /* bit 3,4 */
                         (enable_ram_io << 5) | /* bit 5 */
                         (enable_freeze_exit << 6); /* bit 6 */

            LOG(("rr mode 111 (extended) (%02x)", value));

            if ((value & 0xe7) == 0x22) {
                /*
                    different to original AR:

                    if bit 5 (RAM enable) is 1,
                    bit 0,1 (exrom/game) is == 2 (cart off),
                    bit 2,6,7 (cart disable, freeze clear) are 0,

                    then Cart ROM (Bank 0..3) is mapped at 8000-9fff,
                    and Cart RAM (Bank 0) is mapped at A000-bfff
                    using 16K Game config
                */
                mapped_game = 0;
                mapped_exrom = 1;
                enable_ramh = 1;
                LOG(("Nordic Power Mapping"));
            } else {
                if (enable_ram_io) {
                    enable_raml = 1;
                }
            }



            /* FIXME */
            enable_io1 = 1;
            enable_io2 = 1;
#if 1
            if (enable_ram_io1) {
                enable_io1 = 0;
                enable_io2 = 1;
            } else {
                enable_io1 = 0;
                enable_io2 = 1;
            }
#endif
        } else {
            LOG(("rr mode 222 (not extended)"));
            LOG(("enable_ram_io:%d", enable_ram_io));
            /* action replay hack */
            if (enable_ram_io) {
                BYTE value = (enable_game) |    /* bit 0 */
                             ((enable_exrom ^ 1) << 1) | /* bit 1 FIXME: is the bit inverted in the register ?! */
                             (((bank_address_13_15 & 3) << 3) | ((bank_address_13_15 & 4) << 5)) | /* bit 3,4,7 */
                             (enable_ram_io << 5) | /* bit 5 */
                             (enable_freeze_exit << 6); /* bit 6 */
                if (value != 0x22) {
                    enable_raml = 1;
                }
                LOG(("po2:%02x", value));
            } else {
                /* ram not mapped */
                enable_raml = 0;
                enable_ramh = 0;
            }
            LOG(("enable_raml:%d", enable_raml));

            LOG(("enable_ram_io1:%d", enable_ram_io1));
            if (enable_ram_io1) {
                enable_io1 = 1;
                enable_io2 = 0;
            } else {
                enable_io1 = 0;
                enable_io2 = 1;
            }
            /* in rr compatibility mode, limit ram to 32k */
            rambankl &= 3;
            rambankh &= 3;
            io1bank_ram = io1bank & 3;
            io2bank_ram = io2bank & 3;
        }

        ramA000_bank = rambankl;
        romA000_bank = cartbankl;
    }
    mmcreplay_ramlbank_set(rambankl);
    mmcreplay_ramhbank_set(rambankh);

    mmcreplay_io1bank_set(io1bank, io1bank_ram);
    mmcreplay_io2bank_set(io2bank, io2bank_ram);

    /* FIXME: phi1 should probably be different */
    active_mode_phi1 = (((mapped_exrom ^ 1) << 1) | mapped_game) | (cartbankl << CMODE_BANK_SHIFT);
    active_mode_phi2 = (((mapped_exrom ^ 1) << 1) | mapped_game) | (cartbankl << CMODE_BANK_SHIFT);

    if (release_freeze) {
        wflag |= CMODE_RELEASE_FREEZE;
    }

    cart_config_changed_slotmain(active_mode_phi1, active_mode_phi2, wflag);
    cart_romlbank_set_slotmain(cartbankl);
    cart_romhbank_set_slotmain(cartbankh);

    enable_freeze_exit = 0;     /* reset, it should only trigger once */
}

static void mmcreplay_update_mapper(unsigned int wflag, int release_freeze)
{
    mmcreplay_update_mapper_nolog(wflag, release_freeze);
#ifdef LOG_MAPPER
    mmcreplay_dump_cfg();
#endif
}

/********************************************************************************************************************
IO Area

Disabled register free the I/O memory which is underneath them.
FIXME: Disabling RR Register disables ALL ROM/RAM banking too.
********************************************************************************************************************/

#ifdef DEBUG_IOBANKS
int iobank_read = 0;
int iobank_write = 0;
#endif

/********************************************************************************************************************
  IO1 - $deXX
********************************************************************************************************************/

BYTE mmcreplay_io1_read(WORD addr)
{
    BYTE value;

    mmcreplay_io1_device.io_source_valid = 0;

    if (rr_active) {
        switch (addr & 0xff) {
            /* $DE00 / $DE01: RR control register read
             *
             * bit 0:     0 = Flash write protected, 1 = Flash write enabled
             * bit 1:     0 = always map first ram bank in i/o space, 1 = enable io ram banking (R)
             * bit 2:     1 = freeze button pressed
             * bit 3:     bank address 13 (R)
             * bit 4:     bank address 14 (R)
             * bit 5:     ** ALWAYS 0 **
             * bit 6:     0 = ram/rom @ DFxx, 1 = ram/rom @ $DExx (R)
             * bit 7:     bank address 15 (R)
             */
            case 0:
            case 1:
                mmcreplay_io1_device.io_source_valid = 1;
                value = ((bank_address_13_15 & 3) << 3) | /* bit 3,4 */
                        (enable_flash_write) |  /* bit 0 */
                        (allow_bank << 1) |     /* bit 1 */
                        (freeze_pressed << 2) | /* bit 2 */
                        (enable_ram_io1 << 6) | /* bit 6 */
                        ((bank_address_13_15 & 4) << 5); /* bit 7 */
#ifdef LOG_READ_DE00
                LOG(("MMCREPLAY: IO1 RD %04x %02x enable_flash_write %x allow_bank %x freeze_pressed %x bank_address_13_15 %x enable_ram_io1 %x", addr, value, enable_flash_write, allow_bank, freeze_pressed, bank_address_13_15, enable_ram_io1));
#endif
                return value;

            default:
                /*
                 * $DE02 - $DE0F: Clockport memory area (when enabled)
                 */
                if (mmcr_clockport_enabled) {
                    if ((addr & 0xff) < 0x10) {
#ifdef LOG_CLOCKPORT
                        LOG(("MMCREPLAY: Clockport RD %04x", addr));
#endif
                        return 0;
                    }
                }
                break;
        }

        if (enable_io1) {
            mmcreplay_io1_device.io_source_valid = 1;

            if (enable_ram_io) {
#ifdef LOG_READ_IO1_RAM
                LOG(("MMCREPLAY: RAM IO1 RD %04x %02x (%02x:%04x)", addr,
                     mmcr_ram[(addr & 0x1fff) + (io1_ram_bank << 13)],
                     io1_ram_bank, (addr & 0x1fff)));
#endif
                return mmcr_ram[(io1_ram_bank << 13) + 0x1e00 +
                                (addr & 0xff)];
            }
#ifdef LOG_READ_IO1_ROM
            LOG(("MMCREPLAY: ROM IO1 RD %04x %02x (%02x:%04x)", addr,
                 roml_banks[(addr & 0x1fff) + (io1_bank << 13)], io1_bank,
                 (addr & 0x1fff)));
#endif
            return flash040core_read(flashrom_state, (io1_ram_bank << 13) + 0x1e00 + (addr & 0xff));
        } else {
#ifdef LOG_READ_IO1_DISABLED
            LOG(("MMCREPLAY: DISABLED IO1 RD %04x %02x (%02x:%04x)", addr,
                 roml_banks[(addr & 0x1fff) + (io1_bank << 13)], io1_bank,
                 (addr & 0x1fff)));
#endif
        }
    } else {
        /* FIXME: when RR regs are disabled, is the mapped RAM still there ? */
        if (enable_io1) {
            mmcreplay_io1_device.io_source_valid = 1;

            if (enable_ram_io) {
#ifdef LOG_READ_IO1_RAM
                LOG(("MMCREPLAY: RAM IO1 RD %04x %02x (%02x:%04x)", addr,
                     mmcr_ram[(addr & 0x1fff) + (io1_ram_bank << 13)],
                     io1_ram_bank, (addr & 0x1fff)));
#endif
                return mmcr_ram[(io1_ram_bank << 13) + 0x1e00 +
                                (addr & 0xff)];
            }
#ifdef LOG_READ_IO1_ROM
            LOG(("MMCREPLAY: ROM IO1 RD %04x %02x (%02x:%04x)", addr,
                 roml_banks[(addr & 0x1fff) + (io1_bank << 13)], io1_bank,
                 (addr & 0x1fff)));
#endif
            return flash040core_read(flashrom_state, (io1_ram_bank << 13) + 0x1e00 + (addr & 0xff));
        } else {
#ifdef LOG_READ_IO1_DISABLED
            LOG(("MMCREPLAY: DISABLED IO1 RD %04x %02x (%02x:%04x)", addr,
                 roml_banks[(addr & 0x1fff) + (io1_bank << 13)], io1_bank,
                 (addr & 0x1fff)));
#endif
        }
    }
    return 0;
}

void mmcreplay_io1_store(WORD addr, BYTE value)
{
    if (rr_active) {
        switch (addr & 0xff) {
            case 0:
                /* $DE00:  RR control register write
                 *
                 * bit 0:     GAME line (W)
                 * bit 1:     EXROM line (W)
                 * bit 2:     1 = disable RR (W) (*)
                 * bit 3:     bank address 13 (W)
                 * bit 4:     bank address 14 (W)
                 * bit 5:     0 = rom enable, 1 = ram enable (W)
                 * bit 6:     1 = exit freeze mode (W)
                 * bit 7:     bank address 15 (W)
                 * (*) bit can be reset by setting bit 6 of $df12
                 */

                enable_game = value & 1;        /* bit 0 */
                enable_exrom = ((value >> 1) & 1) ^ 1;  /* bit 1 FIXME: is the bit inverted in the register ?! */
                bank_address_13_15 = (((value >> 3) & 3) | ((value >> 5) & 4)); /* bit 3,4,7 */
                rr_active_bit = value & 4;      /* bit 2 */
                if (rr_active_bit) {
                    rr_active = 0;
                }
                enable_ram_io = (value >> 5) & 1;       /* bit 5 */
                enable_freeze_exit = (value >> 6) & 1;  /* bit 6 */
#ifdef LOG_WRITE_DE00
                LOG(("MMCREPLAY: IO1 ST %04x %02x enable_game %x enable_exrom %x disable freezer regs %x bank_address_13_15 %x enable_ram_io %x enable_freeze_exit %x", addr, value, enable_game, enable_exrom, value & 4, bank_address_13_15, enable_ram_io, enable_freeze_exit));
#endif
                mmcreplay_update_mapper(CMODE_WRITE, enable_freeze_exit);
                return;

            case 1:
                /*
                 * $DE01: extended RR control register write
                 *        --------------------------------------
                 *        bit 0:  0 = disable clockport, 1 = enable clockport (W)
                 *   (*2) bit 1:  0 = disable io ram banking, 1 = enable io ram banking (W)
                 *   (*2) bit 2:  0 = enable freeze, 1 = disable freeze (W)
                 *        bit 3:  bank address 13 (mirror of $DE00) (W)
                 *        bit 4:  bank address 14 (mirror of $DE00) (W)
                 *   (*2) bit 5:  0 = enable MMC registers, 1 = disable MMC registers (W)(*1)
                 *   (*2) bit 6:  0 = ram/rom @ DFxx, 1 = ram/rom @ $DExx (W)
                 *        bit 7:  bank address 15 (mirror of $DE00) (W)
                 *
                 * (*1) Can only be written when bit 6 of $DF12 is 1. Register becomes effective
                 *      when bit 0 of $DF11 is 1.
                 * (*2) these bits are write-once if bit 0 of $DF11 is 1
                 */

                /* Every bit in $de01 can always be altered in Super Mapper mode+MMC Bios Mode. */
                bank_address_13_15 = (((value >> 3) & 3) | ((value >> 5) & 4)); /* bit 3,4,7 */
                if (mmcr_clockport_enabled != (value & 1)) {
                    mmcr_clockport_enabled = value & 1; /* bit 0 */
                }

                /* bits 1,2,5,6 are "write once" if not in mmc bios mode */
                if ((write_once == 0) || (disable_mmc_bios == 0)) {
                    allow_bank = (value >> 1) & 1;      /* bit 1 */
                    no_freeze = (value >> 2) & 1;       /* bit 2 */
                    enable_ram_io1 = (value >> 6) & 1;  /* bit 6 */
                    if ((enable_extended_mode == 1)
                        || (enable_16k_mapping == 1)) {
                        /* FIXME: documentation says that this bit becomes effective when df11 bit0 is 1 */
                        /*bit 5:        0 = enable MMC registers, 1 = disable MMC registers (W) */
                        enable_mmc_regs_pending = ((value >> 5) & 1) ^ 1;       /* bit 5 */
                        if (disable_mmc_bios == 1) {
                            enable_mmc_regs = ((value >> 5) & 1) ^ 1;   /* bit 5 */
                        }
                    }
                    if (disable_mmc_bios != 0) {
                        write_once = 1;
                    }
                }
#ifdef LOG_WRITE_DE01
                LOG(("MMCREPLAY: IO1 ST %04x %02x mmcr_clockport_enabled %x allow_bank %x no_freeze %x bank_address_13_15 %x enable_mmc_regs_pending %x enable_ram_io1 %x", addr, value, mmcr_clockport_enabled, allow_bank, no_freeze, bank_address_13_15, enable_mmc_regs_pending, enable_ram_io1));
#endif
                mmcreplay_update_mapper(CMODE_WRITE, 0);
                return;

            default:
                /*
                 * $DE02 - $DE0F: Clockport memory area (when enabled)
                 */
                if (mmcr_clockport_enabled) {
                    if ((addr & 0xff) < 0x10) {
#ifdef LOG_CLOCKPORT
                        LOG(("MMCREPLAY: Clockport ST %04x %02x", addr, value));
#endif
                        return;
                    }
                }
                break;
        }

        if (enable_io1) {
            if (enable_ram_io) {
#ifdef LOG_WRITE_IO1_RAM
                LOG(("store RAM IO1 %04x %02x (%02x:%04x)", addr, value,
                     io1_ram_bank, (addr & 0x1fff)));
#endif
                mmcr_ram[(io1_ram_bank << 13) + 0x1e00 + (addr & 0xff)] =
                    value;
            } else {
#ifdef LOG_WRITE_IO1_ROM
                LOG(("store DISABLED RAM IO1 %04x %02x (%02x:%04x)", addr,
                     value, io1_ram_bank, (addr & 0x1fff)));
#endif
                flash040core_store(flashrom_state, (io1_ram_bank << 13) + 0x1e00 + (addr & 0xff), value);
            }
        } else {
#ifdef LOG_WRITE_IO1_DISABLED
            LOG(("store DISABLED IO1 %04x %02x (%02x:%04x)", addr, value,
                 io1_bank, (addr & 0x1fff)));
#endif
        }
    } else {                   /* rr active */
        /* FIXME: when RR regs are disabled, is the mapped RAM still there ? */
        if (enable_io1) {
            if (enable_ram_io) {
#ifdef LOG_WRITE_IO1_RAM
                LOG(("store RAM IO1 %04x %02x (%02x:%04x)", addr, value,
                     io1_ram_bank, (addr & 0x1fff)));
#endif
                mmcr_ram[(io1_ram_bank << 13) + 0x1e00 + (addr & 0xff)] = value;
            } else {
#ifdef LOG_WRITE_IO1_ROM
                LOG(("store DISABLED RAM IO1 %04x %02x (%02x:%04x)", addr,
                     value, io1_ram_bank, (addr & 0x1fff)));
#endif
                flash040core_store(flashrom_state, (io1_ram_bank << 13) + 0x1e00 + (addr & 0xff), value);
            }
        } else {
#ifdef LOG_WRITE_IO1_DISABLED
            LOG(("store DISABLED IO1 %04x %02x (%02x:%04x)", addr, value,
                 io1_bank, (addr & 0x1fff)));
#endif
        }
    }
}

/********************************************************************************************************************
  IO2 - $dfXX
********************************************************************************************************************/

BYTE mmcreplay_io2_read(WORD addr)
{
    BYTE value;

    mmcreplay_io2_device.io_source_valid = 0;

    switch (addr & 0xff) {
        case 0x10:
            /*
             * $DF10: MMC SPI transfer register
             *
             * response from the card is read here
             */
            if (enable_mmc_regs) {
                mmcreplay_io2_device.io_source_valid = 1;

                value = spi_mmc_data_read();
#ifdef LOG_READ_DF10
                LOG(("MMCREPLAY: IO2 RD %04x %02x", addr, value));
#endif
                return value;
            }
            break;
        case 0x11:
            /*
             * $DF11: MMC control register
             *        ------------------------
             *        bit 0:  0 = MMC BIOS enabled, 1 = MMC BIOS disabled                   (R/W) (*)
             *        bit 1:  0 = card selected, 1 = card not selected                      (R/W) (**)
             *        bit 2:  0 = 250khz transfer, 1 = 8mhz transfer                        (R/W)
             *        bit 3:  ** ALWAYS 0 **
             *        bit 4:  ** ALWAYS 0 **
             *        bit 5:  0 = allow RR rom when MMC BIOS disabled , 1 = disable RR ROM  (R/W)  (***)
             *        bit 6:  0 = SPI write trigger mode, 1 = SPI read trigger mode         (R/W)
             *        bit 7:  ** ALWAYS 0 **
             *
             * (*)   Enabling MMC Bios sets ROM banking to the last 64K bank
             * (**)  This bit also controls the green activity LED.
             * (***) When in mmcreplay bios mode, bit 5 controls RAM banking (0 = $e000 - $ffff, 1 = $8000 - $9fff)
             *       When in 16K mode, bit 5 enables RAM at $a000 - $bfff
             */
            if (enable_mmc_regs) {
                mmcreplay_io2_device.io_source_valid = 1;
                value = disable_mmc_bios;       /* bit 0 */
                value |= (spi_mmc_card_selected_read() << 1);  /* bit 1 */
                value |= (spi_mmc_enable_8mhz_read() << 2);    /* bit 2 */
                /* bit 3,4 always 0 */
                value |= (disable_rr_rom << 5); /* bit 5 */
                value |= (spi_mmc_trigger_mode_read() << 6);   /* bit 6 */
                /* bit 7 always 0 */
#ifdef LOG_READ_DF11
                LOG(("MMCREPLAY: IO2 RD %04x %02x disable_mmc_bios %x disable_rr_rom %x", addr, value, disable_mmc_bios, disable_rr_rom));
#endif
                return value;
            }
            break;
        case 0x12:
            /*
             * $DF12: MMC status register
             *        -----------------------
             *        bit 0:  0 = SPI ready, 1 = SPI busy                       (R)
             *                1 = forbid ROM write accesses                     (W) (*)
             *        bit 1:  feedback of $DE00 bit 0 (GAME)                    (R)
             *        bit 2:  feedback of $DE00 bit 1 (EXROM)                   (R)
             *        bit 3:  0 = card inserted, 1 = no card inserted           (R)
             *        bit 4:  0 = card write enabled, 1 = card write disabled   (R)
             *        bit 5:  EEPROM DATA line / DDR Register                   (R/W) (**)
             *        bit 6:  0 = RR compatibility mode, 1 = Extended mode      (W) (***)
             *        bit 7:  EEPROM CLK line                                   (W)
             *
             * (*)   Setting this bit will disable writes to ROM until next reset.
             * (**)  Setting DATA to "1" enables reading data bit from EEPROM at this position.
             * (***) Selecting RR compatibility mode limits RAM to 32K and
             *       disables writes to extended banking register.
             *       Selecting Extended mode enables full RAM banking and enables
             *       Nordic Power mode in RR mode.
             */
            if (enable_mmc_regs) {
                mmcreplay_io2_device.io_source_valid = 1;

                value = 0;
                /* EEPROM is only accessible in MMC Replay Bios mode */
                if (disable_mmc_bios == 0) {
                    value = eeprom_data_read() << 5;   /* bit 5 */
                }
                value |= (spi_mmc_busy());     /* bit 0 */
                value |= (enable_game << 1);    /* bit 1 */
                value |= (enable_exrom ^ 1) << 2;       /* bit 2 FIXME: inverted in reg ? */
                value |= (spi_mmc_card_inserted() ^ 1) << 3;   /* bit 3 */
                value |= (spi_mmc_card_write_enabled() ^ 1) << 4;      /* bit 4 */
                /* bit 6,7 not readable */
#ifdef LOG_READ_DF12
                LOG(("MMCREPLAY: IO2 RD %04x %02x enable_game %x enable_exrom %x", addr, value, enable_game, (enable_exrom ^ 1)));
#endif
                return value;
            }
            break;
        case 0x13:
            /*
             * $DF13: Extended banking register (*)
             *        -----------------------------
             *        bit 0: bank address 16        (R/W)
             *        bit 1: bank address 17        (R/W)
             *        bit 2: bank address 18        (R/W)
             *        bit 3: ** ALWAYS 0 **
             *        bit 4: ** ALWAYS 0 **
             *        bit 5: 16K rom mapping        (R/W)
             *        bit 6: 1 = enable RR register (W)
             *        bit 7: ** ALWAYS 0 **
             *
             * (*) Can only be read/written to when bit 6 of $DF12 is 1.
             */

            if (enable_extended_mode) {
                mmcreplay_io2_device.io_source_valid = 1;

                value = bank_address_16_18;     /* bit 0-2 */
                /* bit 3,4 always 0 */
                value |= enable_16k_mapping << 5;       /* bit 5 */
                /* bit 7 always 0 */
#ifdef LOG_READ_DF13
                LOG (("MMCREPLAY: IO2 RD %04x %02x bank_address_16_18 %x enable_16k_mapping %x", addr, value, bank_address_16_18, enable_16k_mapping));
#endif
                return value;
            } else {
#ifdef LOG_READ_DF13
                LOG(("MMCREPLAY: DISABLED IO2 RD %04x", addr));
#endif
            }
            break;
    }

    if (enable_io2) {
        mmcreplay_io2_device.io_source_valid = 1;

        if (enable_ram_io) {
#ifdef LOG_READ_IO2_RAM
            LOG(("MMCREPLAY: RAM IO2 RD %04x %02x (%02x:%04x)", addr,
                 mmcr_ram[(addr & 0x1fff) + (io2_ram_bank << 13)],
                 io2_ram_bank, (addr & 0x1fff)));
#endif
            return mmcr_ram[(io2_ram_bank << 13) + 0x1f00 + (addr & 0xff)];
        }
#ifdef LOG_READ_IO2_ROM
        LOG(("MMCREPLAY: ROM IO2 RD %04x %02x (%02x:%04x)", addr,
             roml_banks[(addr & 0x1fff) + (io2_bank << 13)], io2_bank,
             (addr & 0x1fff)));
#endif
        return flash040core_read(flashrom_state, (io2_ram_bank << 13) + 0x1f00 + (addr & 0xff));
    } else {
#ifdef LOG_READ_IO2_DISABLED
        LOG(("MMCREPLAY: DISABLED IO2 RD %04x %02x (%02x:%04x)", addr,
             roml_banks[(io2_ram_bank << 13) + 0x1f00 + (addr & 0xff)],
             io2_bank, (addr & 0x1fff)));
#endif
    }

    return 0;
}

void mmcreplay_io2_store(WORD addr, BYTE value)
{
    switch (addr & 0xff) {
        case 0x10:
            /*
             * $DF10: MMC SPI transfer register
             *
             * byte written is sent to the card
             */
            if (enable_mmc_regs) {
#ifdef LOG_WRITE_DF10
                LOG(("MMCREPLAY: IO2 ST %04x %02x", addr, value));
#endif
                spi_mmc_data_write(value);
                return;
            }
            break;
        case 0x11:
            /*
             * $DF11: MMC control register
             *        ------------------------
             *        bit 0:  0 = MMC BIOS enabled, 1 = MMC BIOS disabled                    (R/W) (*)
             *        bit 1:  0 = card selected, 1 = card not selected                       (R/W) (**)
             *        bit 2:  0 = 250khz transfer, 1 = 8mhz transfer                         (R/W)
             *        bit 3:  ** ALWAYS 0 **
             *        bit 4:  ** ALWAYS 0 **
             *        bit 5:  0 = allow RR rom when MMC BIOS disabled , 1 = disable RR ROM   (R/W) (***)
             *        bit 6:  0 = SPI write trigger mode, 1 = SPI read trigger mode          (R/W)
             *        bit 7:  ** ALWAYS 0 **
             *
             * (*)   Enabling MMC Bios sets ROM banking to the last 64K bank
             * (**)  This bit also controls the green activity LED.
             * (***) When in mmcreplay bios mode, bit 5 controls RAM banking (0 = $e000 - $ffff, 1 = $8000 - $9fff)
             *       When in 16K mode, bit 5 enables RAM at $a000 - $bfff
             */
            if (enable_mmc_regs) {
                disable_mmc_bios = (value) & 1; /* bit 0 */
                disable_rr_rom = (value >> 5) & 1;      /* bit 5 */

#ifdef LOG_WRITE_DF11
                LOG(("MMCREPLAY: IO2 ST %04x %02x disable_mmc_bios %x disable_rr_rom %x", addr, value, disable_mmc_bios, disable_rr_rom));
#endif

                spi_mmc_card_selected_write((BYTE)(((value >> 1) ^ 1) & 1));   /* bit 1 */
                spi_mmc_enable_8mhz_write((BYTE)(((value >> 2)) & 1)); /* bit 2 */
                /* bit 3,4 always 0 */
                spi_mmc_trigger_mode_write((BYTE)(((value >> 6)) & 1));        /* bit 6 */
                /* bit 7 always 0 */
                if (disable_mmc_bios) {
                    /* if (enable_mmc_regs_pending) */
                    {
                        enable_mmc_regs = enable_mmc_regs_pending;
                    }
                } else {
                    /* clearing bit 0 resets write once for de01 */
                    write_once = 0;
                }
                mmcreplay_update_mapper(CMODE_WRITE, 0);
                return;
            }
            break;
        case 0x12:
            /*
             * $DF12: MMC status register
             *        -----------------------
             *        bit 0:  0 = SPI ready, 1 = SPI busy                        (R)
             *                1 = forbid ROM write accesses                      (W) (*)
             *        bit 1:  feedback of $DE00 bit 0 (GAME)                     (R)
             *        bit 2:  feedback of $DE00 bit 1 (EXROM)                    (R)
             *        bit 3:  0 = card inserted, 1 = no card inserted            (R)
             *        bit 4:  0 = card write enabled, 1 = card write disabled    (R)
             *        bit 5:  EEPROM DATA line / DDR Register                    (R/W) (**)
             *        bit 6:  0 = RR compatibility mode, 1 = Extended mode       (W) (***)
             *        bit 7:  EEPROM CLK line                                    (W)
             *
             * (*)   Setting this bit will disable writes to ROM until next reset.
             * (**)  Setting DATA to "1" enables reading data bit from EEPROM at this position.
             * (***) Selecting RR compatibility mode limits RAM to 32K and
             *       disables writes to extended banking register.
             *       Selecting Extended mode enables full RAM banking and enables
             *       Nordic Power mode in RR mode.
             */
            if (enable_mmc_regs) {
                /* FIXME bit 0: forbid write access to ROM */

                disable_rom_write = (value ^ 1) & 1;    /* bit 0 */
                enable_extended_mode = (value >> 6) & 1;        /* bit 6 */
                if (enable_extended_mode) {
                    rr_active = 1;
                }
                /* bit 2 ? FIXME */
                /* bit 3 ? FIXME */
#ifdef LOG_WRITE_DF12
                LOG(("MMCREPLAY: IO2 ST %04x %02x disable_rom_write %x enable_extended_mode %x", addr, value, disable_rom_write, enable_extended_mode));
#endif

                /* EEPROM is only accessible in MMC Replay Bios mode */
                if (disable_mmc_bios == 0) {
                    /*
                     * bit 1: ddr FIXME
                     * bit 4: status
                     * bit 5: data/ddr
                     * bit 7: clk
                     */
                    eeprom_port_write((BYTE)((value >> 7) & 1), (BYTE)((value >> 5) & 1),
                                      (value >> 1) & 1, (value >> 4) & 1);
                }

                mmcreplay_update_mapper(CMODE_WRITE, 0);
                return;
            }
            break;
        case 0x13:
            /*
             * $DF13: Extended banking register (*)
             *        -----------------------------
             *        bit 0: bank address 16        (R/W)
             *        bit 1: bank address 17        (R/W)
             *        bit 2: bank address 18        (R/W)
             *        bit 3: ** ALWAYS 0 **
             *        bit 4: ** ALWAYS 0 **
             *        bit 5: 16K rom mapping        (R/W)
             *        bit 6: 1 = enable RR register (W)
             *        bit 7: ** ALWAYS 0 **
             *
             * (*) Can only be read/written to when bit 6 of $DF12 is 1.
             */

            if (enable_mmc_regs) {
                /* $df13 can only be READ AND WRITTEN if bit6 of $df12 = 1 */
                if (enable_extended_mode) {
                    bank_address_16_18 = (value) & 7;   /* bit 0-2 */
                    enable_16k_mapping = (value >> 5) & 1;      /* bit 5 */
                    enable_rr_regs = (value >> 6) & 1;  /* bit 6 */
                    if ((value >> 6) & 1) {      /* bit 6 */
                        /* re-enable RR cartridge */
                        rr_active = 1;
                    }
#ifdef LOG_WRITE_DF13
                    LOG(("MMCREPLAY: IO2 ST %04x %02x bank_address_16_18 %x enable_16k_mapping %x enable rr regs %x", addr, value, bank_address_16_18, enable_16k_mapping, ((value >> 6) & 1)));
#endif
                    mmcreplay_update_mapper(CMODE_WRITE, 0);
                    return;
                }
            }

            break;
    }

    if (enable_io2) {
        if (enable_ram_io) {
#ifdef LOG_WRITE_IO2_RAM
            LOG(("MMCREPLAY: RAM IO2 ST %04x %02x (%02x:%04x)", addr, value,
                 io2_ram_bank, (addr & 0x1fff)));
#endif
            mmcr_ram[(io2_ram_bank << 13) + 0x1f00 + (addr & 0xff)] = value;
        } else {
#ifdef LOG_WRITE_IO2_ROM
            LOG(("MMCREPLAY: ROM IO2 ST %04x %02x (%02x:%04x)",
                 addr, value, io2_ram_bank, (addr & 0x1fff)));
#endif
            flash040core_store(flashrom_state, (io2_ram_bank << 13) + 0x1f00 + (addr & 0xff), value);
        }
    } else {
#ifdef LOG_WRITE_IO2_DISABLED
        LOG(("MMCREPLAY: DISABLED IO2 ST %04x %02x (%02x:%04x)", addr, value,
             io2_bank, (addr & 0x1fff)));
#endif
    }
}

/********************************************************************************************************************
  ClockPort
********************************************************************************************************************/

static BYTE mmcreplay_clockport_read(WORD address)
{
    if (clockport_device) {
        if (address < 0x02) {
            mmcreplay_clockport_device.io_source_valid = 0;
            return 0;
        }
        return clockport_device->read(address, &mmcreplay_clockport_device.io_source_valid, clockport_device->device_context);
    }
    return 0;
}

static BYTE mmcreplay_clockport_peek(WORD address)
{
    if (clockport_device) {
        if (address < 0x02) {
            return 0;
        }
        return clockport_device->peek(address, clockport_device->device_context);
    }
    return 0;
}

static void mmcreplay_clockport_store(WORD address, BYTE byte)
{
    if (clockport_device) {
        if (address < 0x02) {
            return;
        }

        clockport_device->store(address, byte, clockport_device->device_context);
    }
}


/********************************************************************************************************************
  Dump
********************************************************************************************************************/

static int mmcreplay_dump(void)
{
    /* FIXME: incomplete */
    /* mon_out("MMC Replay registers are %s.\n", mmcr_active ? "enabled" : "disabled"); */
    mon_out("Clockport is %s.\n", mmcr_clockport_enabled ? "enabled" : "disabled");
    mon_out("Clockport device: %s.\n", clockport_device_id_to_name(clockport_device_id));

    return 0;
}

/********************************************************************************************************************
MEM Area
********************************************************************************************************************/
#ifdef DEBUG_LOGBANKS
int logbank_read = 0;
int logbank_write = 0;
#endif
/*
    $1000-$7fff in ultimax mode - this is always regular ram
*/
BYTE mmcreplay_1000_7fff_read(WORD addr)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_read != (addr & 0xe000)) {
        LOG(("MMCREPLAY: RAM  RD %04x", addr));
        logbank_read = (addr & 0xe000);
    }
#endif
    if (ultimax_mapping_hack) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

void mmcreplay_1000_7fff_store(WORD addr, BYTE value)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_write != (addr & 0xe000)) {
        LOG(("MMCREPLAY: RAM  ST %04x %02x", addr, value));
        logbank_write = (addr & 0xe000);
    }
#endif
    if (ultimax_mapping_hack) {
        mem_store_without_ultimax(addr, value);
    }
}

/*
    ROML - $8000-$9fff
*/

/* FIXME: something with checking pport.data is wrong, the seperate check
          for the rescue mode should not be necessary */
BYTE mmcreplay_roml_read(WORD addr)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_read != (addr & 0xe000)) {
        if (enable_raml) {
            LOG(("MMCREPLAY: RAML RD %04x %02x (%02x:%04x)", addr,
                 mmcr_ram[(addr & 0x1fff) + (raml_bank << 13)], raml_bank,
                 (addr & 0x1fff)));
            logbank_read = (addr & 0xe000);
        } else {
/*
            LOG(("MMCREPLAY: ROML RD %04x %02x (%02x:%04x)", addr,roml_banks[(addr & 0x1fff) + (roml_bank << 13)],roml_bank,(addr & 0x1fff)));
            logbank_read = (addr&0xe000);
*/
        }
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 3) == 3) {     /* if LoRAM + HiRAM */
            if (enable_raml) {
                return mmcr_ram[(addr & 0x1fff) + (raml_bank << 13)];
            }
            return flash040core_read(flashrom_state, (addr & 0x1fff) + (roml_bank << 13));
        } else if (enable_rescue_mode) {
            return flash040core_read(flashrom_state, (addr & 0x1fff) + (roml_bank << 13));
        }

        return mem_read_without_ultimax(addr);
    }

    if (enable_raml) {
        return mmcr_ram[(addr & 0x1fff) + (raml_bank << 13)];
    }
    return flash040core_read(flashrom_state, (addr & 0x1fff) + (roml_bank << 13));
}

/* FIXME: something with checking pport.data is wrong, the seperate check
          for the rescue mode should not be necessary */
void mmcreplay_roml_store(WORD addr, BYTE value)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_write != (addr & 0xe000)) {
        if (enable_raml) {
            LOG(("MMCREPLAY: RAML ST %04x %02x (%02x:%04x)", addr, value,
                 raml_bank, (addr & 0x1fff)));
        } else {
            LOG(("MMCREPLAY: DISABLED! RAML ST  %04x %02x (%02x:%04x)", addr,
                 value, raml_bank, (addr & 0x1fff)));
        }
        logbank_write = (addr & 0xe000);
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 3) == 3) {     /* if LoRAM + HiRAM */
            if (enable_raml) {
                mmcr_ram[(addr & 0x1fff) + (raml_bank << 13)] = value;
            } else {
                flash040core_store(flashrom_state, (addr & 0x1fff) + (roml_bank << 13), value);
            }
        } else {
            if (enable_rescue_mode) {
                flash040core_store(flashrom_state, (addr & 0x1fff) + (roml_bank << 13), value);
            }
            mem_store_without_ultimax(addr, value);
        }
    } else {
        if (enable_raml) {
            mmcr_ram[(addr & 0x1fff) + (raml_bank << 13)] = value;
        } else {
            flash040core_store(flashrom_state, (addr & 0x1fff) + (roml_bank << 13), value);
        }
    }
}

/*
    $a000 in ultimax mode
*/
BYTE mmcreplay_a000_bfff_read(WORD addr)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_read != (addr & 0xe000)) {
        LOG(("MMCREPLAY: RD %04x", addr));
        logbank_read = (addr & 0xe000);
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 3) == 3) {     /* if LoRAM + HiRAM */
            if (!disable_mmc_bios) {
                if (enable_ramA000) {
                    return mmcr_ram[(addr & 0x1fff) + (ramA000_bank << 13)];
                }
                return flash040core_read(flashrom_state, (addr & 0x1fff) + (romA000_bank << 13));
            }
        }
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

void mmcreplay_a000_bfff_store(WORD addr, BYTE value)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_write != (addr & 0xe000)) {
        LOG(("MMCREPLAY: ST %04x", addr));
        logbank_write = (addr & 0xe000);
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 3) == 3) {     /* if LoRAM + HiRAM */
            if (!disable_mmc_bios) {
                /*
                 * if (enable_ramA000)
                 * {
                 * LOG (("store RAM A000 %04x %02x (%02x:%04x)", addr, value,
                 * ramA000_bank, (addr & 0x1fff)));
                 * mmcr_ram[(addr & 0x1fff) + (ramA000_bank << 13)] = value;
                 * }
                 * else
                 * {
                 * LOG (("store DISABLED RAM A000 %04x %02x (%02x:%04x)", addr, value,
                 * romA000_bank, (addr & 0x1fff)));
                 * mem_store_without_ultimax(addr, value);
                 * }
                 */
                /* there won't be a chipselect generated for write, so writes
                 * always go to C64 memory */
                mem_store_without_ultimax(addr, value);
            }
        } else {
            mem_store_without_ultimax(addr, value);
        }
    }
}

/*
    $c000 in ultimax mode - this is always regular ram
*/

BYTE mmcreplay_c000_cfff_read(WORD addr)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_read != (addr & 0xe000)) {
        LOG(("MMCREPLAY: RAM  RD %04x", addr));
        logbank_read = (addr & 0xe000);
    }
#endif
    if (ultimax_mapping_hack) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

void mmcreplay_c000_cfff_store(WORD addr, BYTE value)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_write != (addr & 0xe000)) {
        LOG(("MMCREPLAY: RAM  ST %04x %02x", addr, value));
        logbank_write = (addr & 0xe000);
    }
#endif
    if (ultimax_mapping_hack) {
        mem_store_without_ultimax(addr, value);
    }
}

/*
    ROMH - $a000 ($e000 in ultimax mode)
*/

BYTE mmcreplay_romh_read(WORD addr)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_read != (addr & 0xe000)) {
        if (enable_ramh) {
            if (addr < 0xfff0) {
                LOG(("MMCREPLAY: RAMH RD %04x %02x (%02x:%04x)", addr,
                     mmcr_ram[(addr & 0x1fff) + (ramh_bank << 13)],
                     ramh_bank, (addr & 0x1fff)));
                logbank_read = (addr & 0xe000);
            }
        } else {
            LOG (("MMCREPLAY: ROMH RD %04x %02x (%02x:%04x)", addr,
                  roml_banks[(addr & 0x1fff) + (romh_bank << 13)], romh_bank,
                  (addr & 0x1fff)));
            logbank_read = (addr & 0xe000);
        }
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 2) == 2) {     /* if HiRAM */
            if (enable_ramh) {
                return mmcr_ram[(addr & 0x1fff) + (ramh_bank << 13)];
            }

            return flash040core_read(flashrom_state, (addr & 0x1fff) + (romh_bank << 13));
        }

        return mem_read_without_ultimax(addr);
    }

    if (enable_ramh) {
        return mmcr_ram[(addr & 0x1fff) + (ramh_bank << 13)];
    }

    return flash040core_read(flashrom_state, (addr & 0x1fff) + (romh_bank << 13));
}

void mmcreplay_romh_store(WORD addr, BYTE value)
{
#ifdef DEBUG_LOGBANKS
    if (logbank_write != (addr & 0xe000)) {
        if (enable_ramh) {
            LOG(("MMCREPLAY: RAMH ST %04x %02x (%02x:%04x)", addr, value,
                 ramh_bank, (addr & 0x1fff)));
        } else {
            LOG (("MMCREPLAY: DISABLED! RAMH ST %04x %02x (%02x:%04x)", addr,
                  value, ramh_bank, (addr & 0x1fff)));
        }
        logbank_write = (addr & 0xe000);
    }
#endif

    if (ultimax_mapping_hack) {
        if ((pport.data & 2) == 2) {     /* if HiRAM */
            if (enable_ramh) {
                mmcr_ram[(addr & 0x1fff) + (ramh_bank << 13)] = value;
            } else {
                /* FIXME: write through to ram ? */
                /* mem_store_without_ultimax(addr, value); */
            }
        } else {
            mem_store_without_ultimax(addr, value);
        }
    }
}

int mmcreplay_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int mmcreplay_romh_phi2_read(WORD addr, BYTE *value)
{
    return mmcreplay_romh_phi1_read(addr, value);
}

/*********************************************************************************************************************

*********************************************************************************************************************/

/*
The MMC-Replay Bios sets the following configurations:
ACTION REPLAY : Allowbank = 0, $df10-$df13 registers disabled. Nordic Power on.
RETRO REPLAY: $de01 unset, $df10-$df13 registers disabled
SUPER MAPPER: 16K mode, Standard cart mode, $df10-$df13 registers enabled
*/

void mmcreplay_set_stdcfg(void)
{
    enable_ram_io = 0;
    enable_ram_io1 = 0;
    enable_raml = 0;
    enable_ramh = 0;
    allow_bank = 0;

    enable_mmc_regs = 0;
    mmcr_clockport_enabled = 0;
    bank_address_13_15 = 0;

    /* start in 8k game config */
    enable_exrom = 1 ^ 1;
    enable_game = 0;

    write_once = 0;

    bank_address_16_18 = 7;
    rr_active = 1;
    enable_rr_regs = 1;
    enable_16k_mapping = 0;
    enable_extended_mode = 0;   /* enable nordic power mode */
    enable_mmc_regs = 1;
    no_freeze = 0;
    allow_bank = 0;

    enable_ram_io = 0;          /* ? */
    /* enable_ram_io1 = 0; */

#ifdef TEST_AR_MAPPER   /* bank 0 */
    bank_address_16_18 = 0;
    rr_active = 1;
    enable_rr_regs = 1;
    no_freeze = 0;
    allow_bank = 1;
    enable_ram_io = 0;
    enable_ram_io1 = 0;
    enable_16k_mapping = 0;
    disable_mmc_bios = 1;

    enable_exrom = 1;
    enable_game = 1;
#elif defined(TEST_RR_MAPPER)   /* bank 1 */
    bank_address_16_18 = 1;
    rr_active = 1;
    enable_rr_regs = 1;
    no_freeze = 0;
    /* allow_bank = 1; */
    enable_ram_io = 0;          /* mmmmh */
    enable_ram_io1 = 0;
    enable_16k_mapping = 0;
    disable_mmc_bios = 1;

    enable_exrom = 1;
    enable_game = 1;
#elif defined(TEST_NORDIC_MAPPER)       /* bank 2 */
    bank_address_16_18 = 2;
    rr_active = 1;
    enable_extended_mode = 1;   /* enable nordic power mode */
    enable_rr_regs = 1;
    no_freeze = 0;
    allow_bank = 0;
    /* enable_ram_io = 0; */
    enable_ram_io1 = 0;
    /* mmcreplay_config_changed((0<<1)|0, (0<<1)|0, CMODE_READ); */
    enable_16k_mapping = 0;
    disable_mmc_bios = 1;

    enable_exrom = 1;
    enable_game = 0;
#elif defined(TEST_SUPER_8KCRT) /* bank 2 */
    bank_address_16_18 = 2;
    rr_active = 1;
    enable_extended_mode = 0;   /* enable nordic power mode */
    enable_rr_regs = 0;
    no_freeze = 0;
    allow_bank = 0;
    enable_ram_io = 0;
    enable_ram_io1 = 0;
    enable_16k_mapping = 0;
    disable_mmc_bios = 0;

#elif defined(TEST_SUPER_16KCRT)        /* bank 3 */
    bank_address_16_18 = 3;
    rr_active = 1;
    enable_extended_mode = 0;   /* enable nordic power mode */
    enable_rr_regs = 0;
    no_freeze = 0;
    allow_bank = 0;
    enable_ram_io = 0;
    enable_ram_io1 = 0;
    enable_16k_mapping = 1;
    disable_mmc_bios = 0;

    enable_exrom = 1;
    enable_game = 1;
#elif defined(TEST_RESCUE_MODE)
/*
    enable_exrom = 1;
    enable_game = 0;
*/
    enable_rescue_mode = 1;
    enable_mmc_regs = 1;
    enable_mmc_regs_pending = 1;
#else
#endif
}

int mmcreplay_freeze_allowed(void)
{
    if (no_freeze) {
        return 0;
    }
    return 1;
}

/*

generally freezing ONLY affects de00 and de01, df10-df13 are never altered on freeze!

Freeze DOES NOT CLEAR banking bits 16-18.
Banking bits A13-A15 are cleared when freeze is pressed.
Ram is always enabled in freeze mode. The Ram bit ($de00 bit 5) however is reset
$df11 bit 5 is not changed when freeze, meaning: you can put freeze code in ram!

Super Mapper mapping in freeze mode
-----------------------------------

In freeze mode, the memory map is different than Retro Replay:

$de00: bit 5 = 0
----------------
$de00 - $dfff (minus registers): 512 byte ROM area (mirrored from $9e00-$9fff)

$de00: bit 5 = 1
----------------
Disables any memory at $de00-$dfff due to CPLD routing problems. This is however
no problem, since RAM is always enabled at $8000-$9fff if you need it.

$df11: bit 5 = 0
----------------
$8000 - $9fff: 8K RAM bank low
$e000 - $ffff: 8K RAM bank hi

$df11: bit 5 = 1
----------------
$8000 - $9fff: 8K RAM bank lo
$e000 - $ffff: 8K FLASH rom bank hi

*/
void mmcreplay_freeze(void)
{
    LOG(("MMCREPLAY: ----------------------------------------"));
    LOG(("MMCREPLAY: Freeze"));
    LOG(("MMCREPLAY: ----------------------------------------"));

    /*** $DE00:     RR control register write */
    /* bit 0:       GAME line (W) */
    enable_game = 1;            /* (w) status of game line */
    enable_flash_write = 0;     /* (r) */
    /* bit 1:       EXROM line (W) */
    enable_exrom = 1 ^ 1;       /* status of exrom line */
    /* bit 2:       1 = disable RR (W) */
    /*              (*) bit can be reset by setting bit 6 of $df12 */
    rr_active = 1;              /* (w) Cart is activated.  */

    /* we don't have a proper hook to release this bit after a while,
     * so we can only set it to 0 and hope for the best */
    /* freeze_pressed = 1; *//* (r) freeze button pressed.  */
    freeze_pressed = 0;
    /* ^ bit 3: bank address 13 (W) */
    /* ^ bit 4: bank address 14 (W) */
    /* bit 5:       0 = rom enable, 1 = ram enable (W) */
    enable_ram_io = 0;          /* reset on freeze */
    /* bit 6:       1 = exit freeze mode (W) */
    enable_freeze_exit = 0;
    /* bit 7:       bank address 15 (W) */
    bank_address_13_15 = 0;     /* always cleared when freezed */

    /*** $DE01:     extended RR control register write */
    /* bit 0:       0 = disable clockport, 1 = enable clockport (W) */
    mmcr_clockport_enabled = 0; /* used globally, eg in c64io.c */
    /* bit 1:       0 = disable io ram banking, 1 = enable io ram banking (W) */
    allow_bank = 1;             /* RAM bank switching allowed.  */
    /* bit 2:       0 = enable freeze, 1 = disable freeze (W) */
    no_freeze = 0;              /* Freeze is disallowed.  */
    /* ^ bit 3:     bank address 13 (mirror of $DE00) (W) */
    /* ^ bit 4:     bank address 14 (mirror of $DE00) (W) */
    /* bit 5:       0 = enable MMC registers, 1 = disable MMC registers (W) */
    /*              (*) Can only be written when bit 6 of $DF12 is 1. Register becomes effective
     * when bit 0 of $DF11 is 1. */
    enable_mmc_regs = 0;
    enable_mmc_regs_pending = 0;
    /* bit 6:   0 = ram/rom @ DFxx, 1 = ram/rom @ $DExx (W) */
    enable_ram_io1 = 0;         /* REU compatibility mapping.  */
    /* ^ bit 7: bank address 15 (mirror of $DE00) (W) */

#ifdef TEST_AR_MAPPER           /* bank 0 */
    enable_rr_regs = 1;
#elif defined(TEST_RR_MAPPER)   /* bank 4 */
    enable_ram_io = 1;          /* FIXME: should not actually be set here! */
    enable_rr_regs = 1;
#elif defined(TEST_NORDIC_MAPPER)       /* bank 1 */
    enable_rr_regs = 1;
#elif defined(TEST_SUPER_8KCRT) /* bank 2 */
    enable_rr_regs = 0;
#elif defined(TEST_SUPER_16KCRT)        /* bank 3 */
    enable_rr_regs = 0;
#else
    enable_rr_regs = 1;
#endif
    /* FIXME: OR 0x20 ? enable ram in freeze mode ? */
    mmcreplay_update_mapper(CMODE_READ, 0);
    flash040core_reset(flashrom_state);
}

/*
 generally on reset all registers will get initialized with 0, except

 $DE00: bit 2
 $DF11: bit 1
 $DF13: bit 0,1,2,5

 which will get initialized with 1
*/

void mmcreplay_reset(void)
{
    LOG(("MMCREPLAY: ----------------------------------------"));
    LOG(("MMCREPLAY: Reset"));
    LOG(("MMCREPLAY: ----------------------------------------"));

    rr_active = 1;

    /* 8 game */
    enable_exrom = 1 ^ 1;
    enable_game = 0;

    enable_ram_io = 0;
    enable_ram_io1 = 0;
    enable_raml = 0;
    enable_ramh = 0;
    allow_bank = 0;
    disable_mmc_bios = 0;
    enable_16k_mapping = 0;
    enable_extended_mode = 0;   /* enable nordic power mode */

    bank_address_13_15 = 0;
    bank_address_16_18 = 7;

    mmcreplay_set_stdcfg();

    if (enable_rescue_mode) {
        log_debug("MMCREPLAY: Rescue Mode enabled");
    }

    mmcreplay_update_mapper(CMODE_READ, 0);
    flash040core_reset(flashrom_state);

    if (mmcr_enabled && clockport_device) {
        clockport_device->reset(clockport_device->device_context);
    }
}


void mmcreplay_config_init(void)
{
#if 0
    /* for testing with the scanner */
    {
        int l, h, bank;
        for (h = 0; h < 8; h++) {
            for (l = 0; l < 8; l++) {
                bank = (h << 3) + l;
                mmcr_ram[0x0080 + (bank << 13)] = 1 + ('r' - 'a');
                mmcr_ram[0x0081 + (bank << 13)] = 1 + ('a' - 'a');
                mmcr_ram[0x0082 + (bank << 13)] = 0x30 + h;
                mmcr_ram[0x0083 + (bank << 13)] = 0x30 + l;
                mmcr_ram[0x1e80 + (bank << 13)] = 1 + ('r' - 'a');
                mmcr_ram[0x1e81 + (bank << 13)] = 1 + ('a' - 'a');
                mmcr_ram[0x1e82 + (bank << 13)] = 0x30 + h;
                mmcr_ram[0x1e83 + (bank << 13)] = 0x30 + l;
                mmcr_ram[0x1f80 + (bank << 13)] = 1 + ('r' - 'a');
                mmcr_ram[0x1f81 + (bank << 13)] = 1 + ('a' - 'a');
                mmcr_ram[0x1f82 + (bank << 13)] = 0x30 + h;
                mmcr_ram[0x1f83 + (bank << 13)] = 0x30 + l;
            }
        }
    }
#endif
    mmcreplay_set_stdcfg();
    mmcreplay_update_mapper(CMODE_READ, 0);

    flash040core_reset(flashrom_state);
}

void mmcreplay_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, MMCREPLAY_FLASHROM_SIZE);

    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_NORMAL, roml_banks);
    memcpy(flashrom_state->flash_data, rawcart, MMCREPLAY_FLASHROM_SIZE);

    mmcr_ram = lib_malloc(MMCREPLAY_RAM_SIZE);

    mmcreplay_set_stdcfg();
    mmcreplay_update_mapper(CMODE_READ, 0);
}

/* ------------------------------------------------------------------------- */

static int set_mmcr_clockport_device(int val, void *param)
{
    if (val == clockport_device_id) {
        return 0;
    }

    if (!mmcr_enabled) {
        clockport_device_id = val;
        return 0;
    }

    if (clockport_device_id != CLOCKPORT_DEVICE_NONE) {
        clockport_device->close(clockport_device);
        clockport_device_id = CLOCKPORT_DEVICE_NONE;
        clockport_device = NULL;
    }

    if (val != CLOCKPORT_DEVICE_NONE) {
        clockport_device = clockport_open_device(val, (char *)STRING_MMC_REPLAY);
        if (!clockport_device) {
            return -1;
        }
        clockport_device_id = val;
    }
    return 0;
}

static int clockport_activate(void)
{
    if (mmcr_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device = clockport_open_device(clockport_device_id, (char *)STRING_MMC_REPLAY);
    if (!clockport_device) {
        return -1;
    }
    return 0;
}

static int clockport_deactivate(void)
{
    if (!mmcr_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device->close(clockport_device);
    clockport_device = NULL;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int mmcreplay_common_attach(const char *filename)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    if (clockport_activate() < 0) {
        return -1;
    }

    mmcreplay_io1_list_item = io_source_register(&mmcreplay_io1_device);
    mmcreplay_io2_list_item = io_source_register(&mmcreplay_io2_device);
    mmcreplay_clockport_list_item = io_source_register(&mmcreplay_clockport_device);

    mmcr_enabled = 1;

    mmc_open_card_image(mmcr_card_filename, mmcr_card_rw);
    eeprom_open_image(mmcr_eeprom_filename, mmcr_eeprom_rw);

    mmcr_filename = lib_stralloc(filename);
    return 0;
}

int mmcreplay_bin_attach(const char *filename, BYTE *rawcart)
{
    int len = 0;
    FILE *fd;

    mmcr_filetype = 0;
    mmcr_filename = NULL;

    if (util_file_load(filename, rawcart, MMCREPLAY_FLASHROM_SIZE,
                       UTIL_FILE_LOAD_SKIP_ADDRESS | UTIL_FILE_LOAD_FILL) < 0) {
        return -1;
    }

    fd = fopen(filename, "rb");
    len = util_file_length(fd);
    fclose(fd);

    if (len == 0x10000) {
        if (util_file_load(filename, &rawcart[7 * 0x10000], 0x10000,
                           UTIL_FILE_LOAD_SKIP_ADDRESS | UTIL_FILE_LOAD_FILL) < 0) {
            return -1;
        }
    }

    mmcr_filetype = CARTRIDGE_FILETYPE_BIN;
    return mmcreplay_common_attach(filename);
}

int mmcreplay_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    mmcr_filetype = 0;
    mmcr_filename = NULL;

    memset(rawcart, 0xff, 0x80000);

    for (i = 0; i <= 63; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 63) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    if ((i != 8) && (i != 64)) {
        return -1;
    }

    if (i == 8) {
        memcpy(&rawcart[7 * 0x10000], &rawcart[0], 0x10000);
        memset(&rawcart[0], 0xff, 0x10000);
    }

    mmcr_filetype = CARTRIDGE_FILETYPE_CRT;
    return mmcreplay_common_attach(filename);
}

static int checkempty(int bank)
{
    int i;
    bank *= 0x10000;
    for (i = 0; i < 0x10000; i++) {
        if (roml_banks[i + bank] != 0xff) {
            return 0;
        }
    }
    return 1;
}

int mmcreplay_bin_save(const char *filename)
{
    FILE *fd;
    int i, n = 0;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (checkempty(i)) {
            n++;
        }
    }

    if ((n == 7) && (!checkempty(7))) {
        if (fwrite(&roml_banks[0x70000], 1, 0x10000, fd) != 0x10000) {
            fclose(fd);
            return -1;
        }
    } else {
        if (fwrite(&roml_banks[0x00000], 1, 0x10000 * 8, fd) != (0x10000 * 8)) {
            fclose(fd);
            return -1;
        }
    }
    fclose(fd);
    return 0;
}

int mmcreplay_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    BYTE *data;
    int i, n = 0;

    fd = crt_create(filename, CARTRIDGE_MMC_REPLAY, 1, 0, STRING_MMC_REPLAY);

    if (fd == NULL) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (checkempty(i)) {
            n++;
        }
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;

    if ((!checkempty(7)) && (n == 7)) {
        data = &roml_banks[0x70000];

        for (i = 0; i < 8; i++) {
            chip.bank = i + (7 * 8); /* bank */

            if (crt_write_chip(data, &chip, fd)) {
                fclose(fd);
                return -1;
            }
            data += 0x2000;
        }
    } else {
        data = &roml_banks[0x00000];

        for (i = 0; i < (8 * 8); i++) {
            chip.bank = i; /* bank */

            if (crt_write_chip(data, &chip, fd)) {
                fclose(fd);
                return -1;
            }
            data += 0x2000;
        }
    }
    fclose(fd);
    return 0;
}


int mmcreplay_flush_image(void)
{
    if (mmcr_filetype == CARTRIDGE_FILETYPE_BIN) {
        return mmcreplay_bin_save(mmcr_filename);
    } else if (mmcr_filetype == CARTRIDGE_FILETYPE_CRT) {
        return mmcreplay_crt_save(mmcr_filename);
    }
    return -1;
}

void mmcreplay_detach(void)
{
    if (mmcr_write_image && flashrom_state->flash_dirty) {
        mmcreplay_flush_image();
    }

    flash040core_shutdown(flashrom_state);
    lib_free(flashrom_state);
    lib_free(mmcr_ram);
    lib_free(mmcr_filename);
    mmcr_ram = NULL;
    mmcr_filename = NULL;
    mmc_close_card_image();
    eeprom_close_image(mmcr_eeprom_rw);
    export_remove(&export_res);
    clockport_deactivate();
    io_source_unregister(mmcreplay_io1_list_item);
    io_source_unregister(mmcreplay_io2_list_item);
    io_source_unregister(mmcreplay_clockport_list_item);
    mmcreplay_io1_list_item = NULL;
    mmcreplay_io2_list_item = NULL;
    mmcreplay_clockport_list_item = NULL;
    mmcr_enabled = 0;
}

int mmcreplay_cart_enabled(void)
{
    return mmcr_enabled;
}

/* ------------------------------------------------------------------------- */

static int set_mmcr_card_filename(const char *name, void *param)
{
    if (mmcr_card_filename != NULL && name != NULL && strcmp(name, mmcr_card_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    util_string_set(&mmcr_card_filename, name);

    if (mmcr_enabled) {
        return mmc_open_card_image(mmcr_card_filename, mmcr_card_rw);
    }

    return 0;
}

static int set_mmcr_eeprom_filename(const char *name, void *param)
{
    if (mmcr_eeprom_filename != NULL && name != NULL && strcmp(name, mmcr_eeprom_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    util_string_set(&mmcr_eeprom_filename, name);

    if (mmcr_enabled) {
        return eeprom_open_image(mmcr_eeprom_filename, mmcr_eeprom_rw);
    }

    return 0;
}

static int set_mmcr_card_rw(int val, void* param)
{
    mmcr_card_rw = val ? 1 : 0;

    if (mmcr_enabled) {
        return mmc_open_card_image(mmcr_card_filename, mmcr_card_rw);
    }

    return 0;
}

static int set_mmcr_eeprom_rw(int val, void* param)
{
    mmcr_eeprom_rw = val ? 1 : 0;
    return 0;
}

static int set_mmcr_rescue_mode(int val, void* param)
{
    enable_rescue_mode = val ? 1 : 0;
    return 0;
}

static int set_mmcr_sd_type(int val, void* param)
{
    switch (val) {
        case MMCR_TYPE_AUTO:
        case MMCR_TYPE_MMC:
        case MMCR_TYPE_SD:
        case MMCR_TYPE_SDHC:
            break;
        default:
            return -1;
    }
    mmcr_sd_type = val;
    mmc_set_card_type((BYTE)val);
    return 0;
}

static int set_mmcr_image_write(int val, void *param)
{
    mmcr_write_image = val ? 1 : 0;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "MMCRCardImage", "", RES_EVENT_NO, NULL,
      &mmcr_card_filename, set_mmcr_card_filename, NULL },
    { "MMCREEPROMImage", "", RES_EVENT_NO, NULL,
      &mmcr_eeprom_filename, set_mmcr_eeprom_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MMCRRescueMode", 0, RES_EVENT_NO, NULL,
      &enable_rescue_mode, set_mmcr_rescue_mode, NULL },
    { "MMCRImageWrite", 0, RES_EVENT_NO, NULL,
      &mmcr_write_image, set_mmcr_image_write, NULL },
    { "MMCRCardRW", 1, RES_EVENT_NO, NULL,
      &mmcr_card_rw, set_mmcr_card_rw, NULL },
    { "MMCRSDType", MMCR_TYPE_AUTO, RES_EVENT_NO, NULL,
      &mmcr_sd_type, set_mmcr_sd_type, NULL },
    { "MMCREEPROMRW", 1, RES_EVENT_NO, NULL,
      &mmcr_eeprom_rw, set_mmcr_eeprom_rw, NULL },
    { "MMCRClockPort", 0, RES_EVENT_NO, NULL,
      &clockport_device_id, set_mmcr_clockport_device, NULL },
    RESOURCE_INT_LIST_END
};

int mmcreplay_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void mmcreplay_resources_shutdown(void)
{
    lib_free(mmcr_card_filename);
    lib_free(mmcr_eeprom_filename);
    lib_free(clockport_device_names);
    clockport_device_names = NULL;
}

static const cmdline_option_t cmdline_options[] = {
    { "-mmcrrescue", SET_RESOURCE, 0,
      NULL, NULL, "MMCRRescueMode", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_RESCUE_MODE_ENABLE,
      NULL, NULL },
    { "+mmcrrescue", SET_RESOURCE, 0,
      NULL, NULL, "MMCRRescueMode", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_RESCUE_MODE_DISABLE,
      NULL, NULL },
    { "-mmcrimagerw", SET_RESOURCE, 0,
      NULL, NULL, "MMCRImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_MMC_REPLAY_IMAGE,
      NULL, NULL },
    { "+mmcrimagerw", SET_RESOURCE, 0,
      NULL, NULL, "MMCRImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_MMC_REPLAY_IMAGE,
      NULL, NULL },
    { "-mmcrcardimage", SET_RESOURCE, 1,
      NULL, NULL, "MMCRCardImage", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_FILE, IDCLS_SELECT_MMC_REPLAY_CARD_IMAGE_FILENAME,
      NULL, NULL },
    { "-mmcrcardrw", SET_RESOURCE, 0,
      NULL, NULL, "MMCRCardRW", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_CARD_WRITE_ENABLE,
      NULL, NULL },
    { "+mmcrcardrw", SET_RESOURCE, 0,
      NULL, NULL, "MMCRCardRW", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_CARD_WRITE_DISABLE,
      NULL, NULL },
    { "-mmcreepromimage", SET_RESOURCE, 1,
      NULL, NULL, "MMCREEPROMImage", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_P_FILE, IDCLS_SELECT_MMC_REPLAY_EEPROM_IMAGE,
      NULL, NULL },
    { "-mmcreepromrw", SET_RESOURCE, 0,
      NULL, NULL, "MMCREEPROMRW", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_EEPROM_WRITE_ENABLE,
      NULL, NULL },
    { "+mmcreepromrw", SET_RESOURCE, 0,
      NULL, NULL, "MMCREEPROMRW", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC_REPLAY_EEPROM_WRITE_DISABLE,
      NULL, NULL },
    { "-mmcrsdtype", SET_RESOURCE, 1,
      NULL, NULL, "MMCRSDType", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SELECT_MMC_REPLAY_SD_TYPE,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t clockport_cmdline_options[] =
{
    { "-mmcrclockportdevice", SET_RESOURCE, 1,
      NULL, NULL, "MMCRClockPort", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_DEVICE, IDCLS_CLOCKPORT_DEVICE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int mmcreplay_cmdline_options_init(void)
{
    int i;
    char *tmp;
    char number[10];

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    sprintf(number, "%d", clockport_supported_devices[0].id);

    clockport_device_names = util_concat(". (", number, ": ", clockport_supported_devices[0].name, NULL);

    for (i = 1; clockport_supported_devices[i].name; ++i) {
        tmp = clockport_device_names;
        sprintf(number, "%d", clockport_supported_devices[i].id);
        clockport_device_names = util_concat(tmp, ", ", number, ": ", clockport_supported_devices[i].name, NULL);
        lib_free(tmp);
    }
    tmp = clockport_device_names;
    clockport_device_names = util_concat(tmp, ")", NULL);
    lib_free(tmp);
    clockport_cmdline_options[0].description = clockport_device_names;

    return cmdline_register_options(clockport_cmdline_options);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTMMCR"

/* FIXME: implement snapshot support */
int mmcreplay_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int mmcreplay_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (mmcreplay_common_attach() < 0) {
        return -1;
    }
    return 0;
#endif
}
