/*
 * cbm2memsnapshot.c - CBM-II memory snapshot handling.
 *
 * Written by
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

#include "vice.h"

#include <stdio.h>

#include "cbm2-resources.h"
#include "cbm2.h"
#include "cbm2mem.h"
#include "cbm2memsnapshot.h"
#include "cbm2rom.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"

static log_t cbm2_snapshot_log = LOG_ERR;

/*
 * CBM2 memory dump should be 128, 256, 512 or 1024k, depending on the
 * config, as RAM.
 */
#define CBM2MEM_DUMP_VER_MAJOR   1
#define CBM2MEM_DUMP_VER_MINOR   0

/*
 * UBYTE        MEMSIZE         size in 128k (1=128, 2=256, 4=512, 8=1024)
 * UBYTE        CONFIG          Bit 0: cart08_ram
 *                                  1: cart1_ram
 *                                  2: cart2_ram
 *                                  3: cart4_ram
 *                                  4: cart6_ram
 *                                  5: cartC_ram
 *                                  6: 1= RAM starts at 0 (C500), videoram is
 *                                        1k VIC-II video, 1k colorram
 *                                     0= RAM starts at 0x10000 (others),
 *                                        videoram is 2k crtc videoram
 *
 * UBYTE        HCONFIG         Bit 0-1: ModelLine
 *
 * UBYTE        EXECBANK        CPU exec bank register
 * UBYTE        INDBANK         CPU indirect bank register
 * ARRAY        SYSRAM          2k system RAM, Bank15 $0000-$07ff
 * ARRAY        VIDEO           2k video RAM, Bank15 $d000-$d7ff
 * ARRAY        RAM             size according to MEMSIZE
 * ARRAY        RAM08           (only if memsize < 1M) 2k for cart08_ram
 * ARRAY        RAM1            (only if memsize < 1M) 4k for cart1_ram
 * ARRAY        RAM2            (only if memsize < 1M) 8k for cart2_ram
 * ARRAY        RAM4            (only if memsize < 1M) 8k for cart4_ram
 * ARRAY        RAM6            (only if memsize < 1M) 8k for cart6_ram
 * ARRAY        RAMC            (only if memsize < 1M) 4k for cartC_ram
 *
 */

static const char module_name[] = "CBM2MEM";

static int mem_write_ram_snapshot_module(snapshot_t *p)
{
    snapshot_module_t *m;
    BYTE config, memsize;
    int effective_ramsize, effective_start;

    m = snapshot_module_create(p, module_name,
                               CBM2MEM_DUMP_VER_MAJOR, CBM2MEM_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* calculate start and size of RAM to save */
    /* ramsize starts counting at 0x10000 if less than 512k */
    effective_ramsize = ramsize;
    effective_start = 0x10000;
    if ((machine_class == VICE_MACHINE_CBM5x0) && ramsize < 512) {
        effective_ramsize += 64;
    }
    if ((machine_class == VICE_MACHINE_CBM5x0) || ramsize >= 512) {
        effective_start = 0;
    }
    memsize = effective_ramsize >> 7;   /* rescale from 1k to 128k */

    config = (cart08_ram ? 1 : 0)
             | (cart1_ram ? 2 : 0)
             | (cart2_ram ? 4 : 0)
             | (cart4_ram ? 8 : 0)
             | (cart6_ram ? 16 : 0)
             | (cartC_ram ? 32 : 0)
             | ((machine_class == VICE_MACHINE_CBM5x0) ? 64 : 0);

    SMW_B(m, memsize);
    SMW_B(m, config);
    SMW_B(m, (BYTE)(cbm2_model_line & 3));

    SMW_B(m, (BYTE)(cbm2mem_bank_exec));
    SMW_B(m, (BYTE)(cbm2mem_bank_ind));

    SMW_BA(m, mem_ram + 0xf0000, 0x0800);
    SMW_BA(m, mem_rom + 0xd000, 0x0800);

    /* main memory array */
    SMW_BA(m, mem_ram + effective_start, ((int)memsize) << 17);

    if (memsize < 4) {  /* if 1M memory, bank 15 is included */
        if (config & 1) {
            SMW_BA(m, mem_ram + 0xf0800, 0x0800);
        }
        if (config & 2) {
            SMW_BA(m, mem_ram + 0xf1000, 0x1000);
        }
        if (config & 4) {
            SMW_BA(m, mem_ram + 0xf2000, 0x2000);
        }
        if (config & 8) {
            SMW_BA(m, mem_ram + 0xf4000, 0x2000);
        }
        if (config & 16) {
            SMW_BA(m, mem_ram + 0xf6000, 0x2000);
        }
        if (config & 32) {
            SMW_BA(m, mem_ram + 0xfc000, 0x1000);
        }
    }

    snapshot_module_close(m);

    return 0;
}

static int mem_read_ram_snapshot_module(snapshot_t *p)
{
    BYTE byte, vmajor, vminor;
    snapshot_module_t *m;
    BYTE config, hwconfig;
    int memsize;
    int effective_ramsize, effective_start;
    int bank0;

    m = snapshot_module_open(p, module_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != CBM2MEM_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_B(m, &byte);
    memsize = ((int)byte) & 0xff;

    SMR_B(m, &config);

    SMR_B(m, &hwconfig);
    resources_set_int("ModelLine", hwconfig & 3);

    SMR_B(m, &byte);
    cbm2mem_set_bank_exec(byte);
    SMR_B(m, &byte);
    cbm2mem_set_bank_ind(byte);

    SMR_BA(m, mem_ram + 0xf0000, 0x0800);
    SMR_BA(m, mem_rom + 0xd000, 0x0800);

    /* calculate start and size of RAM to load */
    /* ramsize starts counting at 0x10000 if less than 512k */
    bank0 = config & 64;
    effective_ramsize = memsize << 7;
    effective_start = 0x10000;
    if (bank0 || effective_ramsize >= 512) {
        effective_start = 0;
    }
    if (bank0 && effective_ramsize < 512) {
        effective_ramsize -= 64;
    }

    SMR_BA(m, mem_ram + effective_start, memsize << 17);

    ramsize = effective_ramsize;

    cart08_ram = config & 1;
    cart1_ram = config & 2;
    cart2_ram = config & 4;
    cart4_ram = config & 8;
    cart6_ram = config & 16;
    cartC_ram = config & 32;

    if (memsize < 4) {
        SMR_BA(m, mem_ram + 0x10000, memsize << 17);
    } else {
        SMR_BA(m, mem_ram, memsize << 17);
    }

    if (memsize < 4) {  /* if 1M memory, bank 15 is included */
        if (config & 1) {
            SMR_BA(m, mem_ram + 0xf0800, 0x0800);
        }
        if (config & 2) {
            SMR_BA(m, mem_ram + 0xf1000, 0x1000);
        }
        if (config & 4) {
            SMR_BA(m, mem_ram + 0xf2000, 0x2000);
        }
        if (config & 8) {
            SMR_BA(m, mem_ram + 0xf4000, 0x2000);
        }
        if (config & 16) {
            SMR_BA(m, mem_ram + 0xf6000, 0x2000);
        }
        if (config & 32) {
            SMR_BA(m, mem_ram + 0xfc000, 0x1000);
        }
    }

    mem_initialize_memory();

    snapshot_module_close(m);

    return 0;
}

/*********************************************************************/

/*
 * UBYTE        CONFIG          Bit 1: cart1 ROM included
 *                                  2: cart2 ROM included
 *                                  3: cart4 ROM included
 *                                  4: cart6 ROM included
 *                                  5: chargen is of C510 type (VIC-II)
 *
 * ARRAY        KERNAL          8k Kernal ROM ($e000-$ffff)
 * ARRAY        BASIC           16k Basic ROM ($8000-$bfff)
 * ARRAY        CHARGEN         4k chargen ROM image ($c*** for VIC-II)
 * ARRAY        ROM1            4k for cart1 (if config & 2)
 * ARRAY        ROM2            8k for cart2 (if config & 4)
 * ARRAY        ROM4            8k for cart4 (if config & 8)
 * ARRAY        ROM6            8k for cart6 (if config & 16)
 */

static const char module_rom_name[] = "CBM2ROM";
#define CBM2ROM_DUMP_VER_MAJOR   1
#define CBM2ROM_DUMP_VER_MINOR   0

static int mem_write_rom_snapshot_module(snapshot_t *p, int save_roms)
{
    snapshot_module_t *m;
    BYTE config;
    int trapfl;
    const char *cart_1_name, *cart_2_name, *cart_4_name, *cart_6_name;

    if (!save_roms) {
        return 0;
    }

    m = snapshot_module_create(p, module_rom_name,
                               CBM2ROM_DUMP_VER_MAJOR, CBM2ROM_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* disable traps before saving the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    resources_get_string("Cart1Name", &cart_1_name);
    resources_get_string("Cart2Name", &cart_2_name);
    resources_get_string("Cart4Name", &cart_4_name);
    resources_get_string("Cart6Name", &cart_6_name);

    config = ((cart_1_name ? 2 : 0)
              | (cart_2_name ? 4 : 0)
              | (cart_4_name ? 8 : 0)
              | (cart_6_name ? 16 : 0)
              | ((machine_class == VICE_MACHINE_CBM5x0) ? 32 : 0));

    /* SMW_B(m, save_roms & 3); */
    SMW_B(m, config);

    {
        /* kernal */
        SMW_BA(m, mem_rom + 0xe000, 0x2000);
        /* basic */
        SMW_BA(m, mem_rom + 0x8000, 0x4000);
        /* chargen */
        if (machine_class == VICE_MACHINE_CBM5x0) {
            SMW_BA(m, mem_chargen_rom, 0x1000);
        } else {
            SMW_BA(m, mem_chargen_rom, 0x0800);
            SMW_BA(m, mem_chargen_rom + 0x1000, 0x0800);
        }

        if (config & 2) {
            SMW_BA(m, mem_rom + 0x1000, 0x1000);
        }
        if (config & 4) {
            SMW_BA(m, mem_rom + 0x2000, 0x2000);
        }
        if (config & 8) {
            SMW_BA(m, mem_rom + 0x4000, 0x2000);
        }
        if (config & 16) {
            SMW_BA(m, mem_rom + 0x6000, 0x2000);
        }
    }

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    snapshot_module_close(m);

    return 0;
}

static int mem_read_rom_snapshot_module(snapshot_t *p)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    BYTE config;
    int i, trapfl;

    m = snapshot_module_open(p, module_rom_name, &vmajor, &vminor);
    if (m == NULL) {
        return 0;       /* optional */
    }
    if (vmajor != CBM2ROM_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    SMR_B(m, &config);

    /* kernal */
    SMR_BA(m, mem_rom + 0xe000, 0x2000);
    /* basic */
    SMR_BA(m, mem_rom + 0x8000, 0x4000);

    /* chargen */
    if (config & 32) {
        SMR_BA(m, mem_chargen_rom, 0x1000);
    } else {
        SMR_BA(m, mem_chargen_rom, 0x0800);
        SMR_BA(m, mem_chargen_rom + 0x1000, 0x0800);
        /* Inverted chargen into second half. This is a hardware feature.  */
        for (i = 0; i < 2048; i++) {
            mem_chargen_rom[i + 2048] = mem_chargen_rom[i] ^ 0xff;
            mem_chargen_rom[i + 6144] = mem_chargen_rom[i + 4096] ^ 0xff;
        }
    }

    if (config & 2) {
        SMR_BA(m, mem_rom + 0x1000, 0x1000);
    }
    if (config & 4) {
        SMR_BA(m, mem_rom + 0x2000, 0x2000);
    }
    if (config & 8) {
        SMR_BA(m, mem_rom + 0x4000, 0x2000);
    }
    if (config & 16) {
        SMR_BA(m, mem_rom + 0x6000, 0x2000);
    }

    log_warning(cbm2_snapshot_log,
                "Dumped Romset files and saved settings will "
                "represent\nthe state before loading the snapshot!");

    cbm2rom_checksum();

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    snapshot_module_close(m);

    return 0;
}

/*********************************************************************/

int cbm2_snapshot_write_module(snapshot_t *p, int save_roms)
{
    if (mem_write_ram_snapshot_module(p) < 0
        || mem_write_rom_snapshot_module(p, save_roms) < 0
        ) {
        return -1;
    }
    return 0;
}

int cbm2_snapshot_read_module(snapshot_t *p)
{
    if (mem_read_ram_snapshot_module(p) < 0
        || mem_read_rom_snapshot_module(p) < 0) {
        return -1;
    }

    return 0;
}
