/*
 * petmemsnapshot.c - PET memory snapshot handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

/*
 * FIXME: the rom_*_loaded flag stuff is not clear enough.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "autostart.h"
#include "kbdbuf.h"
#include "log.h"
#include "mem.h"
#include "pet.h"
#include "petmem.h"
#include "petmemsnapshot.h"
#include "petrom.h"
#include "pets.h"
#include "resources.h"
#include "snapshot.h"
#include "tape.h"
#include "types.h"
#include "machine.h"

static log_t pet_snapshot_log = LOG_ERR;

/*
 * PET memory dump should be 4-32k or 128k, depending on the config, as RAM.
 * Plus 64k expansion RAM (8096 or SuperPET) if necessary. Also there
 * is the 1/2k video RAM as "VRAM".
 * In this prototype we save the full ram......
 */

static const char module_ram_name[] = "PETMEM";
#define PETMEM_DUMP_VER_MAJOR   1
#define PETMEM_DUMP_VER_MINOR   3

/*
 * UBYTE        CONFIG          Bits 0-3: 0 = 40 col PET without CRTC
 *                                        1 = 40 col PET with CRTC
 *                                        2 = 80 col PET (with CRTC)
 *                                        3 = SuperPET
 *                                        4 = 8096
 *                                        5 = 8296
 *                              Bit 6: 1= RAM at $9***
 *                              Bit 7: 1= RAM at $A***
 *
 * UBYTE        KEYBOARD        0 = UK business
 *                              1 = graphics
 *
 * UBYTE        MEMSIZE         memory size of low 32k in k (4,8,16,32)
 *
 * UBYTE        CONF8X96        8x96 configuration register
 * UBYTE        SUPERPET        SuperPET config:
 *                              Bit 0: spet_ramen,  1= RAM enabled
 *                                  1: spet_ramwp,  1= RAM write protected
 *                                  2: spet_ctrlwp, 1= CTRL reg write prot.
 *                                  3: spet_diag,   0= diag active
 *                                  4-7: spet_bank, RAM block in use
 *
 * ARRAY        RAM             4-32k RAM (not 8296, dep. on MEMSIZE)
 * ARRAY        VRAM            2/4k RAM (not 8296, dep in CONFIG)
 * ARRAY        EXTRAM          64k (SuperPET and 8096 only)
 * ARRAY        RAM             128k RAM (8296 only)
 *
 *                              Added in format V1.1, should be part of
 *                              KEYBOARD in later versions.
 *
 * BYTE         POSITIONAL      bit 0=0 = symbolic keyboard mapping
 *                                   =1 = positional keyboard mapping
 *
 *                              Added in format V1.2
 * BYTE         EOIBLANK        bit 0=0: EOI does not blank screen
 *                                   =1: EOI does blank screen
 *
 *                              Added in format V1.3
 * WORD         CPU_SWITCH      6502 / 6809 / PROG
 * BYTE         VAL             6702 state information
 * BYTE         PREVODD
 * BYTE         WANTODD
 * WORD[8]      SHIFT
 */

static int mem_write_ram_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;
    BYTE config, rconf, memsize, conf8x96, superpet, superpet2;
    int kbdindex;
    int i;

    memsize = petres.ramSize;
    if (memsize > 32) {
        memsize = 32;
    }

    if (!petres.crtc) {
        config = 0;
    } else {
        config = petres.videoSize == 0x400 ? 1 : 2;
    }

    if (petres.map) {
        config = petres.map + 3;
    } else {
        if (petres.superpet) {
            config = 3;
        }
    }

    rconf = (petres.ramsel9 ? 0x40 : 0) | (petres.ramselA ? 0x80 : 0);

    conf8x96 = petmem_map_reg;

    superpet = (spet_ramen ? 1 : 0)
               | (spet_ramwp ? 2 : 0)
               | (spet_ctrlwp ? 4 : 0)
               | (spet_diag ? 8 : 0)
               | ((spet_bank << 4) & 0xf0);

    m = snapshot_module_create(s, module_ram_name,
                               PETMEM_DUMP_VER_MAJOR, PETMEM_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }
    SMW_B(m, (BYTE)(config | rconf));

    resources_get_int("KeymapIndex", &kbdindex);
    SMW_B(m, (BYTE)(kbdindex >> 1));

    SMW_B(m, memsize);
    SMW_B(m, conf8x96);
    SMW_B(m, superpet);

    if (config != 5) {
        SMW_BA(m, mem_ram, memsize << 10);

        SMW_BA(m, mem_ram + 0x8000, (config < 2) ? 0x400 : 0x800);

        if (config == 3 || config == 4) {
            SMW_BA(m, mem_ram + 0x10000, 0x10000);
        }
    } else {    /* 8296 */
        SMW_BA(m, mem_ram, 0x20000);
    }

    /* V1.1 */
    SMW_B(m, (BYTE)(kbdindex & 1));
    /* V1.2 */
    SMW_B(m, (BYTE)(petres.eoiblank ? 1 : 0));
    /* V1.3 */
    SMW_W(m, (WORD)petres.superpet_cpu_switch);
    SMW_B(m, (BYTE)dongle6702.val);
    SMW_B(m, (BYTE)dongle6702.prevodd);
    SMW_B(m, (BYTE)dongle6702.wantodd);
    for (i = 0; i < 8; i++) {
        SMW_W(m, (WORD)dongle6702.shift[i]);
    }
    /* Extra SuperPET2 byte; more state of $EFFC */
    superpet2 = spet_bank & 0x10;
    if (spet_firq_disabled) {
        superpet2 |= 0x20;
    }
    if (spet_flat_mode) {
        superpet2 |= 0x40;
    }
    SMW_B(m, superpet2);

    snapshot_module_close(m);

    return 0;
}

static int mem_read_ram_snapshot_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    BYTE config, rconf, byte, memsize, conf8x96, superpet;
    petinfo_t peti = {
        32, 0x0800, 1, 80, 0, 0, 0, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL
    };
    int old6809mode;
    int spet_bank = 0;

    m = snapshot_module_open(s, module_ram_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != PETMEM_DUMP_VER_MAJOR) {
        log_error(pet_snapshot_log,
                  "Cannot load PET RAM module with major version %d",
                  vmajor);
        snapshot_module_close(m);
        return -1;
    }

    old6809mode = petres.superpet &&
                  petres.superpet_cpu_switch == SUPERPET_CPU_6809;

    SMR_B(m, &config);

    SMR_B(m, &byte);
    peti.kbd_type = byte;

    SMR_B(m, &memsize);
    SMR_B(m, &conf8x96);
    SMR_B(m, &superpet);

    rconf = config & 0xc0;
    config &= 0x0f;

    peti.ramSize = memsize;
    peti.crtc = 1;
    peti.IOSize = 0x800;
    peti.video = 80;
    peti.superpet = 0;

    switch (config) {
        case 0:         /* 40 cols w/o CRTC */
            peti.crtc = 0;
            peti.video = 40;
            break;
        case 1:         /* 40 cols w/ CRTC */
            peti.video = 40;
            break;
        case 2:         /* 80 cols (w/ CRTC) */
            break;
        case 3:         /* SuperPET */
            spet_ramen = superpet & 1;
            spet_ramwp = superpet & 2;
            spet_ctrlwp = superpet & 4;
            spet_diag = superpet & 8;
            spet_bank = (superpet >> 4) & 0x0f;
            peti.superpet = 1;
            break;
        case 4:         /* 8096 */
            peti.ramSize = 96;
            break;
        case 5:         /* 8296 */
            peti.ramSize = 128;
            break;
    }

    peti.ramsel9 = (rconf & 0x40) ? 1 : 0;
    peti.ramselA = (rconf & 0x80) ? 1 : 0;

    petmem_set_conf_info(&peti);  /* set resources and config accordingly */
    petmem_map_reg = conf8x96;

    mem_initialize_memory();

    pet_crtc_set_screen();

    if (config != 5) {
        SMR_BA(m, mem_ram, memsize << 10);

        SMR_BA(m, mem_ram + 0x8000, (config < 2) ? 0x400 : 0x800);

        if (config == 3 || config == 4) {
            SMR_BA(m, mem_ram + 0x10000, 0x10000);
        }
    } else {    /* 8296 */
        SMR_BA(m, mem_ram, 0x20000);
    }

    if (vminor > 0) {
        int kindex;
        SMR_B(m, &byte);
        resources_get_int("KeymapIndex", &kindex);
        resources_set_int("KeymapIndex", (kindex & ~1) | (byte & 1));
    }
    if (vminor > 1) {
        SMR_B(m, &byte);
        resources_set_int("EoiBlank", byte & 1);
    }
    if (vminor > 2) {
        int new6809mode, i;
        BYTE b;
        WORD w;

        SMR_W(m, &w); petres.superpet_cpu_switch = w;
        SMR_B(m, &b); dongle6702.val = b;
        SMR_B(m, &b); dongle6702.prevodd = b;
        SMR_B(m, &b); dongle6702.wantodd = b;

        for (i = 0; i < 8; i++) {
            SMR_W(m, &w);
            dongle6702.shift[i] = w;
        }

        /* Extra superpet2 bits */
        b = 0;  /* when not present in file */
        SMR_B(m, &b);
        spet_bank |= (b & 0x10);
        spet_firq_disabled = (b & 0x20);
        spet_flat_mode = (b & 0x40);

        /*
         * TODO: make the CPU switch if needed, WITHOUT a reset!
         * (A real-world CPU switch toggle always implies a reset)
         * If the user loads a dump file running in the other mode,
         * she may need to reset (to get to the correct CPU),
         * then reload the dump again.
         */
        new6809mode = petres.superpet &&
                      petres.superpet_cpu_switch == SUPERPET_CPU_6809;
        if (new6809mode != old6809mode) {
            log_error(pet_snapshot_log,
                      "Snapshot for different CPU. Re-load the snapshot.");
            machine_trigger_reset(MACHINE_RESET_MODE_HARD);
            return -1;
        }
        /* set banked or flat memory mapping */
        mem_initialize_memory_6809();
    }

    /* spet_bank_4k = spet_bank << 12; */
    set_spet_bank(spet_bank);

    snapshot_module_close(m);

    return 0;
}

static const char module_rom_name[] = "PETROM";
#define PETROM_DUMP_VER_MAJOR   1
#define PETROM_DUMP_VER_MINOR   1

/*
 * UBYTE        CONFIG          Bit 0: 1= $9*** ROM included
 *                                  1: 1= $a*** ROM included
 *                                  2: 1= $b*** ROM included
 *                                  3: 1= $e900-$efff ROM included
 *                                  4: 1= $9000-$ffff 6809 ROM
 *                                        and upper half CHARGEN ROM included
 *
 * ARRAY        KERNAL          4k KERNAL ROM image $f000-$ffff
 * ARRAY        EDITOR          2k EDITOR ROM image $e000-$e800
 * ARRAY        CHARGEN         2k CHARGEN ROM image
 * ARRAY        ROM9            4k $9*** ROM (if CONFIG & 1)
 * ARRAY        ROMA            4k $A*** ROM (if CONFIG & 2)
 * ARRAY        ROMB            4k $B*** ROM (if CONFIG & 4)
 * ARRAY        ROMC            4k $C*** ROM
 * ARRAY        ROMD            4k $D*** ROM
 * ARRAY        ROME9           7 blocks $e900-$efff ROM (if CONFIG & 8)
 *                              Added in format V1.1:
 * ARRAY        ROM6809         24k $A000-$FFFF ROM   (if CONFIG & 16)
 * ARRAY        CHARGEN(2)      upper half of CHARGEN (if CONFIG & 16)
 *
 */

static int mem_write_rom_snapshot_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;
    BYTE config;
    int i, trapfl;

    if (!save_roms) {
        return 0;
    }

    m = snapshot_module_create(s, module_rom_name,
                               PETROM_DUMP_VER_MAJOR, PETROM_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* disable traps before saving the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);
    petrom_unpatch_2001();

    config = (petrom_9_loaded ? 1 : 0)
             | (petrom_A_loaded ? 2 : 0)
             | (petrom_B_loaded ? 4 : 0)
             | ((petres.ramSize == 128) ? 8 : 0)
             | (petres.superpet ? 16 : 0);

    SMW_B(m, config);

    {
        SMW_BA(m, mem_rom + 0x7000, 0x1000);
        SMW_BA(m, mem_rom + 0x6000, 0x0800);

        /* pick relevant data from chargen ROM */
        for (i = 0; i < 128; i++) {
            SMW_BA(m, mem_chargen_rom + i * 16, 8);
        }
        for (i = 0; i < 128; i++) {
            SMW_BA(m, mem_chargen_rom + 0x1000 + i * 16, 8);
        }

        if (config & 1) {
            SMW_BA(m, mem_rom + 0x1000, 0x1000);
        }
        if (config & 2) {
            SMW_BA(m, mem_rom + 0x2000, 0x1000);
        }
        if (config & 4) {
            SMW_BA(m, mem_rom + 0x3000, 0x1000);
        }

        SMW_BA(m, mem_rom + 0x4000, 0x2000);

        if (config & 8) {
            SMW_BA(m, mem_rom + 0x6900, 0x0700);
        }

        if (config & 16) {
            SMW_BA(m, mem_6809rom, PET_6809_ROMSIZE);

            /* pick relevant data from upper half of chargen ROM */
            for (i = 0; i < 128; i++) {
                SMW_BA(m, mem_chargen_rom + 0x2000 + i * 16, 8);
            }
            for (i = 0; i < 128; i++) {
                SMW_BA(m, mem_chargen_rom + 0x3000 + i * 16, 8);
            }
        }
    }

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);
    petrom_patch_2001();

    snapshot_module_close(m);

    return 0;
}

static int mem_read_rom_snapshot_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    BYTE config;
    int trapfl, new_iosize;

    m = snapshot_module_open(s, module_rom_name, &vmajor, &vminor);
    if (m == NULL) {
        return 0;       /* optional */
    }
    if (vmajor != PETROM_DUMP_VER_MAJOR) {
        log_error(pet_snapshot_log,
                  "Cannot load PET ROM module with major version %d",
                  vmajor);
        snapshot_module_close(m);
        return -1;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);
    petrom_unpatch_2001();

    config = (petrom_9_loaded ? 1 : 0)
             | (petrom_A_loaded ? 2 : 0)
             | (petrom_B_loaded ? 4 : 0)
             | ((petres.pet2k || petres.ramSize == 128) ? 8 : 0);

    SMR_B(m, &config);

    /* De-initialize kbd-buf, autostart and tape stuff here before
       loading the new ROMs. These depend on addresses defined in the
       rom - they might be different in the loaded ROM. */
    kbdbuf_init(0, 0, 0, 0);
    autostart_init(0, 0, 0, 0, 0, 0);
    tape_deinstall();

    petrom_9_loaded = config & 1;
    petrom_A_loaded = config & 2;
    petrom_B_loaded = config & 4;

    if (config & 8) {
        new_iosize = 0x100;
    } else {
        new_iosize = 0x800;
    }
    if (new_iosize != petres.IOSize) {
        petres.IOSize = new_iosize;
        mem_initialize_memory();
    }

    {
        /* kernal $f000-$ffff */
        SMR_BA(m, mem_rom + 0x7000, 0x1000);
        /* editor $e000-$e7ff */
        SMR_BA(m, mem_rom + 0x6000, 0x0800);

        /* chargen ROM */
        resources_set_int("Basic1Chars", 0);
        SMR_BA(m, mem_chargen_rom, 0x0800);

        /* $9000-$9fff */
        if (config & 1) {
            SMR_BA(m, mem_rom + 0x1000, 0x1000);
        }
        /* $a000-$afff */
        if (config & 2) {
            SMR_BA(m, mem_rom + 0x2000, 0x1000);
        }
        /* $b000-$bfff */
        if (config & 4) {
            SMR_BA(m, mem_rom + 0x3000, 0x1000);
        }

        /* $c000-$dfff */
        SMR_BA(m, mem_rom + 0x4000, 0x2000);

        /* $e900-$efff editor extension */
        if (config & 8) {
            SMR_BA(m, mem_rom + 0x6900, 0x0700);
        }

        /* 6809 ROMs */
        if (config & 16) {
            SMR_BA(m, mem_6809rom, PET_6809_ROMSIZE);
            SMR_BA(m, mem_chargen_rom + 0x0800, 0x0800);
        }

        petrom_convert_chargen(mem_chargen_rom);
    }

    log_warning(pet_snapshot_log, "Dumped Romset files and saved settings will "
                "represent\nthe state before loading the snapshot!");

    petres.rompatch = 0;

    petrom_get_kernal_checksum();
    petrom_get_editor_checksum();
    petrom_checksum();

    petrom_patch_2001();

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    snapshot_module_close(m);

    return 0;
}

int pet_snapshot_write_module(snapshot_t *s, int save_roms)
{
    if (mem_write_ram_snapshot_module(s) < 0
        || mem_write_rom_snapshot_module(s, save_roms) < 0) {
        return -1;
    }
    return 0;
}

int pet_snapshot_read_module(snapshot_t *s)
{
    if (mem_read_ram_snapshot_module(s) < 0
        || mem_read_rom_snapshot_module(s) < 0) {
        return -1;
    }
    return 0;
}
