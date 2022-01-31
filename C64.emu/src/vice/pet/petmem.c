/*
 * petmem.c - PET memory handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "6809.h"
#include "cartio.h"
#include "cartridge.h"
#include "crtc-mem.h"
#include "crtctypes.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "pet.h"
#include "pet-resources.h"
#include "petacia.h"
#include "petcolour.h"
#include "petdww.h"
#include "pethre.h"
#include "petmem.h"
#include "petmodel.h"
#include "petpia.h"
#include "pets.h"
#include "petvia.h"
#include "ram.h"
#include "resources.h"
#include "types.h"
#include "via.h"
#include "vsync.h"

static uint8_t mem_read_patchbuf(uint16_t addr);
static void mem_initialize_memory_6809_flat(void);

uint8_t petmem_2001_buf_ef[256];

/* ------------------------------------------------------------------------- */

/*
 * We keep the current system config in here.
 * It is initialized to defaults from the default resource values.
 */

petres_t petres;

/* if the above is true, there's no need for this: */
#if 0
= {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    { NULL, NULL, NULL, NULL, NULL, NULL }, /* h6809romName[] */
    0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

/* ------------------------------------------------------------------------- */

/* The PET memory. */

#define RAM_ARRAY 0x20000 /* this includes 8x96 expansion RAM */

uint8_t mem_ram[RAM_ARRAY]; /* 128K to make things easier. Real size is 4-128K. */
uint8_t mem_rom[PET_ROM_SIZE];
uint8_t mem_chargen_rom[PET_CHARGEN_ROM_SIZE];
uint8_t mem_6809rom[PET_6809_ROMSIZE];

#define EXT_RAM         (64 * 1024)

static int ram_size = RAM_ARRAY;       /* FIXME? */

/* Memory read and write tables. */
static read_func_ptr_t _mem_read_tab[0x101];
static store_func_ptr_t _mem_write_tab[0x101];
static read_func_ptr_t _mem_read_tab_watch[0x101];
static store_func_ptr_t _mem_write_tab_watch[0x101];
static uint8_t *_mem_read_base_tab[0x101];
static int mem_read_limit_tab[0x101];

read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
static uint8_t **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

/* 8x96 mapping register */
uint8_t petmem_map_reg = 0;
#define FFF0_ENABLED                0x80
#define FFF0_IO_PEEK_THROUGH        0x40
#define FFF0_SCREEN_PEEK_THROUGH    0x20
/* Internal jumper, used by HRE board */
uint8_t petmem_ramON = 0;

static int bank8offset = 0;
static int bankCoffset = 0;

/* memory tables for the 6809 */
static read_func_ptr_t _mem6809_read_tab[0x101];
static store_func_ptr_t _mem6809_write_tab[0x101];
static read_func_ptr_t _mem6809_read_tab_watch[0x101];
static store_func_ptr_t _mem6809_write_tab_watch[0x101];
static uint8_t *_mem6809_read_base_tab[0x101];
static int mem6809_read_limit_tab[0x101];

read_func_ptr_t *_mem6809_read_tab_ptr;
store_func_ptr_t *_mem6809_write_tab_ptr;

static log_t pet_mem_log = LOG_ERR;

static uint8_t last_access = 0;

/* Current watchpoint state. 
          0 = no watchpoints
    bit0; 1 = watchpoints active
    bit1; 2 = watchpoints trigger on dummy accesses
*/
static int watchpoints_active = 0;

/* ------------------------------------------------------------------------- */

uint8_t zero_read(uint16_t addr)
{
    last_access = mem_ram[addr & 0xff];
    return last_access;
}

void zero_store(uint16_t addr, uint8_t value)
{
    mem_ram[addr & 0xff] = value;
    last_access = value;
}

static uint8_t ram_read(uint16_t addr)
{
    last_access = mem_ram[addr];
    return last_access;
}

static void ram_store(uint16_t addr, uint8_t value)
{
    mem_ram[addr] = value;
    last_access = value;
}

static uint8_t read_ext8(uint16_t addr)
{
    last_access = mem_ram[addr + bank8offset];
    return last_access;
}

static void store_ext8(uint16_t addr, uint8_t value)
{
    mem_ram[addr + bank8offset] = value;
    last_access = value;
}

static uint8_t read_extC(uint16_t addr)
{
    last_access = mem_ram[addr + bankCoffset];
    return last_access;
}

static void store_extC(uint16_t addr, uint8_t value)
{
    mem_ram[addr + bankCoffset] = value;
    last_access = value;
}

/*
 * Map $8400-$87FF to $8000-$83FF and $8C00-$8FFF to $8800-$8BFF.
 * This is only relevant for 40 column models, since 80 column models
 * don't have a mirror image of the screen memory.
 * Above $8800 there is normally empty space, but the colour extension
 * places its colour memory there, which in turn may have a mirror
 * if it is the 40 column model.
 */
static uint8_t read_vmirror(uint16_t addr)
{
    last_access = mem_ram[0x8000 + (addr & 0x0bff)];   /* 0x3FF + 0x800 */
    return last_access;
}

static void store_vmirror(uint16_t addr, uint8_t value)
{
    mem_ram[0x8000 + (addr & 0xbff)] = value;
    last_access = value;
}

/*
 * On the 2001, the very first model, the 1K of screen was mirrored
 * through the whole $8xxx block.
 */
static uint8_t read_vmirror_2001(uint16_t addr)
{
    last_access = mem_ram[0x8000 + (addr & 0x03ff)];
    return last_access;
}

static void store_vmirror_2001(uint16_t addr, uint8_t value)
{
    mem_ram[0x8000 + (addr & 0x3ff)] = value;
    last_access = value;
}

uint8_t rom_read(uint16_t addr)
{
    last_access = mem_rom[addr & 0x7fff];
    return last_access;
}

void rom_store(uint16_t addr, uint8_t value)
{
    mem_rom[addr & 0x7fff] = value;
    last_access = value;
}

#define ROM6809_BASE    0xA000

static uint8_t rom6809_read(uint16_t addr)
{
    last_access = mem_6809rom[addr - ROM6809_BASE];
    return last_access;
}

#if 0
static void rom6809_store(uint16_t addr, uint8_t value)
{
    mem_6809rom[addr - ROM6809_BASE] = value;
    last_access = value;
}
#endif

uint8_t read_unused(uint16_t addr)
{
    return last_access;
}

static uint8_t read_io_88_8f(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0x8800:
            return petio_8800_read(addr);
        case 0x8900:
            return petio_8900_read(addr);
        case 0x8a00:
            return petio_8a00_read(addr);
        case 0x8b00:
            return petio_8b00_read(addr);
        case 0x8c00:
            return petio_8c00_read(addr);
        case 0x8d00:
            return petio_8d00_read(addr);
        case 0x8e00:
            return petio_8e00_read(addr);
        case 0x8f00:
            return petio_8f00_read(addr);
    }

    return last_access;
}

static uint8_t read_io_e9_ef(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xe900:
            return petio_e900_read(addr);
        case 0xea00:
            return petio_ea00_read(addr);
        case 0xeb00:
            return petio_eb00_read(addr);
        case 0xec00:
            return petio_ec00_read(addr);
        case 0xed00:
            return petio_ed00_read(addr);
        case 0xee00:
            return petio_ee00_read(addr);
        case 0xef00:
            return petio_ef00_read(addr);
    }

    return last_access;
}

static uint8_t mem_read_patchbuf(uint16_t addr)
{
    last_access = petmem_2001_buf_ef[addr & 0xff];
    return last_access;
}

/* ------------------------------------------------------------------------- */

/* Functions for watchpoint memory access.  */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[addr >> 8](addr, value);
}

static uint8_t read6809_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem6809_read_tab[addr >> 8](addr);
}

static void store6809_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem6809_write_tab[addr >> 8](addr, value);
}

/* ------------------------------------------------------------------------- */

/* SuperPET handling
 *
 * This adds some write-only registers at $eff*, an ACIA at $eff0 and
 * 64k RAM that are mapped in 4k pages at $9***.
 * Here the 8x96 expansion RAM doubles as the SuperPET banked RAM.
 * There is also a dongle (6702) at $efe0.
 */

int spet_ramen = 1;
int spet_bank = 0;
static uint8_t *spet_bank_ptr;
int spet_ctrlwp = 1;
int spet_diag = 0;
int spet_ramwp = 0;
int spet_flat_mode = 0;         /* This is for the extra TPUG-designed */
int spet_firq_disabled = 0;     /* ...Super OS/9 MMU. */
#define DEBUG_DONGLE    0

/* Internal state of the 6702 dongle */
#define DONGLE_MAGIC    (128 + 64 + 16 + 4 + 2) /* = 214 = $D6 = %1101 0110 */
static const int leftmost[8] = {
    1 << (6 - 1),       /*   1 The size of each shift register is 6, 3, 7... */
    1 << (3 - 1),       /*   2 and therefore those are also the periods of */
    1 << (7 - 1),       /*   4 the output bits. */
    1 << (8 - 1),       /*   8 */
    1 << (1 - 1),       /*  16 */
    1 << (3 - 1),       /*  32 */
    1 << (5 - 1),       /*  64 */
    1 << (2 - 1),       /* 128 */
};
struct dongle6702_s dongle6702;

static void reset6702(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if ((1 << i) & (DONGLE_MAGIC | 1)) {
            dongle6702.shift[i] = leftmost[i];
        } else {
            dongle6702.shift[i] = 0;
        }
    }
    dongle6702.val = DONGLE_MAGIC;
    dongle6702.prevodd = 1;
    dongle6702.wantodd = 0;
}

static inline uint8_t read6702(void)
{
    last_access = dongle6702.val;
    return last_access;
}

/*
 * Only the first odd value which is written after
 * an even value has an effect.
 *
 * The actual odd value is also not important, but how it differs from
 * the previous odd value is what counts.
 *
 * There are 8 circular shift registers in the 6702. They have different
 * lengths.  Each time that the odd value is written, they shift one position
 * to the right.
 *
 * There is also a register with 8 output bits, one belonging to each
 * shift register.  Each time a 1 "shifts out of" a shift register, it
 * toggles its output register bit. The shift register is circular, so
 * the register's bit gets fed back to the left.
 *
 * For each input bit (from the odd value) which differs from the
 * previous odd value, the bit at the far end of its shift register is
 * toggled.  You will see the effect on the output only some even/odd
 * rounds in the future.
 *
 * The odd value is remembered to be compared with the next odd value.
 *
 * The output register is the value that is read from the 6702.
 *
 * William Levak had a slightly different model in mind but it works
 * out the same. He wrote the following Basic program to mimic it:
 * 
100 dims(7,7)
110 l(0)=6:l(1)=3:l(2)=7:l(3)=8:l(4)=1:l(5)=3:l(6)=5:l(7)=2
120 b(0)=1:b(1)=2:b(2)=4:b(3)=8:b(4)=16:b(5)=32:b(6)=64:b(7)=128
130 t=214:ln=214
140 t2=peek(61408):ift<>t2thene=e+1
150 printn,t,t2,e
160 rem n2=int(256*rnd(ti)):print"next number?"n2:goto180
170 input"next number";n2
180 poke61408,n2
190 if(n2andeo)=0theneo=1-(n2and1):goto160
200 mk=(lnorn2)-(lnandn2)
210 fori=0to7:ifmkandb(i)thens(i,c(i))=1-s(i,c(i))
220 next                                                                 
230 fori=0to7:c(i)=c(i)+1:ifc(i)=l(i)thenc(i)=0                            
240 next:ln=n2:eo=1-(n2and1)                                                 
250 fori=0to7:ifs(i,c(i))=0goto280                                             
260 ifb(i)andtthent=int(t-b(i)):goto280                                         
270 t=b(i)ort
280 next
290 n=n+1:goto140
 *
 * Thanks to Ruud Baltissen, William Levak, Rob Clarke,
 * Kajtar Zsolt and Segher Boessenkool, Dave E Roberts,
 * from cbm-hackers, for their contributions.
 * -Olaf Seibert.
 */
static inline void write6702(uint8_t input)
{
    last_access = input;

    if ((input & 1) == dongle6702.wantodd) {
        if (dongle6702.wantodd) {
            int i;
            int v = dongle6702.val;
            int changed = dongle6702.prevodd ^ input;
            int mask = 0x80;

            /* loop over all 8 output bits / shift registers */
            for (i = 7; i >= 0; i--, mask >>= 1) {
                /* If the input bit changed: toggle leftmost bit */
                if (changed & mask) {
                    dongle6702.shift[i] ^= leftmost[i];
                }
                /* The rightmost bit (the one that gets shifted out in
                 * a moment) determines if the output changes.
                 * I.e., if the rightmost bit is set, toggle it in v.
                 * Also simulate a rotate of the shift register by setting
                 * a 1 just left of the leftmost bit, so that the
                 * subsequent right-shift will put a 1 at the left.
                 * (If there is a 0, all this is a no-op.)
                 */
                if (dongle6702.shift[i] & 1) {
                    v ^= mask;
                    dongle6702.shift[i] |= leftmost[i] << 1; /* wrap bit around */
                }
                dongle6702.shift[i] >>= 1;
            }

            dongle6702.prevodd = input;
            dongle6702.val = v;
        }
        dongle6702.wantodd ^= 1;
    }
}

static int efe0_dump(void)
{
    int i;
    int mask = 1;

    mon_out("efe0 = $%02x; previous in = $%02x; odd/even = %d\n",
            (unsigned int)dongle6702.val,
            (unsigned int)dongle6702.prevodd,
            dongle6702.wantodd);
    for (i = 0; i < 8; i++, mask <<= 1) {
        int j;
        int maskj;
        int sh = dongle6702.shift[i];
        int lm = leftmost[i];

        mon_out("%d %3d: $%02x  %%", i, mask, (unsigned int)sh);

        for (j = 7, maskj = 1 << j; j >= 0; j--, maskj >>= 1) {
            if (maskj > lm) {
                mon_out(" ");
            } else if (sh & maskj) {
                mon_out("1");
            } else {
                mon_out("0");
            }
        }
        mon_out("\n");
    }

    return 0;
}

void set_spet_bank(int banknr)
{
    spet_bank_ptr = &mem_ram[EXT_RAM + (banknr << 12)];
}

void petmem_reset(void)
{
    spet_ramen = 1;
    set_spet_bank(0);
    spet_ctrlwp = 1;
    spet_flat_mode = 0;
    spet_firq_disabled = 0;

    petmem_map_reg = 0;
    petmem_ramON = 0;
#if DEBUG_DONGLE
    printf("reset 6702\n");
#endif
    reset6702();
}

static void superpet_mem_powerup(void)
{
/* Those two are not reset by a soft reset (/RES), only by power down */
    spet_diag = 0;
    spet_ramwp = 0;     /* should look at hardware switch */
}

int petmem_superpet_diag(void)
{
    return petres.superpet && spet_diag;
}

static uint8_t read_super_io(uint16_t addr)
{
    if (addr >= 0xeff4) {       /* unused / readonly */
        return last_access;
    } else if (addr >= 0xeff0) {       /* ACIA */
        last_access = acia1_read((uint16_t)(addr & 0x03));
    } else if ((addr & 0x0010) == 0) {
        /* Dongle E F xxx0 xxxx, see zimmers.net,
         * schematics/computers/pet/SuperPET/324055.gif.
         * Typical address is $EFE0, possibly EFE0...3.
         */
        if (addr >= 0xefe0 && addr < 0xefe4) {
            last_access = read6702();
#if DEBUG_DONGLE
            log_message(pet_mem_log, "*** DONGLE %04x -> 0x%02X %3d", addr, last_access, last_access);
#endif /* DEBUG_DONGLE */
        } else {
            last_access = 0xff;
        }
    }
    return last_access;   /* fallback */
}

static void store_super_io(uint16_t addr, uint8_t value)
{
    last_access = value;

    if (addr >= 0xeffe) {       /* RAM/ROM switch */
        spet_ramen = !(value & 1);
        /* printf("spet_ramen := %d\n", spet_ramen); */
    } else
    if (addr >= 0xeffc) {       /* Bank select */
        set_spet_bank(value & 0x0F);
        spet_firq_disabled = (value & 0x20);
        spet_flat_mode = (value & 0x40);
        spet_ctrlwp = !(value & 0x80);
        /* printf("spet_bank := %x  ", spet_bank);
           printf("spet_flat_mode := %d  ", !!spet_flat_mode);
           printf("spet_firq_disabled := %d  ", !!spet_firq_disabled);
           printf("spet_ctrlwp := %d\n", !!spet_ctrlwp); */
        if (spet_flat_mode) {
            /* This is for the extra TPUG-designed Super OS/9 MMU.
             * There is no need to check if this is a change in value,
             * since in the new state there is no access to I/O;
             * it it switched off by the SYNC instruction.
             * See http://mikenaberezny.com/hardware/superpet/super-os9-mmu/
             */
            mem_initialize_memory_6809_flat();
            /* mon_bank(e_default_space, "extram");
               extern WORD PC;
                printf("next opcode: %04X: banked %02X, flat %02X\n",
                       PC,
                       mem_ram[EXT_RAM + spet_bank_4k + (PC & 0x0FFF)],
                       mem_ram[EXT_RAM + PC]
                  ); */
        }
        /* else if (spet_bank_4k != old_spet_bank_4k) {
         *      maincpu_resync_limits(); notyet: 6809 doesn't use bank_base yet.
         * }
         */
    } else {
        if (addr >= 0xeff8) {
            if (!spet_ctrlwp) {
                if (!(value & 1)) {
                    log_error(pet_mem_log, "SuperPET: switching to 6809 not emulated!");
                    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
                }
                spet_ramwp = !(value & 0x2);    /* IF hardware w/p switch is PROG */
                /* printf("spet_ramwp := %d\n", spet_ramwp); */
                spet_diag = (value & 0x8);
            }
        } else
        if (addr >= 0xeff4) {   /* unused */
        } else
        if (addr >= 0xeff0) {   /* ACIA */
            acia1_store((uint16_t)(addr & 0x03), value);
        } else if (addr >= 0xefe0 && addr < 0xefe4) { /* dongle */
#if DEBUG_DONGLE
            log_message(pet_mem_log, "*** DONGLE %04x := %02X %3d", addr, value, value);
#endif
            write6702(value);
        }
    }
}

static uint8_t read_super_9(uint16_t addr)
{
    if (spet_ramen) {
        last_access = spet_bank_ptr[addr & 0x0fff];
    } else {
        last_access = rom_read(addr);
    }
    return last_access;
}

static void store_super_9(uint16_t addr, uint8_t value)
{
    last_access = value;

    if (spet_ramen && !spet_ramwp) {
        spet_bank_ptr[addr & 0x0fff] = value;
    }
}

static uint8_t read_super_flat(uint16_t addr)
{
    last_access = (mem_ram + EXT_RAM)[addr];
    return last_access;
}

static void store_super_flat(uint16_t addr, uint8_t value)
{
    last_access = value;
    (mem_ram + EXT_RAM)[addr] = value;
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

#define PRINT_6809_STORE        0
#define PRINT_6809_READ         0

void mem6809_store(uint16_t addr, uint8_t value)
{
#if PRINT_6809_STORE
    if (addr >= 0x8000 && addr < 0x9000) {
        printf("mem6809_store   %04x <- %02x\n", addr, value);
    }
#endif
    _mem6809_write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem6809_read(uint16_t addr)
{
#if PRINT_6809_READ
    uint8_t v;
    v = _mem6809_read_tab_ptr[addr >> 8](addr);
    printf("mem6809_read   %04x -> %02x\n", addr, v);
    return v;
#else
    return _mem6809_read_tab_ptr[addr >> 8](addr);
#endif
}

void mem6809_store16(uint16_t addr, uint16_t value)
{
#if PRINT_6809_STORE0
    printf("mem6809_store16 %04x <- %04x\n", addr, value);
#endif
    addr++;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)(value & 0xFF));
    addr--;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)(value >> 8));
}

uint16_t mem6809_read16(uint16_t addr)
{
    uint16_t val;
    val = _mem6809_read_tab_ptr[addr >> 8](addr) << 8;
    addr++;
    val |= _mem6809_read_tab_ptr[addr >> 8](addr);
#if PRINT_6809_READ
    printf("mem6809_read16 %04x -> %04x\n", addr, val);
#endif
    return val;
}

#ifdef H6309
void mem6809_store32(uint16_t addr, uint32_t value)
{
#if PRINT_6809_STORE0
    printf("mem6809_store32 %04x <- %04x\n", addr, value);
#endif
    addr += 3;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)(value & 0xFF));
    addr--;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)((value >> 8) & 0xFF));
    addr--;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)((value >> 16) & 0xFF));
    addr--;
    _mem6809_write_tab_ptr[addr >> 8](addr, (uint8_t)(value >> 24));
}

uint32_t mem6809_read32(uint16_t addr)
{
    uint32_t val;
    val = _mem6809_read_tab_ptr[addr >> 8](addr) << 24;
    addr++;
    val |= _mem6809_read_tab_ptr[addr >> 8](addr) << 16;
    addr++;
    val |= _mem6809_read_tab_ptr[addr >> 8](addr) << 8;
    addr++;
    val |= _mem6809_read_tab_ptr[addr >> 8](addr);
#if PRINT_6809_READ
    printf("mem6809_read32 %04x -> %04x\n", addr, val);
#endif
    return val;
}
#endif

/* ------------------------------------------------------------------------- */

/* The PET have all I/O chips connected to the same select lines.  Only one
   address lines is used as separate (high-active) select input.  I.e. PIA1
   always reacts when address & 0x10 is high, for example $e810, $e830,
   $e850, etc.  PIA2 reacts when address & 0x20 is high, for example $e820,
   $e830 $e860,...  The next two functions try to reflect this behaviour.  */

/* When we write, we write all involved chips.  */

static void store_io_e8(uint16_t addr, uint8_t value)
{
    last_access = value;

    if (addr & 0x10) {
        pia1_store(addr, value);
    }

    if (addr & 0x20) {
        pia2_store(addr, value);
    }

    if (addr & 0x40) {
        via_store(addr, value);
    }

    if ((addr & 0x80) && petres.crtc) {
        crtc_store(addr, value);
        crtc_store_hre(addr, value);
    }
}


/*
 * When we read, we only read sensible values. In real operation,
 * the bus drivers of all involved chips interact and you get strange
 * results...
 */
static uint8_t read_io_e8(uint16_t addr)
{
    uint8_t v1, v2, v3, v4;

    switch (addr & 0xf0) {
        case 0x10:              /* PIA1 */
            last_access = pia1_read(addr);
            break;
        case 0x20:              /* PIA2 */
            last_access = pia2_read(addr);
            break;
        case 0x40:
            last_access = via_read(addr); /* VIA */
            break;
        case 0x80:              /* CRTC */
            if (petres.crtc) {
                last_access = crtc_read(addr);
            } /* fall through */
        case 0x00:
            return last_access;
        default:                /* 0x30, 0x50, 0x60, 0x70, 0x90-0xf0 */
            if (addr & 0x10) {
                v1 = pia1_read(addr);
            } else {
                v1 = 0xff;
            }
            if (addr & 0x20) {
                v2 = pia2_read(addr);
            } else {
                v2 = 0xff;
            }
            if (addr & 0x40) {
                v3 = via_read(addr);
            } else {
                v3 = 0xff;
            }
            v4 = 0xff;
            if ((addr & 0x80) && petres.crtc) {
                v4 = crtc_read(addr);
            }
            last_access = v1 & v2 & v3 & v4;
    }
    return last_access;
}

static void store_void(uint16_t addr, uint8_t value)
{
    last_access = value;
}

/*
 * store_dummy() stores in normally-unassigned memory space,
 * except that some hardware expansions may be there.
 * For adresses < 0x8000 it is faster to use store_void().
 */
static void store_dummy(uint16_t addr, uint8_t value)
{
    last_access = value;
}

static void store_io_88_8f(uint16_t addr, uint8_t value)
{
    last_access = value;

    switch (addr & 0xff00) {
        case 0x8800:
            petio_8800_store(addr, value);
            break;
        case 0x8900:
            petio_8900_store(addr, value);
            break;
        case 0x8a00:
            petio_8a00_store(addr, value);
            break;
        case 0x8b00:
            petio_8b00_store(addr, value);
            break;
        case 0x8c00:
            petio_8c00_store(addr, value);
            break;
        case 0x8d00:
            petio_8d00_store(addr, value);
            break;
        case 0x8e00:
            petio_8e00_store(addr, value);
            break;
        case 0x8f00:
            petio_8f00_store(addr, value);
            break;
    }
}

static void store_io_e9_ef(uint16_t addr, uint8_t value)
{
    last_access = value;

    switch (addr & 0xff00) {
        case 0xe900:
            petio_e900_store(addr, value);
            break;
        case 0xea00:
            petio_ea00_store(addr, value);
            break;
        case 0xeb00:
            petio_eb00_store(addr, value);
            break;
        case 0xec00:
            petio_ec00_store(addr, value);
            break;
        case 0xed00:
            petio_ed00_store(addr, value);
            break;
        case 0xee00:
            petio_ee00_store(addr, value);
            break;
        case 0xef00:
            petio_ef00_store(addr, value);
            break;
    }
}

/*
 * This sets the standard PET memory configuration from $9000-$10000.
 * It is used in store_8x96() and mem_initialize_memory().
 *
[/NO ROM = 1; see page 12 for more cases]
+-----+--------+------------+-------------------------------------------------+
|     |Control |            |                                                 |
|     |Register|            |      main memory $8000 - $FFFF        [UB1-UB8] |
|     |        |            |      [$0000 - $7FFF is always RAM from UB1-UB8] |
|     |$FFF0   |            |                                                 |
+-----+--------+------------+-------------------------------------------------+
|     |    I/O |___ ___ ___ |              E000                               |
| ___ |    peek|RAM RAM RAM | E800  F000  -E7FF  B000  A000  9000  8000       |
| NO_ |    thr.|___ ___ ___ |              E900                               |
| ROM |CR7 CR6 |ON  S.9 S.A |-E8FF -FFFF  -EFFF -DFFF -AFFF -9FFF -8FFF       |
+-----+--------+------------+-------------------------------------------------+
|     |        |            |                                                 |
|  1  | 0   X  | 1   1   1  | I/O  Kernal Editor BASIC EPROM EPROM SCREEN     |
|     |        |            |                                                 |
|  1  | 0   X  | 1   1   0  | I/O  Kernal Editor BASIC  RAM  EPROM SCREEN     |
|     |        |            |                               +-----+           |
|  1  | 0   X  | 1   0   1  | I/O  Kernal Editor BASIC EPROM  RAM  SCREEN     |
|     |        |            |                         +-----+                 |
|  1  | 0   X  | 1   0   0  | I/O  Kernal Editor BASIC  RAM   RAM  SCREEN     |
|     |        |            |                   +-----+                       |
|  1  | 0   X  | 0   1   1  | I/O  Kernal Editor  RAM   RAM   RAM  SCREEN     |
|     |        |            |            +------+                             |
|  1  | 0   X  | 0   0   1  | I/O  Kernal  RAM    RAM   RAM   RAM  SCREEN     |
|     |        |            |     +------+                                    |
|  1  | 0   1  | 0   X   0  | I/O   RAM    RAM    RAM   RAM   RAM  SCREEN     |
|     |        |            |+----+                                           |
|  1  | 0   0  | 0   X   0  | RAM   RAM    RAM    RAM   RAM   RAM  SCREEN     |
|     |        |            |                                                 |
+-----+--------+------------+-------------------------------------------------+
SCREEN: 2000 bytes for screen memory, and 2096 bytes of available RAM.
 *
 */

static void set_std_9tof(void)
{
    int i, l;
    void (*store)(uint16_t, uint8_t);
    uint8_t (*fetch)(uint16_t);
    int ram9, ramA, ramBCD, ramE, ramE8, ramF;

    /* printf("set_std_9tof: petres.ramSize=%d, petres.map=%d\n", petres.ramSize, petres.map); */
    if (petres.map == PET_MAP_8296) {
        store = ram_store;

        if (petmem_ramON) {
            ram9 = ramA = ramBCD = 1;
            ramE = petres.ramsel9 || petres.ramselA;
            ramE8 = petres.ramselA && !(petmem_map_reg & FFF0_IO_PEEK_THROUGH);
            ramF = petres.ramselA;
            /*
             * XXX: If there is no I/O peek through, how can we write
             * again to the E888 register to restore I/O?
             */
        } else {
            ram9 = petres.ramsel9;
            ramA = petres.ramselA;
            ramBCD = ramE = ramE8 = ramF = 0;
        }
    } else {
        store = store_dummy;
        ram9 = ramA = ramBCD = ramE = ramE8 = ramF = 0;
    }

    /* Setup RAM/ROM at $9000 - $9FFF. */
    if (petres.superpet) {
        for (i = 0x90; i < 0xa0; i++) {
            _mem_read_tab[i] = read_super_9;
            _mem_write_tab[i] = store_super_9;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    } else {
        fetch = ram9 ? ram_read : rom_read;
        for (i = 0x90; i < 0xa0; i++) {
            _mem_read_tab[i] = fetch;
            _mem_write_tab[i] = store;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    }

    /* Possibly set up the Double-W HiRes board at $9000 - $9FFF. */
    if (petdww_enabled && petdww_mem_at_9000()) {
        petdww_override_std_9toa(_mem_read_tab, _mem_write_tab,
                                 _mem_read_base_tab, mem_read_limit_tab);
    }

    /* Setup RAM/ROM at $A000 - $AFFF. */
    fetch = ramA ? ram_read : rom_read;
    for (i = 0xa0; i < 0xb0; i++) {
        _mem_read_tab[i] = fetch;
        _mem_write_tab[i] = store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /* Setup RAM/ROM at $B000 - $DFFF: Basic. */
    fetch = ramBCD ? ram_read : rom_read;
    for (i = 0xb0; i <= 0xdf; i++) {
        _mem_read_tab[i] = fetch;
        _mem_write_tab[i] = store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /* Setup RAM/ROM at $E000 - $E7FF: Editor. */
    fetch = ramE ? ram_read : rom_read;
    for (i = 0xe0; i <= 0xe7; i++) {
        _mem_read_tab[i] = fetch;
        _mem_write_tab[i] = store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /* End of I/O address space */
    l = ((0xe800 + petres.IOSize) >> 8) & 0xff;

    if (ramE8) {
        /* Setup RAM at $E800 - $E800 + petres.IOSize. */
        for (i = 0xe0; i < l; i++) {
            _mem_read_tab[i] = ram_read;
            _mem_write_tab[i] = store;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    } else {
        /* Setup I/O at $e800 - $e800 + petres.IOSize. */
        /* i.e. IO at $e800... */
        _mem_read_tab[0xe8] = read_io_e8;
        _mem_write_tab[0xe8] = store_io_e8;
        _mem_read_base_tab[0xe8] = NULL;
        mem_read_limit_tab[0xe8] = 0;

        /* ... and unused address space following it, if any. */
        for (i = 0xe9; i < l; i++) {
            _mem_read_tab[i] = read_io_e9_ef;
            _mem_write_tab[i] = store_io_e9_ef;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    }

    /* Setup RAM/ROM at $E800 + petres.IOSize - $EFFF: Extended Editor */
    fetch = ramE ? ram_read : rom_read;
    for (i = l; i <= 0xef; i++) {
        _mem_read_tab[i] = fetch;
        _mem_write_tab[i] = store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /*
     * $EF00 is needed for SuperPET I/O or 2001 ROM patch.
     * This means that those models can't support an extended editor ROM.
     */
    if (petres.superpet) {
        _mem_read_tab[0xef] = read_super_io;
        _mem_write_tab[0xef] = store_super_io;
        _mem_read_base_tab[0xef] = NULL;
        mem_read_limit_tab[0xef] = 0;
    } else if (petres.rompatch) {
        _mem_read_tab[0xef] = mem_read_patchbuf;
        _mem_write_tab[0xef] = store_void;
        _mem_read_base_tab[0xef] = NULL;
        mem_read_limit_tab[0xef] = 0;
    }

    /* Setup RAM/ROM at $f000 - $ffff: Kernal */
    fetch = ramF ? ram_read : rom_read;
    for (i = 0xf0; i <= 0xff; i++) {
        _mem_read_tab[i] = fetch;
        _mem_write_tab[i] = store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    _mem_read_base_tab_ptr = _mem_read_base_tab;
    mem_read_limit_tab_ptr = mem_read_limit_tab;
}

void ramsel_changed(void)
{
    set_std_9tof();
    maincpu_resync_limits();
}

void get_mem_access_tables(read_func_ptr_t **read, store_func_ptr_t **write, uint8_t ***base, int **limit)
{
    *read = _mem_read_tab;
    *write = _mem_write_tab;
    *base = _mem_read_base_tab;
    *limit = mem_read_limit_tab;
}

/* called by mem_update_config(), mem_toggle_watchpoints() */
static void mem_update_tab_ptrs(int flag)
{
    if (flag) {
        _mem_read_tab_ptr = _mem_read_tab_watch;
        _mem_write_tab_ptr = _mem_write_tab_watch;
        if (flag > 1) {
            /* enable watchpoints on dummy accesses */
            _mem_read_tab_ptr_dummy = _mem_read_tab_watch;
            _mem_write_tab_ptr_dummy = _mem_write_tab_watch;
        } else {
            _mem_read_tab_ptr_dummy = _mem_read_tab;
            _mem_write_tab_ptr_dummy = _mem_write_tab;
        }
    } else {
        /* all watchpoints disabled */
        _mem_read_tab_ptr = _mem_read_tab;
        _mem_write_tab_ptr = _mem_write_tab;
        _mem_read_tab_ptr_dummy = _mem_read_tab;
        _mem_write_tab_ptr_dummy = _mem_write_tab;
    }
}

void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem6809_read_tab_ptr = _mem6809_read_tab_watch;
        _mem6809_write_tab_ptr = _mem6809_write_tab_watch;
    } else {
        _mem6809_read_tab_ptr = _mem6809_read_tab;
        _mem6809_write_tab_ptr = _mem6809_write_tab;
    }

    mem_update_tab_ptrs(flag);
    watchpoints_active = flag;
}

/*
 * From PETdoc.txt:
 *
 * $fff0 register in PET 8x96
 * 8096 exp-mem (64K):
 * The control register is at $FFF0/65520
 * You have 4 16K-banks, 0...3
 *
 * $8000     $9000               $C000           $E800    $F000     $FFFF
 * !----------------------------!!--------------------------------------!
 *         Bank 0 or 2                     Bank 1 or 3
 * !--------!                                    !-------!
 *   screen                                        io
 *
 * Control Register $FFF0:
 * bit 7:  0 normal 8032 configuration (screen, ROMs, IO, ROMs)
 *        80 expansion memory
 * bit 6:  0 RAM $E800-$EFFF (only when bit7=1)
 *        40 IO peek through
 * bit 5:  0 exp-mem $8000-$8FFF (-"-)
 *        20 screen peek through
 * bit 4: 10 not used
 * bit 3:  0 bank 1 $C000-$FFFF
 *        08 bank 3
 * bit 2:  0 bank 0 $8000-$BFFF
 *        04 bank 2
 * bit 1: 02 write protect bank 1/3
 * bit 0: 01 write protect bank 0/2
 * when bit7=0, all other bits are ignored
 *
 * The missing 32K can't be accessed witout hardware modifications.
 * You can only use the 2K "behind" the screen $8800-$8FFF (exact: 34768-
 * 36863), available in the normal configuration.
 * The register is write-only, and the value is written through to the
 * previously selected ram bank.
 *
 */

#define FFF0_BANK_C      0x08
#define FFF0_BANK_8      0x04
#define FFF0_BANK_C_WP   0x02
#define FFF0_BANK_8_WP   0x01

/* Save old store function for last byte.  */
static void (*store_ff)(uint16_t addr, uint8_t value) = NULL;

/* Write to last page of memory in 8x96.  */
static void store_8x96(uint16_t addr, uint8_t value)
{
    uint8_t changed;
    int l, protected;

    last_access = value;

    if (store_ff) {
        store_ff(addr, value);
    }

    changed = petmem_map_reg ^ value;

    if (addr == 0xfff0 && changed &&
        ((petmem_map_reg | changed) & FFF0_ENABLED)) {
        if (value & FFF0_ENABLED) {     /* ext. RAM enabled */
            /* A5 = FFF0_ENABLED | FFF0_SCREEN_PEEK_THROUGH |
             *      FFF0_BANK_8 | FFF0_BANK_8_WP
             */
            if (changed & 0xa5) {       /* $8000-$bfff */
                protected = value & FFF0_BANK_8_WP;
                l = 0x80;
                if (value & FFF0_SCREEN_PEEK_THROUGH) {
                    /* screen memory mapped through */
                    for (; l < 0x90; l++) {
                        _mem_read_tab[l] = ram_read;
                        _mem_write_tab[l] = ram_store;
                        _mem_read_base_tab[l] = NULL;
                        mem_read_limit_tab[l] = 0;
                    }
                }
                bank8offset = 0x8000 + ((value & FFF0_BANK_8) ? 0x8000 : 0);
                for (; l < 0xc0; l++) {
                    _mem_read_tab[l] = read_ext8;
                    if (protected) {
                        _mem_write_tab[l] = store_dummy;
                    } else {
                        _mem_write_tab[l] = store_ext8;
                    }
                    _mem_read_base_tab[l] = NULL;
                    mem_read_limit_tab[l] = 0;
                }
                maincpu_resync_limits();
            }
            /* CA = FFF0_ENABLED | FFF0_IO_PEEK_THROUGH |
             *       FFF0_BANK_C | FFF0_BANK_C_WP
             */
            if (changed & 0xca) {       /* $c000-$ffff */
                protected = value & FFF0_BANK_C_WP;
                bankCoffset = 0x8000 + ((value & FFF0_BANK_C) ? 0x8000 : 0);
                for (l = 0xc0; l < 0x100; l++) {
                    if ((l == 0xe8) && (value & FFF0_IO_PEEK_THROUGH)) {
                        _mem_read_tab[l] = read_io_e8;
                        _mem_write_tab[l] = store_io_e8;
                        _mem_read_base_tab[l] = NULL;
                        mem_read_limit_tab[l] = 0;
                    } else {
                        _mem_read_tab[l] = read_extC;
                        if (protected) {
                            _mem_write_tab[l] = store_dummy;
                        } else {
                            _mem_write_tab[l] = store_extC;
                        }
                        _mem_read_base_tab[l] = NULL;
                        mem_read_limit_tab[l] = 0;
                    }
                }
                store_ff = _mem_write_tab[0xff];
                _mem_write_tab[0xff] = store_8x96;
                maincpu_resync_limits();
            }
        } else {                /* disable ext. RAM */
            petmem_set_vidmem();
            set_std_9tof();
            store_ff = _mem_write_tab[0xff];
            _mem_write_tab[0xff] = store_8x96;
            maincpu_resync_limits();
        }
        petmem_map_reg = value;
    }
}

static int fff0_dump(void)
{
    mon_out("%s memory mapping.\n",
            (petres.map == PET_MAP_8296 ? "8296" :
             petres.map == PET_MAP_8096 ? "8096" :
             petres.map == PET_MAP_LINEAR ? "plain" :
             "unknown"));
    mon_out("fff0 = %02x: ", petmem_map_reg);
    if (petmem_map_reg & FFF0_ENABLED) {
        mon_out("enabled, ");
        if (petmem_map_reg & FFF0_IO_PEEK_THROUGH) {
            mon_out("I/O peek through, ");
        }
        if (petmem_map_reg & FFF0_SCREEN_PEEK_THROUGH) {
            mon_out("screen peek through, ");
        }
        if (petmem_map_reg & 0x10) {
            mon_out("$10 unused bit set, ");
        }
        mon_out("\nC000-FFFF: bank %d %s, ",
                ((petmem_map_reg & FFF0_BANK_C) ? 3 : 1),
                ((petmem_map_reg & FFF0_BANK_C_WP) ? "(write protected)" : "(r/w)")
                );
        mon_out("8000-BFFF: bank %d %s.\n",
                ((petmem_map_reg & FFF0_BANK_8) ? 2 : 0),
                ((petmem_map_reg & FFF0_BANK_8_WP) ? "(write protected)" : "(r/w)")
                );
    } else {
        mon_out("disabled.\n");
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

void petmem_set_vidmem(void)
{
    int i, l;

    l = ((0x8000 + petres.videoSize) >> 8) & 0xff;
/*
    log_message(pet_mem_log, "petmem_set_vidmem(videoSize=%04x, l=%d)",
                petres.videoSize,l);
*/
    /* Setup RAM from $8000 to $8000 + petres.videoSize ($8400 or $8800) */
    for (i = 0x80; i < l; i++) {
        _mem_read_tab[i] = ram_read;
        _mem_write_tab[i] = ram_store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /* Setup video mirror from $8000 + petres.videoSize to $87ff */
    /* falls through if videoSize >= 0x800 */
    for (; i < 0x88; i++) {
        _mem_read_tab[i] = read_vmirror;
        _mem_write_tab[i] = store_vmirror;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /*
     * Model 2001 has its 1K screen memory mirrored all over $8xxx,
     * unlike later models.
     */
    if (petres.screenmirrors2001) {
        /* Setup screen mirror 3 and 4 from $8800 to $8FFF */
        for (; i < 0x90; i++) {
            _mem_read_tab[i] = read_vmirror_2001;
            _mem_write_tab[i] = store_vmirror_2001;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    } else if (pet_colour_type == PET_COLOUR_TYPE_OFF) {
        /* Setup unused from $8800 to $8FFF */
        /* falls through if videoSize >= 0x1000 */
        for (; i < 0x90; i++) {
            _mem_read_tab[i] = read_io_88_8f;
            _mem_write_tab[i] = store_io_88_8f;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    } else {
        /* Setup colour RAM from $8800 to $8BFF or $8FFF */
        int c = 0x8000 + COLOUR_MEMORY_START;
        i = (c >> 8) & 0xff;
        l = ((c + petres.videoSize) >> 8) & 0xff;
        if (l > 0x90) { /* compatibility with 8296 */
            l = 0x90;
        }

        for (; i < l; i++) {
            _mem_read_tab[i] = ram_read;
            _mem_write_tab[i] = ram_store;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }

        /* Setup colour mirror from $8800 + petres.videoSize to $8FFF */
        /* falls through if videoSize >= 0x800 */
        for (; i < 0x90; i++) {
            _mem_read_tab[i] = read_vmirror;
            _mem_write_tab[i] = store_vmirror;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    }
}

/* ------------------------------------------------------------------------- */

static void mem_initialize_memory_6809_banked(void)
{
    int i;

    /* extern WORD iPC; printf("mem_initialize_memory_6809_banked %04x bank %x\n", iPC, spet_bank); */
    for (i = 0x00; i < 0xa0; i++) {
        _mem6809_read_tab[i] = _mem_read_tab[i];
        _mem6809_write_tab[i] = _mem_write_tab[i];
        _mem6809_read_base_tab[i] = _mem_read_base_tab[i];
        mem6809_read_limit_tab[i] = mem_read_limit_tab[i];
    }
    /*
     * Set up the ROMs.
     */
    for (i = 0xa0; i < 0xe8; i++) {
        _mem6809_read_tab[i] = rom6809_read;
        _mem6809_write_tab[i] = store_void;
        _mem6809_read_base_tab[i] = mem_6809rom + i - (ROM6809_BASE >> 8);
        mem6809_read_limit_tab[i] = 0xe7fc;
    }
    for (i = 0xf0; i < 0x100; i++) {
        _mem6809_read_tab[i] = rom6809_read;
        _mem6809_write_tab[i] = store_void;
        _mem6809_read_base_tab[i] = mem_6809rom + i - (ROM6809_BASE >> 8);
        mem6809_read_limit_tab[i] = 0xfffc;
    }
    /*
     * Also copy the I/O setup from the 6502 view.
     */
    for (i = 0xe8; i < 0xf0; i++) {
        _mem6809_read_tab[i] = _mem_read_tab[i];
        _mem6809_write_tab[i] = _mem_write_tab[i];
        _mem6809_read_base_tab[i] = _mem_read_base_tab[i];
        mem6809_read_limit_tab[i] = mem_read_limit_tab[i];
    }

    _mem6809_read_tab[0x100] = _mem6809_read_tab[0];
    _mem6809_write_tab[0x100] = _mem6809_write_tab[0];
    _mem6809_read_base_tab[0x100] = _mem6809_read_base_tab[0];
    mem6809_read_limit_tab[0x100] = -1;

    /* maincpu_resync_limits(); notyet: 6809 doesn't use bank_base yet. */
}

static void mem_initialize_memory_6809_flat(void)
{
    int i;

    /* extern WORD iPC; printf("mem_initialize_memory_6809_flat   %04X bank %x\n", iPC, spet_bank); */

    for (i = 0x00; i < 0x101; i++) {
        _mem6809_read_tab[i] = read_super_flat;
        _mem6809_write_tab[i] = store_super_flat;
        _mem6809_read_base_tab[i] = mem_ram + EXT_RAM + (i << 8);
        mem6809_read_limit_tab[i] = 0xfffc;
    }

    _mem6809_read_base_tab[0x100] = _mem6809_read_base_tab[0];
    mem6809_read_limit_tab[0x100] = -1;
    /* maincpu_resync_limits(); notyet: 6809 doesn't use bank_base yet. */
}

void mem_initialize_memory_6809(void)
{
    if (spet_flat_mode) {
        mem_initialize_memory_6809_flat();
    } else {
        mem_initialize_memory_6809_banked();
    }
}

int superpet_sync(void)
{
    if (spet_firq_disabled) {
        log_error(pet_mem_log, "SuperPET: SYNC encountered, but no FIRQ possible!");
        return 1;
    } else {
        spet_flat_mode = 0;
        mem_initialize_memory_6809_banked();
        /* mon_bank(e_default_space, "6809");
           extern WORD PC;
           printf("next opcode: %04X: banked %02X, flat %02X\n",
                   PC,
                   mem_ram[EXT_RAM + spet_bank_4k + (PC & 0x0FFF)],
                   mem_ram[EXT_RAM + PC]
              ); */

        return 0;
    }
}

/* This does the plain 8032 configuration, as 8096 stuff only comes up when
   writing to $fff0.  */
void mem_initialize_memory(void)
{
    int i, l;

    l = petres.ramSize << 2;       /* ramSize in kB, l in 256 Byte */
    if (l > (32 << 2)) {
        l = (32 << 2);             /* fix 8096 / 8296 */
    }
    /* Setup RAM from $0000 to petres.ramSize */
    _mem_read_tab[0] = zero_read;
    _mem_write_tab[0] = zero_store;
    _mem_read_base_tab[0] = NULL;
    mem_read_limit_tab[0] = 0;

    for (i = 0x01; i < l; i++) {
        _mem_read_tab[i] = ram_read;
        _mem_write_tab[i] = ram_store;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    /* Setup unused from petres.ramSize to $7fff */
    for (i = l; i < 0x80; i++) {
        _mem_read_tab[i] = read_unused;
        _mem_write_tab[i] = store_void;
        _mem_read_base_tab[i] = NULL;
        mem_read_limit_tab[i] = 0;
    }

    petmem_set_vidmem();

    set_std_9tof();

    if (petres.map) {              /* catch writes to $fff0 register */
        store_ff = _mem_write_tab[0xff];
        _mem_write_tab[0xff] = store_8x96;
    }
    _mem_read_tab[0x100] = _mem_read_tab[0];
    _mem_write_tab[0x100] = _mem_write_tab[0];
    _mem_read_base_tab[0x100] = _mem_read_base_tab[0];
    mem_read_limit_tab[0x100] = 0;

    ram_size = petres.ramSize * 1024;
    _mem_read_tab_ptr = _mem_read_tab;
    _mem_write_tab_ptr = _mem_write_tab;
    _mem_read_tab_ptr_dummy = _mem_read_tab;
    _mem_write_tab_ptr_dummy = _mem_write_tab;

    /* setup watchpoint tables */
    _mem_read_tab_watch[0] = zero_read_watch;
    _mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i < 0x101; i++) {
        _mem_read_tab_watch[i] = read_watch;
        _mem_write_tab_watch[i] = store_watch;
    }

    if (petres.map && petmem_map_reg) {
        uint8_t old_map_reg;

        old_map_reg = petmem_map_reg;
        petmem_map_reg = 0;
        store_8x96(0xfff0, old_map_reg);
    } else {
        petmem_map_reg = 0;
    }


    if (petres.superpet) {
        /*
         * Initialize SuperPET 6809 memory view.
         * Basically, it is the same as the 6502 view, except for the
         * ROMs in addresses $A000 - $FFFF and but including the I/O range
         * of $E800 - $EFFF.
         */
        mem_initialize_memory_6809();

        for (i = 0; i < 0x101; i++) {
            _mem6809_read_tab_watch[i] = read6809_watch;
            _mem6809_write_tab_watch[i] = store6809_watch;
        }

        _mem6809_read_tab_ptr = _mem6809_read_tab;
        _mem6809_write_tab_ptr = _mem6809_write_tab;
    }

    maincpu_resync_limits();
}

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : (p - (addr & 0xff00));
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

void mem_powerup(void)
{
    int i;
    ram_init(mem_ram, RAM_ARRAY);
    /*
     * Initial screen memory is random, which is physically different memory
     * (and also faster) in most models.
     * (And it helps a bit with the colour option when a proper editor
     * ROM isn't installed)
     */
    for (i = 0; i < 0x1000; i++) {
        mem_ram[0x8000 + i] = (uint8_t)lib_unsigned_rand(0, 255);
    }

    superpet_mem_powerup();
}

/* ------------------------------------------------------------------------- */

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    int basicstart;

    /* FIXME: should really check a basic_checksum */
    if (petres.kernal_checksum == PET_KERNAL1_CHECKSUM) {
        basicstart = 0x7a;
    } else {
        basicstart = 0x28;
    }

    if (start != NULL) {
        *start = mem_ram[basicstart] | (mem_ram[basicstart + 1] << 8);
    }
    if (end != NULL) {
        *end = mem_ram[basicstart + 2] | (mem_ram[basicstart + 3] << 8);
    }
}

void mem_set_basic_text(uint16_t start, uint16_t end)
{
    int basicstart, loadadr;

    if (petres.kernal_checksum == PET_KERNAL1_CHECKSUM) {
        basicstart = 0x7a;    /* Pointers to Basic program + variables */
        loadadr = 0xe3;       /* Utility pointers for start and end of load */
    } else {
        basicstart = 0x28;
        loadadr = 0xc7;
    }

    mem_ram[basicstart] = mem_ram[loadadr] = start & 0xff;
    mem_ram[basicstart + 1] = mem_ram[loadadr + 1] = start >> 8;

    mem_ram[basicstart + 2] =
        mem_ram[basicstart + 4] =
            mem_ram[basicstart + 6] = mem_ram[loadadr + 2] = end & 0xff;
    mem_ram[basicstart + 3] =
        mem_ram[basicstart + 5] =
            mem_ram[basicstart + 7] = mem_ram[loadadr + 3] = end >> 8;
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
    /* just call mem_store() to be safe.
       This could possibly be changed to write straight into the
       memory array.  mem_ram[addr & mask] = value; */
    mem_store((uint16_t)(addr & 0xffff), value);
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
    return (addr >= 0xf000) && !(petmem_map_reg & 0x80);
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

static uint8_t peek_bank_io(uint16_t addr)
{
    uint8_t v1, v2, v3, v4;

    switch (addr & 0xf0) {
        case 0x10:              /* PIA1 */
            return pia1_peek(addr);
        case 0x20:              /* PIA2 */
            return pia2_peek(addr);
        case 0x40:
            return via_peek(addr); /* VIA */
        case 0x80:              /* CRTC */
            if (petres.crtc) {
                return crtc_read(addr);
            }
            /* FALL THROUGH */
        case 0x00:
            return addr >> 8;
        default:                /* 0x30, 0x50, 0x60, 0x70, 0x90-0xf0 */
            break;
    }

    if (addr & 0x10) {
        v1 = pia1_peek(addr);
    } else {
        v1 = 0xff;
    }
    if (addr & 0x20) {
        v2 = pia2_peek(addr);
    } else {
        v2 = 0xff;
    }
    if (addr & 0x40) {
        v3 = via_peek(addr);
    } else {
        v3 = 0xff;
    }
    v4 = 0xff;
    if ((addr & 0x80) && petres.crtc) {
        v4 = crtc_read(addr);
    }
    return v1 & v2 & v3 & v4;
}

/* Exported banked memory access functions for the monitor.  */
#define MAXBANKS (7)

static const char *banknames[MAXBANKS + 1] = {
    "default",
    "cpu",
    "ram",
    "rom",
    "io",
    "extram",
    "6809",
    /* by convention, a "bank array" has a 2-hex-digit bank index appended */
    NULL
};

enum {
    bank_cpu = 0,
    bank_ram,
    bank_rom,
    bank_io,
    bank_extram,
    bank_cpu6809
};

static const int banknums[MAXBANKS + 1] = {
    bank_cpu, /* default */
    bank_cpu,
    bank_ram,
    bank_rom,
    bank_io,
    bank_extram,
    bank_cpu6809, -1
};

static const int bankindex[MAXBANKS + 1] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static const int bankflags[MAXBANKS + 1] = { 0, 0, 0, 0, 0, 0, 0, -1 };

const char **mem_bank_list(void)
{
    return banknames;
}

const int *mem_bank_list_nos(void) {
    return banknums;
}

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

uint8_t mem_bank_read(int bank, uint16_t addr, void *context)
{
    switch (bank) {
        case bank_cpu:      /* current */
            return mem_read(addr);
            break;
        case bank_extram:       /* extended RAM area (8x96, SuperPET) */
            return mem_ram[addr + EXT_RAM];
            break;
        case bank_io:          /* io */
            if (addr >= 0xe800 && addr < 0xe900) {
                return read_io_e8(addr);
            }
            if (petres.superpet && (addr & 0xff00) == 0xef00) {
                return read_super_io(addr);
            }
            if (addr >= 0xe900 && addr < 0xe800 + petres.IOSize) {
                return read_io_e9_ef(addr);
            }
            /* FALL THROUGH */
        case bank_rom:         /* rom */
            if (addr >= 0x9000) {
                return mem_rom[addr & 0x7fff];
            }
            break;
        case bank_cpu6809:     /* 6809 */
            return mem6809_read(addr);
        case bank_ram:         /* ram */
            break;
    }
    return mem_ram[addr];
}

/* used by monitor if sfx off */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    switch (bank) {
        case bank_cpu:      /* current */
            return mem_read(addr); /* FIXME */
            break;
        case bank_io:           /* io */
            if (addr >= 0xe800 && addr < 0xe900) {
                return peek_bank_io(addr);
            }
            if (petres.superpet && (addr & 0xff00) == 0xef00) {
                return read_super_io(addr);
            }
            if (addr >= 0xe900 && addr < 0xe800 + petres.IOSize) {
                uint8_t result;
                /* is_peek_access = 1; FIXME */
                result = read_unused(addr);
                /* is_peek_access = 0; FIXME */
                return result;
            }
    }
    return mem_bank_read(bank, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    switch (bank) {
        case bank_cpu:      /* current */
            mem_store(addr, byte);
            return;
        case bank_extram:       /* extended RAM area (8x96, SuperPET) */
            mem_ram[addr + EXT_RAM] = byte;
            return;
        case bank_io:           /* io */
            if (addr >= 0xe800 && addr < 0xe900) {
                store_io_e8(addr, byte);
                return;
            }
            if (petres.superpet && (addr & 0xff00) == 0xef00) {
                store_super_io(addr, byte);
                return;
            }
            if (addr >= 0xe900 && addr < 0xe800 + petres.IOSize) {
                store_io_e9_ef(addr, byte);
                return;
            }
            /* FALL THROUGH */
        case bank_rom:          /* rom */
            if (addr >= 0x9000) {
                return;
            }
            break;
        case bank_cpu6809:      /* rom */
            mem6809_store(addr, byte);
            return;
        case bank_ram:                 /* ram */
            break;
    }
    mem_ram[addr] = byte;
}

/* used by monitor if sfx off */
void mem_bank_poke(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_bank_write(bank, addr, byte, context);
}


static int mem_dump_io(void *context, uint16_t addr)
{
    if ((addr >= 0xe810) && (addr <= 0xe81f)) {
        return pia1_dump();
    } else if ((addr >= 0xe820) && (addr <= 0xe82f)) {
        return pia2_dump();
    } else if ((addr >= 0xe840) && (addr <= 0xe84f)) {
        return viacore_dump(machine_context.via);
    } else if ((addr >= 0xe880) && (addr <= 0xe881)) {
        if (petres.crtc) {
            return crtc_dump();
        }
    } else if (addr == 0xe888) {
        return e888_dump();
    } else if ((addr >= 0xeb00) && (addr <= 0xeb0f)) {
        if (petdww_enabled) {
            return petdwwpia_dump();
        }
    } else if (addr == 0xfff0) {
        if (petres.map) {
            return fff0_dump();
        }
    }
    if (petres.superpet) {
        if (addr >= 0xefe0 && addr <= 0xefe3) {
            return efe0_dump();
        } else if (addr >= 0xeff0 && addr <= 0xeff3) {
            /* ACIA */
            return acia1_dump();
        } else if (addr == 0xeff8) {
            /* Control switch */
            mon_out("CPU: %s\n",
                    petres.superpet_cpu_switch == SUPERPET_CPU_6502 ? "6502" :
                    petres.superpet_cpu_switch == SUPERPET_CPU_6809 ? "6809" :
                    "PROG (unimpl)");
            mon_out("RAM write protect: $%x\n", (unsigned int)spet_ramwp);
            mon_out("diagnostic sense: $%x\n", (unsigned int)spet_diag);
            return 0;
        } else if (addr == 0xeffc) {
            /* Bank select */
            mon_out("bank: $%x\n", (unsigned int)spet_bank);
            mon_out("control write protect: %d\n", spet_ctrlwp);
            mon_out("flat (super-os9) mode: %d\n", !!spet_flat_mode);
            mon_out("firq disabled: %d\n", !!spet_firq_disabled);
            return 0;
        } else if (addr == 0xeffe) {
            /* RAM/ROM switch */
            mon_out("ram_enable: %d\n", spet_ramen);
            return 0;
        }
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    io_source_ioreg_add_list(&mem_ioreg_list);
    mon_ioreg_add_list(&mem_ioreg_list, "PIA1", 0xe810, 0xe81f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "PIA2", 0xe820, 0xe82f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "VIA", 0xe840, 0xe84f, mem_dump_io, NULL, IO_MIRROR_NONE);
    if (petres.crtc) {
        mon_ioreg_add_list(&mem_ioreg_list, "CRTC", 0xe880, 0xe881, mem_dump_io, NULL, IO_MIRROR_NONE);
    }
    if (pethre_enabled) {
        mon_ioreg_add_list(&mem_ioreg_list, "HRE", 0xe888, 0xe888, mem_dump_io, NULL, IO_MIRROR_NONE);
    }
    if (petres.map) {
        mon_ioreg_add_list(&mem_ioreg_list, "8096", 0xfff0, 0xfff0, mem_dump_io, NULL, IO_MIRROR_NONE);
    }
    if (petres.superpet) {
        mon_ioreg_add_list(&mem_ioreg_list, "6702", 0xefe0, 0xefe3, mem_dump_io, NULL, IO_MIRROR_NONE);
        mon_ioreg_add_list(&mem_ioreg_list, "ACIA", 0xeff0, 0xeff3, mem_dump_io, NULL, IO_MIRROR_NONE);
        mon_ioreg_add_list(&mem_ioreg_list, "Control", 0xeff8, 0xeff8, mem_dump_io, NULL, IO_MIRROR_NONE);
        mon_ioreg_add_list(&mem_ioreg_list, "Bank", 0xeffc, 0xeffc, mem_dump_io, NULL, IO_MIRROR_NONE);
        mon_ioreg_add_list(&mem_ioreg_list, "RAM/ROM", 0xeffe, 0xeffe, mem_dump_io, NULL, IO_MIRROR_NONE);
    }

    return mem_ioreg_list;
}

int petmem_get_screen_columns(void)
{
    int cols;

    cols = petres.video;
    if (!cols) {
        cols = petres.rom_video;
        if (!cols) {
            cols = PET_COLS;
        }
    }

    return cols;
}

int petmem_get_rom_columns(void)
{
    return petres.rom_video;
}

/* FIXME: this is incomplete */
void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    *base = 0x8000;
    *rows = 25;
    *columns = (uint8_t)petmem_get_screen_columns();
    *bank = 0;
}

/* used by autostart to locate and "read" kernal output on the current screen
 * this function should return whatever the kernal currently uses, regardless
 * what is currently visible/active in the UI. 
 */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column, uint8_t *line_length, int *blinking)
{
    /* Cursor Blink enable: 1 = Flash Cursor, 0 = Cursor disabled, -1 = n/a */
    if (petres.kernal_checksum == PET_KERNAL1_CHECKSUM) {
        *blinking = mem_ram[0x224] ? 0 : 1;
        *screen_addr = mem_ram[0xe0] + mem_ram[0xe1] * 256; /* Current Screen Line Address */
        *cursor_column = mem_ram[0xe2];    /* Cursor Column on Current Line */
    } else {
        *blinking = mem_ram[0xa7] ? 0 : 1;
        *screen_addr = mem_ram[0xc4] + mem_ram[0xc5] * 256; /* Current Screen Line Address */
        *cursor_column = mem_ram[0xc6];    /* Cursor Column on Current Line */
    }
    *line_length = petres.rom_video;   /* Physical Screen Line Length */
}

/************************** PET resource handling ************************/

/* check PetInfo struct for consistency after change? */

void petmem_check_info(petres_t *pi)
{
    if (pi->video == 40 || (pi->video == 0 && pi->rom_video == 40)) {
        pi->vmask = 0x3ff;
        pi->videoSize = 0x400;
    } else {
        pi->vmask = 0x7ff;
        pi->videoSize = 0x800;
    }

    if (pi->ramSize == 128) {
        pi->vmask = 0x1fff;
        pi->videoSize = 0x1000;
    }
}

/* dummy function to satisfy the global cartridge system */
int cartridge_attach_image(int type, const char *name)
{
    return -1;
}

void cartridge_detach_image(int type)
{
}

void cartridge_unset_default(void)
{
}

