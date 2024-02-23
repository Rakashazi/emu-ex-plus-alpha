/*
 * scpu64mem.c -- SCPU64 memory handling.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scpu64.h"
#include "scpu64-resources.h"
#include "c64cart.h"
#include "c64cia.h"
#include "c64pla.h"
#include "scpu64mem.h"
#include "scpu64rom.h"
#include "scpu64meminit.h"
#include "c64cartmem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "machine.h"
#include "main65816cpu.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "reu.h"
#include "sid.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"
#include "vicii.h"
#include "scpu64cpu.h"
#include "lib.h"
#include "wdc65816.h"
#include "vicii-cycle.h"
#include "traps.h"

/* Machine class */
int machine_class = VICE_MACHINE_SCPU64;

static int scpu64_version_v2 = 1; /* for future v1 emulation */

/* Dummy processor port.  */
pport_t pport;

/* C64 memory-related resources.  */

/* ------------------------------------------------------------------------- */

/* Number of possible memory configurations.  */
#define NUM_CONFIGS     256

/* Number of possible mirroring configurations.  */
#define NUM_MIRRORS     16

static const uint16_t mem_mirrors[NUM_MIRRORS] = {
    0x80bf, 0x80bf, 0x003f, 0x023f,
    0x407f, 0x407f, 0xc0ff, 0xc0ff,
    0x0407, 0x0407,        0,    0,
    0x00ff, 0x02ff, 0x00ff, 0x02ff
};

/* The C64 memory.  */
uint8_t mem_ram[SCPU64_RAM_SIZE];
uint8_t mem_sram[SCPU64_SRAM_SIZE];
uint8_t mem_trap_ram[SCPU64_KERNAL_ROM_SIZE];
uint8_t *mem_simm_ram = NULL;
static int mem_simm_page_size;
static int mem_conf_page_size;
static int mem_conf_size;
unsigned int mem_simm_ram_mask = 0;
uint8_t mem_tooslow[1];
static int traps_pending;

uint8_t mem_chargen_rom[SCPU64_CHARGEN_ROM_SIZE];

/* Internal color memory.  */
static uint8_t mem_color_ram[0x400];
uint8_t *mem_color_ram_cpu, *mem_color_ram_vicii;

/* Pointer to the chargen ROM.  */
uint8_t *mem_chargen_rom_ptr;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
static uint8_t **_mem_read_base_tab_ptr;
static uint32_t *mem_read_limit_tab_ptr;

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_MIRRORS][NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static uint8_t *mem_read_base_tab[NUM_CONFIGS][0x101];
static uint32_t mem_read_limit_tab[NUM_CONFIGS][0x101];

static store_func_ptr_t mem_write_tab_watch[0x101];
static read_func_ptr_t mem_read_tab_watch[0x101];

/* Current mirror config */
static int mirror;

/* Current memory configuration.  */
static int mem_config;

/* Current watchpoint state. 1 = watchpoints active, 0 = no watchpoints */
static int watchpoints_active;


static int mem_reg_sw_1mhz;     /* 1MHz physical switch */
static int mem_reg_sw_jiffy = 1;/* Jiffy physical switch */
int mem_reg_soft_1mhz;          /* 1MHz software enabled */
int mem_reg_sys_1mhz;           /* 1MHz system enabled */
int mem_reg_hwenable;           /* hardware enabled */
int mem_reg_dosext;             /* dos extension enable */
int mem_reg_ramlink;            /* ramlink registers enable */
int mem_reg_optim;              /* optimization mode */
int mem_reg_bootmap;            /* boot map */
int mem_reg_simm;               /* simm configuration */
int mem_pport;                  /* processor "port" */

static int dma_in_progress = 0;

/* ------------------------------------------------------------------------- */

inline static void check_ba_read(void)
{
    if (!scpu64_fastmode && maincpu_ba_low_flags && !dma_in_progress) {
        maincpu_steal_cycles();
    }
}

inline static void check_ba_write(void)
{
    if (!scpu64_fastmode && !scpu64_emulation_mode && maincpu_ba_low_flags && !dma_in_progress) {
        maincpu_steal_cycles();
    }
}
/* ------------------------------------------------------------------------- */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mirror][mem_config][0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mirror][mem_config][addr >> 8](addr, value);
}

void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
    } else {
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[mirror][mem_config];
    }
    watchpoints_active = flag;
}

/* ------------------------------------------------------------------------- */

void scpu64_mem_init(void)
{
    /* Initialize REU BA low interface (FIXME find a better place for this) */
    reu_ba_register(vicii_cycle, vicii_steal_cycles, &maincpu_ba_low_flags, MAINCPU_BA_LOW_REU);
}

void mem_pla_config_changed(void)
{
    mem_config = ((mem_pport & 7) | (export.exrom << 3) | (export.game << 4)
                | (mem_reg_hwenable << 5) | (mem_reg_dosext << 6) | (mem_reg_bootmap << 7));

    if (watchpoints_active) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
    } else {
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[mirror][mem_config];
    }

    _mem_read_base_tab_ptr = mem_read_base_tab[mem_config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[mem_config];

    maincpu_resync_limits();
}

static void pport_store(uint16_t addr, uint8_t value)
{
    if (mem_pport != value) {
        mem_pport = value;
        mem_pla_config_changed();
    }
}

/* reads zeropage, 0/1 comes from RAM */
uint8_t zero_read_dma(uint16_t addr)
{
    addr &= 0xff;

    return mem_ram[addr & 0xff];
}

/* reads zeropage, 0/1 comes from CPU port */
uint8_t zero_read(uint16_t addr)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            return pport.dir_read;
        case 1:
            return pport.data_read;
    }

    return zero_read_dma(addr);
}

/* store zeropage, 0/1 goes to RAM */
void zero_store_dma(uint16_t addr, uint8_t value)
{
    mem_sram[addr] = value;
}

/* store zeropage, 0/1 goes to CPU port */
void zero_store(uint16_t addr, uint8_t value)
{
    zero_store_dma(addr, value);

    if (addr == 1) {
        pport_store(addr, (uint8_t)(value & 7));
    }
}

static void zero_store_mirrored(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    mem_sram[addr] = value;
    if (addr == 1) {
        pport_store(addr, (uint8_t)(value & 7));
    }
    mem_ram[addr] = value;
}

static void zero_store_int(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    if (addr == 1) {
        pport_store(addr, (uint8_t)(value & 7));
    }
    mem_ram[addr] = value;
}

/* ------------------------------------------------------------------------- */

uint8_t chargen_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return mem_chargen_rom[addr & 0xfff];
}

uint8_t ram_read(uint16_t addr)
{
    check_ba_read();
    return mem_sram[addr];
}

void ram_store(uint16_t addr, uint8_t value)
{
    check_ba_write();
    mem_sram[addr] = value;
}

uint8_t ram_read_int(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return mem_ram[addr];
}

void ram_store_int(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    mem_ram[addr] = value;
}

static void ram_store_mirrored(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    mem_sram[addr] = value;
    mem_ram[addr] = value;
}
/* ------------------------------------ */
static void ram_hi_store_mirrored(uint16_t addr, uint8_t value) /* mirrored, no vbank */
{
    if (addr == 0xff00) {
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io_start();
        }
        mem_sram[addr] = value;
        mem_ram[addr] = value;
        if (!dma_in_progress) {
            reu_dma(-1);
            scpu64_clock_write_stretch_io_long();
        }
    } else {
        if (!dma_in_progress) {
            scpu64_clock_write_stretch();
        }
        mem_sram[addr] = value;
        mem_ram[addr] = value;
    }
}

static void ram_hi_store(uint16_t addr, uint8_t value) /* not mirrored */
{
    if (addr == 0xff00) {
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io_start();
        }
        mem_sram[addr] = value;
        if (!dma_in_progress) {
            reu_dma(-1);
            scpu64_clock_write_stretch_io_long();
        }
    } else {
        check_ba_write();
        mem_sram[addr] = value;
    }
}

static void ram_hi_store_int(uint16_t addr, uint8_t value) /* internal */
{
    if (addr == 0xff00) {
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io_start();
        }
        mem_ram[addr] = value;
        if (!dma_in_progress) {
            reu_dma(-1);
            scpu64_clock_write_stretch_io_long();
        }
    } else {
        if (!dma_in_progress) {
            scpu64_clock_write_stretch();
        }
        mem_ram[addr] = value;
    }
}

/* ------------------------------------ */

uint8_t scpu64_kernalshadow_read(uint16_t addr)
{
    check_ba_read();
    return mem_sram[0x8000 + addr];
}

uint8_t ram1_read(uint16_t addr)
{
    check_ba_read();
    return mem_sram[0x10000 + addr];
}

uint8_t scpu64rom_scpu64_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_eprom();
    }
    return scpu64rom_scpu64_rom[addr];
}

/* ------------------------------------------------------------------------- */

/* DMA memory access, this is the same as generic memory access, but needs to
   bypass the CPU port, so it accesses RAM at $00/$01 */

void mem_dma_store(uint16_t addr, uint8_t value)
{
    dma_in_progress = 1;
    if ((addr & 0xff00) == 0) {
        /* exception: 0/1 accesses RAM! */
        zero_store_dma(addr, value);
    } else {
        _mem_write_tab_ptr[addr >> 8](addr, value);
    }
    dma_in_progress = 0;
}

uint8_t mem_dma_read(uint16_t addr)
{
    uint8_t retval = 0;

    dma_in_progress = 1;
    if ((addr & 0xff00) == 0) {
        /* exception: 0/1 accesses RAM! */
        retval = zero_read_dma(addr);
    } else {
        retval = _mem_read_tab_ptr[addr >> 8](addr);
    }
    dma_in_progress = 0;

    return retval;
}


/* ------------------------------------------------------------------------- */

/* Generic memory access.  */

void mem_store(uint16_t addr, uint8_t value)
{
    _mem_write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem_read(uint16_t addr)
{
    return _mem_read_tab_ptr[addr >> 8](addr);
}

void mem_store2(uint32_t addr, uint8_t value)
{
    switch (addr & 0xfe0000) {
    case 0xf60000:
        if (mem_simm_ram_mask) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
                addr &= mem_simm_ram_mask;
            }
            if (mem_reg_hwenable) {
                mem_simm_ram[addr & 0x1ffff] = value;
            }
            if (!dma_in_progress) {
                scpu64_clock_write_stretch_simm(addr);
            }
        }
        return;
    case 0xf80000:
    case 0xfa0000:
    case 0xfc0000:
    case 0xfe0000:
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_eprom();
        }
        return;
    case 0x000000:
        if (addr & 0xfffe) {
            if (addr >= 0x1e000 && mem_trap_ram[addr & 0x1fff] != value) {
                traps_pending = 1;
                mem_trap_ram[addr & 0x1fff] = value;
            }
            mem_sram[addr] = value;
        } else if (scpu64_version_v2) {
            mem_sram[addr & 1] = value;
        } else {
            mem_sram[addr] = value;
        }
        return;
    default:
        if (mem_simm_ram_mask && addr < (unsigned int)mem_conf_size) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
            }
            mem_simm_ram[addr & mem_simm_ram_mask] = value;
            if (!dma_in_progress) {
                scpu64_clock_write_stretch_simm(addr);
            }
        }
    }
}

uint8_t mem_read2(uint32_t addr)
{
    switch (addr & 0xfe0000) {
    case 0xf60000:
        if (mem_simm_ram_mask) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
                addr &= mem_simm_ram_mask;
            }
            if (!dma_in_progress) {
                scpu64_clock_read_stretch_simm(addr);
            }
            return mem_simm_ram[addr & 0x1ffff];
        }
        break;
    case 0xf80000:
    case 0xfa0000:
    case 0xfc0000:
    case 0xfe0000:
        if (!dma_in_progress) {
            scpu64_clock_read_stretch_eprom();
        }
        return scpu64rom_scpu64_rom[addr & (SCPU64_SCPU64_ROM_MAXSIZE-1) & 0x7ffff];
    case 0x000000:
        if (addr & 0xfffe) {
            return mem_sram[addr];
        }
        return scpu64_version_v2 ? mem_sram[addr & 1] : mem_sram[addr];
    default:
        if (mem_simm_ram_mask && addr < (unsigned int)mem_conf_size) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
            }
            if (!dma_in_progress) {
                scpu64_clock_read_stretch_simm(addr);
            }
            return mem_simm_ram[addr & mem_simm_ram_mask];
        }
        break;
    }
    return (uint8_t)(addr >> 16);
}

static uint8_t mem_peek2(uint32_t addr)
{
    switch (addr & 0xfe0000) {
    case 0xf60000:
        if (mem_simm_ram_mask) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
                addr &= mem_simm_ram_mask;
            }
            return mem_simm_ram[addr & 0x1ffff];
        }
        break;
    case 0xf80000:
    case 0xfa0000:
    case 0xfc0000:
    case 0xfe0000:
        return scpu64rom_scpu64_rom[addr & (SCPU64_SCPU64_ROM_MAXSIZE-1) & 0x7ffff];
    case 0x000000:
        if (addr & 0xfffe) {
            return mem_sram[addr];
        }
        return scpu64_version_v2 ? mem_sram[addr & 1] : mem_sram[addr];
    default:
        if (mem_simm_ram_mask && addr < (unsigned int)mem_conf_size) {
            if (mem_simm_page_size != mem_conf_page_size) {
                addr = ((addr >> mem_conf_page_size) << mem_simm_page_size) | (addr & ((1 << mem_simm_page_size)-1));
            }
            return mem_simm_ram[addr & mem_simm_ram_mask];
        }
        break;
    }
    return (uint8_t)(addr >> 16);
}

void mem_store_without_ultimax(uint16_t addr, uint8_t value)
{
    store_func_ptr_t *write_tab_ptr;

    write_tab_ptr = mem_write_tab[mirror][mem_config & 7];

    write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem_read_without_ultimax(uint16_t addr)
{
    read_func_ptr_t *read_tab_ptr;

    read_tab_ptr = mem_read_tab[mem_config & 7];

    return read_tab_ptr[addr >> 8](addr);
}

void mem_store_without_romlh(uint16_t addr, uint8_t value)
{
    store_func_ptr_t *write_tab_ptr;

    write_tab_ptr = mem_write_tab[mirror][0];

    write_tab_ptr[addr >> 8](addr, value);
}

/* ------------------------------------------------------------------------- */
static uint8_t scpu64_hardware_read(uint16_t addr)
{
    uint8_t value = 0x00;

    switch (addr) {
    case 0xd0b0:
        value = scpu64_version_v2 ? 0x40 : 0xc0;
        break;
    case 0xd0b1:
        break;
    case 0xd0b2:       /* bit 7 - hwreg enabled (1)/disabled (0) */
                       /* bit 6 - system 1 MHz enabled (1)/disabled (0) */
        value = (mem_reg_hwenable ? 0x80 : 0x00) | (mem_reg_sys_1mhz ? 0x40 : 0x00);
        break;
    case 0xd0b3:
        if (scpu64_version_v2) {
            value = mem_reg_optim & 0xc0;
        }
        break;
    case 0xd0b4:
        value = mem_reg_optim & 0xc0;
        break;
    case 0xd0b5:      /* bit 7 - Jiffy (1)/No jiffy (0) switch */
                      /* bit 6 - 1 MHz (1)/20 MHz (0) switch */
        value = (mem_reg_sw_jiffy ? 0x80 : 0x00) | (mem_reg_sw_1mhz ? 0x40 : 0x00);
        break;
    case 0xd0b6:      /* bit 7 - Emulation mode (1)/Native (0) */
        value = scpu64_emulation_mode ? 0x80 : 0x00;
        break;
    case 0xd0b7:
        break;
    case 0xd0b9:      /* same as 0xd0b8 */
    case 0xd0b8:      /* bit 7 - software 1 MHz enabled (1)/disabled (0) */
                      /* bit 6 - 1 MHz (1)/20 MHz (2) switch+software+system */
        value = (mem_reg_soft_1mhz ? 0x80 : 0x00) | ((mem_reg_soft_1mhz
                 || (mem_reg_sw_1mhz && !mem_reg_hwenable) || mem_reg_sys_1mhz) ? 0x40 : 0x00);
        break;
    case 0xd0ba:
        break;
    case 0xd0bb:
        break;
    case 0xd0bc:
    case 0xd0bd:
    case 0xd0be:
    case 0xd0bf:
        value = (mem_reg_dosext ? 0x80 : 0x00) | (mem_reg_ramlink ? 0x40 : 0x00);
        break;
    default:
        value = 0xff;
        break;
    }
    return value | (mem_reg_optim & 7);
}

static void scpu64_hardware_store(uint16_t addr, uint8_t value)
{
    switch (addr) {
    case 0xd071:
        break;
    case 0xd072: /* System 1MHz enable */
        if (!mem_reg_sys_1mhz) {
            mem_reg_sys_1mhz = 1;
            scpu64_set_fastmode(0);
        }
        break;
    case 0xd073: /* System 1MHz disable */
        if (mem_reg_sys_1mhz) {
            mem_reg_sys_1mhz = 0;
            scpu64_set_fastmode(!(mem_reg_soft_1mhz || (mem_reg_sw_1mhz && !mem_reg_hwenable)));
        }
        break;
    case 0xd074: /* Optimization modes */
    case 0xd075:
    case 0xd076:
    case 0xd077:
        if (mem_reg_hwenable) {
            mem_reg_optim = (addr << 6);
            mem_set_mirroring(mem_reg_optim);
        }
        break;
    case 0xd078: /* SIMM configuration */
        if (mem_reg_hwenable && mem_reg_simm != value) {
            mem_reg_simm = value;
            mem_set_simm(mem_reg_simm);
        }
        break;
    case 0xd07a: /* Software 1MHz enable */
        if (!mem_reg_soft_1mhz) {
            mem_reg_soft_1mhz = 1;
            scpu64_set_fastmode(0);
        }
        break;
    case 0xd079: /* same as 0xd07b */
    case 0xd07b: /* Software 1MHz disable */
        if (mem_reg_soft_1mhz) {
            mem_reg_soft_1mhz = 0;
            scpu64_set_fastmode(!(mem_reg_sys_1mhz || (mem_reg_sw_1mhz && !mem_reg_hwenable)));
        }
        break;
    case 0xd07c:
        break;
    case 0xd07e: /* hwreg enable */
        if (!mem_reg_hwenable) {
            mem_reg_hwenable = 1;
            scpu64_set_fastmode(!(mem_reg_sys_1mhz || mem_reg_soft_1mhz));
            mem_pla_config_changed();
        }
        break;
    case 0xd07d: /* same as 0xd07d */
    case 0xd07f: /* hwreg disable */
        if (mem_reg_hwenable) {
            mem_reg_hwenable = 0;
            scpu64_set_fastmode(!(mem_reg_sys_1mhz || mem_reg_soft_1mhz || mem_reg_sw_1mhz));
            mem_pla_config_changed();
        }
        break;
    case 0xd0b0:
    case 0xd0b1:
        break;
    case 0xd0b2: /* hwenable and set system 1 MHz */
        if (mem_reg_hwenable) {
            mem_reg_sys_1mhz = !!(value & 0x40);
            if (!(value & 0x80)) {
                mem_reg_hwenable = 0;
                mem_pla_config_changed();
            }
            scpu64_set_fastmode(!(mem_reg_sys_1mhz || mem_reg_soft_1mhz || (mem_reg_sw_1mhz && !mem_reg_hwenable)));
        }
        break;
    case 0xd0b3: /* set optim mode */
        if (mem_reg_hwenable && scpu64_version_v2) {
            mem_reg_optim = (mem_reg_optim & 0x38) | (value & 0xc7);
            mem_set_mirroring(mem_reg_optim);
        }
        break;
    case 0xd0b4: /* set optim mode */
        if (mem_reg_hwenable) {
            mem_reg_optim = (mem_reg_optim & 0x3f) | (value & 0xc0);
            mem_set_mirroring(mem_reg_optim);
        }
        break;
    case 0xd0b5:
        break;
    case 0xd0b6: /* disable bootmap */
        if (mem_reg_hwenable && mem_reg_bootmap) {
            mem_reg_bootmap = 0;
            mem_pla_config_changed();
        }
        break;
    case 0xd0b7: /* enable bootmap */
        if (mem_reg_hwenable && !mem_reg_bootmap) {
            mem_reg_bootmap = 1;
            mem_pla_config_changed();
        }
        break;
    case 0xd0b8: /* set software 1 MHz */
        if (mem_reg_hwenable) {
            mem_reg_soft_1mhz = value >> 7;
            scpu64_set_fastmode(!(mem_reg_sys_1mhz || mem_reg_soft_1mhz || (mem_reg_sw_1mhz && !mem_reg_hwenable)));
        }
        break;
    case 0xd0b9:
    case 0xd0ba:
    case 0xd0bb:
        break;
    case 0xd0bc: /* set dos extension */
        if (mem_reg_hwenable && (mem_reg_dosext != (value >> 7))) {
            mem_reg_dosext = value >> 7;
            mem_pla_config_changed();
        }
        break;
    case 0xd0be: /* dos extension enable */
        if (mem_reg_hwenable && !mem_reg_dosext) {
            mem_reg_dosext = 1;
            mem_pla_config_changed();
        }
        break;
    case 0xd0bd: /* same as 0xd0bf */
    case 0xd0bf: /* dos extension disable */
        if (mem_reg_dosext) {
            mem_reg_dosext = 0;
            mem_pla_config_changed();
        }
        break;
    default:
        break;
    }
}

static void colorram_store(uint16_t addr, uint8_t value)
{
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    mem_color_ram[addr & 0x3ff] = value & 0xf;
}

static uint8_t colorram_read(uint16_t addr)
{
    if (scpu64_version_v2) {
        return mem_sram[0x10000 + addr];
    }
    return mem_color_ram[addr & 0x3ff] | (vicii_read_phi1() & 0xf0);
}

static uint8_t scpu64_d200_read(uint16_t addr)
{
    return mem_sram[0x10000 + addr];
}

static void scpu64_d200_store(uint16_t addr, uint8_t value)
{
    if (mem_reg_hwenable || addr == 0xd27e) {
        mem_sram[0x10000 + addr] = value;
    }
}

static uint8_t scpu64_d300_read(uint16_t addr)
{
    return mem_sram[0x10000 + addr];
}

static void scpu64_d300_store(uint16_t addr, uint8_t value)
{
    if (mem_reg_hwenable) {
        mem_sram[0x10000 + addr] = value;
    }
}
/* ------------------------------------------------------------------------- */

uint8_t scpu64io_d000_read(uint16_t addr)
{
    if ((addr & 0xfff0) == 0xd0b0) {
        if (scpu64_version_v2) {
            check_ba_read();
            return scpu64_hardware_read(addr); /* not an i/o read! */
        }
        if (!dma_in_progress) {
            scpu64_clock_read_stretch_io();
        }
        return scpu64_hardware_read(addr); /* i/o read! */
    }
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d000_read(addr); /* i/o read */
}

static uint8_t scpu64io_d000_peek(uint16_t addr)
{
    if ((addr & 0xfff0) == 0xd0b0) {
        return scpu64_hardware_read(addr);
    } else {
        return c64io_d000_peek(addr);
    }
}

void scpu64io_d000_store(uint16_t addr, uint8_t value)
{
    int oldfastmode;

    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    if ((addr >= 0xd071 && addr < 0xd080) || (addr >= 0xd0b0 && addr < 0xd0c0)) {
        oldfastmode = scpu64_fastmode;
        scpu64_hardware_store(addr, value);
        if (!oldfastmode && scpu64_fastmode) {
            return; /* stretch already handled */
        }
    } else {
        c64io_d000_store(addr, value);
    }
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_d100_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d100_read(addr); /* i/o read */
}

void scpu64io_d100_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    c64io_d100_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_d200_read(uint16_t addr)
{
    check_ba_read();
    if (!scpu64_version_v2) {
        scpu64_clock_read_ioram();
    }
    return scpu64_d200_read(addr); /* not an i/o read! */
}

void scpu64io_d200_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    scpu64_d200_store(addr, value);
}

uint8_t scpu64io_d300_read(uint16_t addr)
{
    check_ba_read();
    if (!scpu64_version_v2) {
        scpu64_clock_read_ioram();
    }
    return scpu64_d300_read(addr); /* not an i/o read! */
}

void scpu64io_d300_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    scpu64_d300_store(addr, value);
}

uint8_t scpu64io_d400_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d400_read(addr); /* i/o read */
}

void scpu64io_d400_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    c64io_d400_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_d500_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d500_read(addr); /* i/o read */
}

void scpu64io_d500_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    c64io_d500_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_d600_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d600_read(addr); /* i/o read */
}

void scpu64io_d600_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch(); /* strange, but not i/o ! */
    }
    c64io_d600_store(addr, value);
}

uint8_t scpu64io_d700_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_d700_read(addr); /* i/o read */
}

void scpu64io_d700_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    c64io_d700_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_colorram_read(uint16_t addr)
{
    if (scpu64_version_v2) {
        check_ba_read();
        return mem_sram[0x10000 + addr]; /* not an i/o read! */
    }
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return mem_color_ram[addr & 0x3ff] | (vicii_read_phi1() & 0xf0); /* i/o read */
}

void scpu64io_colorram_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    colorram_store(addr, value);
}

uint8_t scpu64io_colorram_read_int(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return vicii_read_phi1();
}

void scpu64io_colorram_store_int(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch();
    }
    mem_color_ram[addr & 0x3ff] = value & 0xf;
}

uint8_t scpu64_cia1_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return cia1_read(addr); /* i/o read */
}

void scpu64_cia1_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start_cia();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    cia1_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_cia();
    }
}

uint8_t scpu64_cia2_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return cia2_read(addr); /* i/o read */
}

void scpu64_cia2_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start_cia();
    }
    if (scpu64_version_v2) mem_sram[0x10000 + addr] = value;
    cia2_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_cia();
    }
}

uint8_t scpu64io_de00_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_de00_read(addr); /* i/o read */
}

void scpu64io_de00_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    c64io_de00_store(addr, value);
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64io_df00_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return c64io_df00_read(addr); /* i/o read */
}

void scpu64io_df00_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    c64io_df00_store(addr, value);
    switch (addr) {
    case 0xdf01:
    case 0xdf21:
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io_long();
        }
        break;
    case 0xdf7e:
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io(); /* TODO: verify */
        }
        mem_reg_ramlink = 1;
        break;
    case 0xdf7f:
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io(); /* TODO: verify */
        }
        mem_reg_ramlink = 0;
        break;
    default:
        if (!dma_in_progress) {
            scpu64_clock_write_stretch_io();
        }
        break;
    }
}

uint8_t scpu64_roml_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return roml_read(addr); /* i/o read */
}

void scpu64_roml_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    roml_store(addr, value); /* i/o write */
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64_romh_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return romh_read(addr); /* i/o read */
}

void scpu64_romh_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    romh_store(addr, value); /* i/o write */
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64_ultimax_1000_7fff_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return ultimax_1000_7fff_read(addr); /* i/o read */
}

void scpu64_ultimax_1000_7fff_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    ultimax_1000_7fff_store(addr, value); /* i/o write */
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64_ultimax_a000_bfff_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return ultimax_a000_bfff_read(addr); /* i/o read */
}

void scpu64_ultimax_a000_bfff_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    ultimax_a000_bfff_store(addr, value); /* i/o write */
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

uint8_t scpu64_ultimax_c000_cfff_read(uint16_t addr)
{
    if (!dma_in_progress) {
        scpu64_clock_read_stretch_io();
    }
    return ultimax_c000_cfff_read(addr); /* i/o read */
}

void scpu64_ultimax_c000_cfff_store(uint16_t addr, uint8_t value)
{
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io_start();
    }
    ultimax_c000_cfff_store(addr, value); /* i/o write */
    if (!dma_in_progress) {
        scpu64_clock_write_stretch_io();
    }
}

/* ------------------------------------------------------------------------- */

void mem_set_write_hook(int config, int page, store_func_t *f)
{
    int j;

    for (j = 0; j < NUM_MIRRORS; j++) {
        mem_write_tab[j][config][page] = f;
    }
}

void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func)
{
    mem_read_tab[base][index] = read_func;
}

void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr)
{
    mem_read_base_tab[base][index] = mem_ptr;
}

void mem_initialize_memory(void)
{
    int i, j, l;

    mem_chargen_rom_ptr = mem_chargen_rom;
    mem_color_ram_cpu = mem_color_ram;
    mem_color_ram_vicii = mem_color_ram;

    /* setup watchpoint tables */
    mem_read_tab_watch[0] = zero_read_watch;
    mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i <= 0x100; i++) {
        mem_read_tab_watch[i] = read_watch;
        mem_write_tab_watch[i] = store_watch;
    }

    /* Default is RAM.  */
    /* normal RAM maps */
    for (i = 0; i < NUM_CONFIGS - 0x20; i++) {
        for (j = 0; j <= 0xff; j++) {
            mem_read_tab[i][j] = ram_read;
            mem_read_base_tab[i][j] = mem_sram;
            for (l = 0; l < NUM_MIRRORS; l++) {
                if (mem_mirrors[l] && (mem_mirrors[l] >> 8) <= j && (mem_mirrors[l] & 0xff) >= j) {
                    /* mirrored */
                    if (j == 0) {
                        mem_write_tab[l][i][j] = zero_store_mirrored;
                    } else if (j == 0xff) {
                        mem_write_tab[l][i][j] = ram_hi_store_mirrored;
                    } else {
                        mem_write_tab[l][i][j] = ram_store_mirrored;
                    }
                } else { /* nothing to see here */
                    if (j == 0) {
                        mem_write_tab[l][i][j] = zero_store;
                    } else if (j == 0xff) {
                        mem_write_tab[l][i][j] = ram_hi_store;
                    } else {
                        mem_write_tab[l][i][j] = ram_store;
                    }
                }
            }
        }
    }
    /* internal RAM maps */
    for (i = NUM_CONFIGS - 0x20; i < NUM_CONFIGS; i++) {
        for (j = 0; j <= 0xff; j++) {
            mem_read_tab[i][j] = ram_read_int;
            mem_read_base_tab[i][j] = mem_ram;
            for (l = 0; l < NUM_MIRRORS; l++) {
                if (j == 0) {
                    mem_write_tab[l][i][j] = zero_store_int;
                } else if (j == 0xff) {
                    mem_write_tab[l][i][j] = ram_hi_store_int;
                } else {
                    mem_write_tab[l][i][j] = ram_store_int;
                }
            }
        }
    }

    scpu64meminit();

    for (i = 0; i < NUM_CONFIGS; i++) {
        mem_read_tab[i][0x100] = mem_read_tab[i][0];
            for (l = 0; l < NUM_MIRRORS; l++) {
                mem_write_tab[l][i][0x100] = mem_write_tab[l][i][0];
            }
        mem_read_base_tab[i][0x100] = mem_read_base_tab[i][0];
    }

    /* A fully automatic limit filler ;) */
    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0, l = 1; j <= 0xff; l++) {
            uint8_t *p = mem_read_base_tab[i][j];
            read_func_ptr_t f = mem_read_tab[i][j];
            uint32_t range;

            while (l <= 0xff && p == mem_read_base_tab[i][l]) {
                l++;
            }
            /* Some areas are I/O or cartridge (NULL) or too slow and need cycle stretching */
            range = (p == NULL || f == ram_read_int || f == scpu64rom_scpu64_read || f == chargen_read) ? 0 : ((j << 24) | ((l << 8)-3));
            while (j < l) {
                mem_read_limit_tab[i][j] = range;
                j++;
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }

    vicii_set_chargen_addr_options(0x7000, 0x1000);

    mem_pport = 7;
    export.exrom = 0;
    export.game = 0;
    mem_reg_bootmap = 1;

    /* Setup initial memory configuration.  */
    mem_pla_config_changed();
    cartridge_init_config();
}

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p;
    uint32_t limits;

    if (addr >= 0x10000) {
        if (addr < 0x20000) {
            *base = mem_sram + 0x10000;
            *limit = 0xfffd;
            *start = 0x0000;
        } else if (!scpu64_fastmode) {
            if (addr >= 0xf80000) {
                *base = scpu64rom_scpu64_rom + (addr & 0x70000 & (SCPU64_SCPU64_ROM_MAXSIZE-1));
                *limit = 0xfffd;
                *start = 0x0000;
            } else if (addr >= 0xf60000 && mem_simm_ram_mask && mem_simm_page_size == mem_conf_page_size) {
                *base = mem_simm_ram + (addr & 0x10000);
                *limit = 0xfffd;
                *start = 0x0000;
            } else if (mem_simm_ram_mask && mem_simm_page_size == mem_conf_page_size && addr < (unsigned int)mem_conf_size) {
                *base = mem_simm_ram + (addr & 0xff0000 & mem_simm_ram_mask);
                *limit = 0xfffd;
                *start = 0x0000;
            } else {
                *base = NULL;
                *limit = 0;
                *start = 0;
            }
        } else {
            *base = NULL;
            *limit = 0;
            *start = 0;
        }
    } else {
        p = _mem_read_base_tab_ptr[addr >> 8];
        if (p != NULL) {
            *base = p;
            limits = mem_read_limit_tab_ptr[addr >> 8];
            *limit = limits & 0xffff;
            *start = limits >> 16;
            if (traps_pending) {
                traps_refresh();
                traps_pending = 0;
            }
        } else if (scpu64_fastmode) {
            *base = NULL;
            *limit = 0;
            *start = 0;
        } else {
            cartridge_mmu_translate(addr, base, start, limit);
        }
    }
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, SCPU64_RAM_SIZE);
    ram_init(mem_sram, SCPU64_SRAM_SIZE);
    ram_init(mem_trap_ram, SCPU64_KERNAL_ROM_SIZE);
    vicii_init_colorram(mem_color_ram);
}

/* ------------------------------------------------------------------------- */

/* Change the current video bank.  Call this routine only when the vbank
   has really changed.  */
void mem_set_vbank(int new_vbank)
{
    vicii_set_vbank(new_vbank);
}

void mem_set_mirroring(int new_mirroring)
{
    mirror = ((new_mirroring & 0x1) ? 1 : 0) | ((new_mirroring & 0x4) ? 2 : 0)
           | ((new_mirroring & 0x40) ? 4 : 0) | ((new_mirroring & 0x80) ? 8 : 0);

    /* Do not override watchpoints on vbank switches.  */
    if (_mem_write_tab_ptr != mem_write_tab_watch) {
        _mem_write_tab_ptr = mem_write_tab[mirror][mem_config];
    }
}

void mem_set_simm(int config)
{
    switch (config & 7) {
    case 0:
        mem_conf_page_size = 9 + 2;
        mem_conf_size = 1 * 1024 *1024;
        break;
    case 1:
        mem_conf_page_size = 10 + 2;
        mem_conf_size = 4 * 1024 *1024;
        break;
    case 2:
        mem_conf_page_size = 10 + 2;
        mem_conf_size = 8 * 1024 *1024;
        break;
    case 3:
        mem_conf_page_size = 10 + 2;
        mem_conf_size = 16 * 1024 *1024;
        break;
    default:
        mem_conf_page_size = 11 + 2;
        mem_conf_size = 16 * 1024 *1024;
        break;
    }
    scpu64_set_simm_row_size(mem_conf_page_size);
}

void scpu64_hardware_reset(void)
{
    mem_reg_optim = 0xc7;
    mem_reg_soft_1mhz = 0;
    mem_reg_sys_1mhz = 0;
    mem_reg_hwenable = 0;
    mem_reg_dosext = 0;
    mem_reg_ramlink = 0;
    mem_reg_bootmap = 1;
    mem_reg_simm = 4;
    mem_pport = 7;
    mem_set_mirroring(mem_reg_optim);
    mem_set_simm(mem_reg_simm);
    mem_pla_config_changed();
}

/* Set the tape nonsense status.  */
void mem_set_tape_sense(int sense)
{
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked.  */

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    if (start != NULL) {
        *start = mem_sram[0x2b] | (mem_sram[0x2c] << 8);
    }
    if (end != NULL) {
        *end = mem_sram[0x2d] | (mem_sram[0x2e] << 8);
    }
}

void mem_set_basic_text(uint16_t start, uint16_t end)
{
    mem_sram[0x2b] = mem_sram[0xac] = start & 0xff;
    mem_sram[0x2c] = mem_sram[0xad] = start >> 8;
    mem_sram[0x2d] = mem_sram[0x2f] = mem_sram[0x31] = mem_sram[0xae] = end & 0xff;
    mem_sram[0x2e] = mem_sram[0x30] = mem_sram[0x32] = mem_sram[0xaf] = end >> 8;
}

/* this function should always read from the screen currently used by the kernal
   for output, normally this does just return system ram - except when the
   videoram is not memory mapped.
   used by autostart to "read" the kernal messages
*/
uint8_t mem_read_screen(uint16_t addr)
{
    return ram_read(addr);
}

void mem_inject(uint32_t addr, uint8_t value)
{
    /* could be made to handle various internal expansions in some sane way */
    mem_ram[addr & 0xffff] = mem_sram[addr & 0xffff] = value;
}

/* In banked memory architectures this will always write to the bank that
   contains the keyboard buffer and "number of keys in buffer", regardless of
   what the CPU "sees" currently.
   In all other cases this just writes to the first 64kb block, usually by
   wrapping to mem_inject().
*/
void mem_inject_key(uint16_t addr, uint8_t value)
{
    mem_inject(addr, value);
}

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(uint16_t addr)
{
    if (addr >= 0xe000) {
        switch (mem_config) {
            case 2:
            case 3:
            case 6:
            case 7:
            case 10:
            case 11:
            case 14:
            case 15:
            case 26:
            case 27:
            case 30:
            case 31:
                return 1;
            default:
                return 0;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

void store_bank_io(uint16_t addr, uint8_t byte)
{
    switch (addr & 0xff00) {
        case 0xd000:
            if ((addr >= 0xd071 && addr < 0xd080) || (addr >= 0xd0b0 && addr < 0xd0c0)) {
                scpu64_hardware_store(addr, byte);
            } else {
                c64io_d000_store(addr, byte);
            }
            break;
        case 0xd100:
            c64io_d100_store(addr, byte);
            break;
        case 0xd200:
            scpu64_d200_store(addr, byte);
            break;
        case 0xd300:
            scpu64_d300_store(addr, byte);
            break;
        case 0xd400:
            c64io_d400_store(addr, byte);
            break;
        case 0xd500:
            c64io_d500_store(addr, byte);
            break;
        case 0xd600:
            c64io_d600_store(addr, byte);
            break;
        case 0xd700:
            c64io_d700_store(addr, byte);
            break;
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            colorram_store(addr, byte);
            break;
        case 0xdc00:
            cia1_store(addr, byte);
            break;
        case 0xdd00:
            cia2_store(addr, byte);
            break;
        case 0xde00:
            c64io_de00_store(addr, byte);
            break;
        case 0xdf00:
            c64io_df00_store(addr, byte);
            break;
    }
    return;
}

uint8_t read_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            if ((addr & 0xfff0) == 0xd0b0) {
                return scpu64_hardware_read(addr);
            }
            return c64io_d000_read(addr);
        case 0xd100:
            return c64io_d100_read(addr);
        case 0xd200:
            return scpu64_d200_read(addr);
        case 0xd300:
            return scpu64_d300_read(addr);
        case 0xd400:
            return c64io_d400_read(addr);
        case 0xd500:
            return c64io_d500_read(addr);
        case 0xd600:
            return c64io_d600_read(addr);
        case 0xd700:
            return c64io_d700_read(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_read(addr);
        case 0xdd00:
            return cia2_read(addr);
        case 0xde00:
            return c64io_de00_read(addr);
        case 0xdf00:
            return c64io_df00_read(addr);
    }
    return 0xff;
}

static uint8_t peek_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            return scpu64io_d000_peek(addr);
        case 0xd100:
            return c64io_d100_peek(addr);
        case 0xd200:
            return scpu64_d200_read(addr);
        case 0xd300:
            return scpu64_d300_read(addr);
        case 0xd400:
            return c64io_d400_peek(addr);
        case 0xd500:
            return c64io_d500_peek(addr);
        case 0xd600:
            return c64io_d600_peek(addr);
        case 0xd700:
            return c64io_d700_peek(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_peek(addr);
        case 0xdd00:
            return cia2_peek(addr);
        case 0xde00:
            return c64io_de00_peek(addr);
        case 0xdf00:
            return c64io_df00_peek(addr);
    }
    return 0xff;
}

/* ------------------------------------------------------------------------- */

int scpu64_interrupt_reroute(void)
{
    return (_mem_read_tab_ptr[0xff] == scpu64_kernalshadow_read || _mem_read_tab_ptr[0xff] == ram1_read) && (!scpu64_emulation_mode || mem_reg_hwenable || mem_reg_sys_1mhz || mem_reg_dosext || mem_reg_ramlink);
}

/* ------------------------------------------------------------------------- */

/* Exported banked memory access functions for the monitor.  */
#define MAXBANKS (6 + 0x100)

static const char *banknames[MAXBANKS + 1] = {
    "default",
    "cpu",
    "ram",
    "rom",
    "io",
    "cart",
    /* by convention, a "bank array" has a 2-hex-digit bank index appended */
    "ram00", "ram01", "ram02", "ram03", "ram04", "ram05", "ram06", "ram07",
    "ram08", "ram09", "ram0a", "ram0b", "ram0c", "ram0d", "ram0e", "ram0f",
    "ram10", "ram11", "ram12", "ram13", "ram14", "ram15", "ram16", "ram17",
    "ram18", "ram19", "ram1a", "ram1b", "ram1c", "ram1d", "ram1e", "ram1f",
    "ram20", "ram21", "ram22", "ram23", "ram24", "ram25", "ram26", "ram27",
    "ram28", "ram29", "ram2a", "ram2b", "ram2c", "ram2d", "ram2e", "ram2f",
    "ram30", "ram31", "ram32", "ram33", "ram34", "ram35", "ram36", "ram37",
    "ram38", "ram39", "ram3a", "ram3b", "ram3c", "ram3d", "ram3e", "ram3f",
    "ram40", "ram41", "ram42", "ram43", "ram44", "ram45", "ram46", "ram47",
    "ram48", "ram49", "ram4a", "ram4b", "ram4c", "ram4d", "ram4e", "ram4f",
    "ram50", "ram51", "ram52", "ram53", "ram54", "ram55", "ram56", "ram57",
    "ram58", "ram59", "ram5a", "ram5b", "ram5c", "ram5d", "ram5e", "ram5f",
    "ram60", "ram61", "ram62", "ram63", "ram64", "ram65", "ram66", "ram67",
    "ram68", "ram69", "ram6a", "ram6b", "ram6c", "ram6d", "ram6e", "ram6f",
    "ram70", "ram71", "ram72", "ram73", "ram74", "ram75", "ram76", "ram77",
    "ram78", "ram79", "ram7a", "ram7b", "ram7c", "ram7d", "ram7e", "ram7f",
    "ram80", "ram81", "ram82", "ram83", "ram84", "ram85", "ram86", "ram87",
    "ram88", "ram89", "ram8a", "ram8b", "ram8c", "ram8d", "ram8e", "ram8f",
    "ram90", "ram91", "ram92", "ram93", "ram94", "ram95", "ram96", "ram97",
    "ram98", "ram99", "ram9a", "ram9b", "ram9c", "ram9d", "ram9e", "ram9f",
    "rama0", "rama1", "rama2", "rama3", "rama4", "rama5", "rama6", "rama7",
    "rama8", "rama9", "ramaa", "ramab", "ramac", "ramad", "ramae", "ramaf",
    "ramb0", "ramb1", "ramb2", "ramb3", "ramb4", "ramb5", "ramb6", "ramb7",
    "ramb8", "ramb9", "ramba", "rambb", "rambc", "rambd", "rambe", "rambf",
    "ramc0", "ramc1", "ramc2", "ramc3", "ramc4", "ramc5", "ramc6", "ramc7",
    "ramc8", "ramc9", "ramca", "ramcb", "ramcc", "ramcd", "ramce", "ramcf",
    "ramd0", "ramd1", "ramd2", "ramd3", "ramd4", "ramd5", "ramd6", "ramd7",
    "ramd8", "ramd9", "ramda", "ramdb", "ramdc", "ramdd", "ramde", "ramdf",
    "rame0", "rame1", "rame2", "rame3", "rame4", "rame5", "rame6", "rame7",
    "rame8", "rame9", "ramea", "rameb", "ramec", "ramed", "ramee", "ramef",
    "ramf0", "ramf1", "ramf2", "ramf3", "ramf4", "ramf5", "ramf6", "ramf7",
    "romf8", "romf9", "romfa", "romfb", "romfc", "romfd", "romfe", "romff",
    NULL
};

static const int banknums[MAXBANKS + 1] =
{
    0,
    0,
    1,
    2,
    3,
    4,

    5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
    85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
    117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
    149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
    197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
    213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
    229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244,
    245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260,

    -1
};

static const int bankindex[MAXBANKS + 1] =
{
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,

    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,

    -2
};

static const int bankflags[MAXBANKS + 1] =
{
    0,
    0,
    0,
    0,
    0,
    0,

    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST , MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST,

    -2
};

const char **mem_bank_list(void)
{
    return banknames;
}

const int *mem_bank_list_nos(void) {
    return banknums;
}

#if 0
const int *mem_bank_list_index(void) {
    return bankindex;
}

const int *mem_bank_list_flags(void) {
    return bankindex;
}
#endif

/* return bank number for a given literal bank name */
int mem_bank_from_name(const char *name)
{
    int i = 0;

    while (banknames[i]) {
        if (!strcmp(name, banknames[i])) {
            return banknums[i];
        }
        i++;
    }
    return -1;
}

/* return current index for a given bank */
int mem_bank_index_from_bank(int bank)
{
    int i = 0;

    while (banknums[i] > -1) {
        if (banknums[i] == bank) {
            return bankindex[i];
        }
        i++;
    }
    return -1;
}

int mem_bank_flags_from_bank(int bank)
{
    int i = 0;

    while (banknums[i] > -1) {
        if (banknums[i] == bank) {
            return bankflags[i];
        }
        i++;
    }
    return -1;
}

/* read memory with side-effects */
uint8_t mem_bank_read(int bank, uint16_t addr, void *context)
{
    if ((bank >= 5) && (bank <= 6)) {
        return mem_sram[((bank - 5) << 16) + addr]; /* ram00..01 */
    }
    if ((bank >= 7) && (bank <= 252)) {
        int addr2 = addr + ((bank - ((bank >= 251) ? 251 : 5)) << 16);
        if (mem_simm_page_size != mem_conf_page_size) {
            addr2 = ((addr2 >> mem_conf_page_size) << mem_simm_page_size) | (addr2 & ((1 << mem_simm_page_size)-1));
        }
        if (mem_simm_ram_mask && addr2 < mem_conf_size) {
            return mem_simm_ram[addr2 & mem_simm_ram_mask]; /* ram02..f6 */
        }
        return bank - 5;
    }
    if ((bank >= 253) && (bank <= 260)) {
        return scpu64rom_scpu64_rom[(((bank - 253) << 16) + addr) & (SCPU64_SCPU64_ROM_MAXSIZE-1)]; /* romf8..ff */
    }

    switch (bank) {
        case 0:                   /* current */
            bank = WDC65816_REGS_GET_PBR(maincpu_monitor_interface->cpu_65816_regs);
            if (bank > 0) {
                return mem_peek2(addr + (bank << 16));
            }
            return mem_read(addr);
        case 3:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                return read_bank_io(addr);
            }
            /* FALL THROUGH */
        case 4:                   /* cart */
            return cartridge_peek_mem(addr);
        case 2:                   /* rom */
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return ram1_read(addr);
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return mem_chargen_rom[addr & 0x0fff];
            }
            if (addr >= 0xe000) {

                return mem_reg_hwenable ? scpu64_kernalshadow_read(addr) : ram1_read(addr);
            }
            /* FALL THROUGH */
        case 1:                   /* ram */
            break;
    }
    return mem_sram[addr];
}

/* read memory without side-effects */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    if ((bank >= 5) && (bank <= 260)) {
        return mem_bank_read(bank, addr, context); /* ram00..ff */
    }
    switch (bank) {
        case 0:                   /* current */
            bank = WDC65816_REGS_GET_PBR(maincpu_monitor_interface->cpu_65816_regs);
            if (bank > 0) {
                return mem_peek2(addr + (bank << 16));
            }
            /* we must check for which bank is currently active, and only use peek_bank_io
               when needed to avoid side effects */
            if ((addr >= 0xd000) && (addr < 0xe000)) {
                if (_mem_read_base_tab_ptr[0xd2] == mem_sram + 0x10000) {
                    return peek_bank_io(addr);
                }
            }
            return mem_read(addr);
            break;
        case 3:                   /* io */
            if ((addr >= 0xd000) && (addr < 0xe000)) {
                return peek_bank_io(addr);
            }
            break;
        case 4:                   /* cart */
            return cartridge_peek_mem(addr);
    }
    return mem_bank_read(bank, addr, context);
}

int mem_get_current_bank_config(void) {
    return 0; /* TODO: not implemented yet */
}

uint8_t mem_peek_with_config(int config, uint16_t addr, void *context) {
    /* TODO, config not implemented yet */
    return mem_bank_peek(0 /* current */, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    if ((bank >= 5) && (bank <= 6)) {
        mem_sram[((bank - 5) << 16) + addr] = byte; /* ram00..01 */
        return;
    }
    if ((bank >= 7) && (bank <= 252)) {
        int addr2 = addr + ((bank - ((bank >= 251) ? 251 : 5)) << 16);
        if (mem_simm_page_size != mem_conf_page_size) {
            addr2 = ((addr2 >> mem_conf_page_size) << mem_simm_page_size) | (addr2 & ((1 << mem_simm_page_size)-1));
        }
        if (mem_simm_ram_mask && addr2 < mem_conf_size) {
            mem_simm_ram[addr2 & mem_simm_ram_mask] = byte; /* ram02..f6 */
        }
        return;
    }
    if ((bank >= 253) && (bank <= 260)) {
        scpu64rom_scpu64_rom[(((bank - 253) << 16) + addr) & (SCPU64_SCPU64_ROM_MAXSIZE-1)] = byte; /* romf8..ff */
        return;
    }
    switch (bank) {
        case 0:                   /* current */
            bank = WDC65816_REGS_GET_PBR(maincpu_monitor_interface->cpu_65816_regs);
            if (bank > 0) {
                mem_store2(addr + (bank << 16), byte);
                return;
            }
            mem_store(addr, byte);
            return;
        case 3:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                store_bank_io(addr, byte);
                return;
            }
            /* FALL THROUGH */
        case 2:                   /* rom */
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return;
            }
            if (addr >= 0xe000) {
                return;
            }
            /* FALL THROUGH */
        case 1:                   /* ram */
            break;
    }
    mem_sram[addr] = byte;
}

/* used by monitor if sfx off */
void mem_bank_poke(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_bank_write(bank, addr, byte, context);
}

static int mem_dump_io(void *context, uint16_t addr)
{
    if ((addr >= 0xdc00) && (addr <= 0xdc3f)) {
        return ciacore_dump(machine_context.cia1);
    } else if ((addr >= 0xdd00) && (addr <= 0xdd3f)) {
        return ciacore_dump(machine_context.cia2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    io_source_ioreg_add_list(&mem_ioreg_list);  /* VIC-II, SID first so it's in address order */

    mon_ioreg_add_list(&mem_ioreg_list, "CIA1", 0xdc00, 0xdc0f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA2", 0xdd00, 0xdd0f, mem_dump_io, NULL, IO_MIRROR_NONE);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    *base = ((vicii_peek(0xd018) & 0xf0) << 6) | ((~cia2_peek(0xdd00) & 0x03) << 14);
    *rows = 25;
    *columns = 40;
    *bank = 0;
}

/* used by autostart to locate and "read" kernal output on the current screen
 * this function should return whatever the kernal currently uses, regardless
 * what is currently visible/active in the UI
 */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column, uint8_t *line_length, int *blinking)
{
    /* Cursor Blink enable: 1 = Flash Cursor, 0 = Cursor disabled, -1 = n/a */
    *blinking = mem_sram[0xcc] ? 0 : 1;
    *screen_addr = mem_sram[0xd1] + mem_sram[0xd2] * 256; /* Cursor Blink enable: 0 = Flash Cursor */
    *cursor_column = mem_sram[0xd3];    /* Cursor Column on Current Line */
    *line_length = mem_sram[0xd5] + 1;  /* Physical Screen Line Length */
}

/* ------------------------------------------------------------------------- */

void mem_set_simm_size(int val)
{
    unsigned int size = val << 20;

    if (size == 0) {
        size = 1;
    }
    mem_simm_ram_mask = size - 1;
    mem_simm_ram = lib_realloc(mem_simm_ram, size);
    ram_init(mem_simm_ram, size);

    switch (val) {
        case 1:
            mem_simm_page_size = 9 + 2; /* 0 */
            break;
        case 4:                             /* 1 */
        case 8:                             /* 2 */
            mem_simm_page_size = 10 + 2;  /* 3 */
            break;
        default:
            mem_simm_page_size = 11 + 2;  /* 4,3 */
            break;
    }
    maincpu_resync_limits();
}

void mem_set_jiffy_switch(int val)
{
    mem_reg_sw_jiffy = !!val;
}

void mem_set_speed_switch(int val)
{
    if (mem_reg_sw_1mhz == val) {
        mem_reg_sw_1mhz = !val;
        scpu64_set_fastmode_nosync(!(mem_reg_soft_1mhz || mem_reg_sys_1mhz || (mem_reg_sw_1mhz && !mem_reg_hwenable)));
    }
}

/* ------------------------------------------------------------------------- */

uint8_t scpu64_trap_read(uint16_t addr)
{
    return mem_trap_ram[addr & 0x1fff];
}

void scpu64_trap_store(uint16_t addr, uint8_t value)
{
    mem_trap_ram[addr & 0x1fff] = value;
}

void mem_color_ram_to_snapshot(uint8_t *color_ram)
{
    memcpy(color_ram, mem_color_ram, 0x400);
}

void mem_color_ram_from_snapshot(uint8_t *color_ram)
{
    memcpy(mem_color_ram, color_ram, 0x400);
}

void scpu64_mem_shutdown(void)
{
    lib_free(mem_simm_ram);
    mem_simm_ram = NULL;
}
