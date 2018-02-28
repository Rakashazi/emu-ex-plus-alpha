/*
 * magicvoice.c - Speech Cartridge
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include "alarm.h"
#include "c64.h"
#include "c64cart.h" /* for export_t */
/* HACK: import main slot api although magic vice is a slot 0 cart, so we can handle the passthrough */
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#define CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT0_API
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "sound.h"
#include "t6721.h"
#include "tpi.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "magicvoice.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    Magic Voice

    U1  CD40105BE (RCA H 432) - 4bit*16 FIFO
    U2  MOS 6525A - Tri Port Interface
        - mapped at io2 (df80-df87)
    U3  8343SEA (LA05-123) - Gate Array (General Instruments)

    U5  T6721A - Voice Synthesizing LSI, Speech Generator, PARCOR Voice Synthesizer
        D0..D3,WR,EOS connected to PB0..PB3,PB4,PA6+PC2 of 6525
        DTRD,DI,phi2 connected to Gate Array

    U6  MOS 251476-01 (8A-06 4341) - 16K ROM

    U7  74LS257 or 74LS222A - Multiplexer with 3-State outputs (4*2 inputs -> 4 outputs)

        used to select wether a12..a15 for MV Cartridge Port comes from
        - the C64 Cartridge Port
        - PB0..PB3 of the 6525

    (note: yes, U4 is missing. these are referring to the schematics by Joachim Nemetz,
           which is like that :))

    ./src/x64 +cart -cartmv Original_MVSM_251476.bin
    ./src/x64 +cart -cartcrt gorf.crt -cartmv Original_MVSM_251476.bin

    say 0 .. N         say a word
    say "word"         say a word
    rate 0 .. 15?      set speed
    voc <addr>         set vocabulary
    rdy                ?

    - Programm starts after Reset at $FFD3
    copies code from $FF36-$FFD2 to $0200-$029C (157 bytes)
    - Programm continues at $021A
    copies $A000-$BFFF from EPROM to RAM at $A000-$BFFF, 8KB
    copies $E000-$FFFF from EPROM to RAM at $E000-$FFFF, 8KB
    copies $AE62-$B461 from RAM to RAM at $C000-$C5FF (Magic Voice Code)
    - Jump to beginning of Magic Voice code at $C000
*/

/* #define MVDEBUG */
/* #define CFGDEBUG */
/* #define FIFODEBUG */
/* #define REGDEBUG */
/* #define NMIDEBUG */

/* FIXME: test and then remove all old code */
/* #define USEPASSTHROUGHHACK */ /* define to use the old passthrough hack */

#ifdef MVDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

#ifdef REGDEBUG
#define DBGREG(x) printf x
#else
#define DBGREG(x)
#endif

static t6721_state *t6721 = NULL; /* context for the t6721 chip */
static tpi_context_t *tpi_context = NULL; /* context for the TPI chip */

#define MV_ROM_SIZE 0x4000
static BYTE mv_rom[MV_ROM_SIZE];

static int mv_extgame = 0, mv_extexrom = 0;
static int mv_game = 1, mv_exrom = 1;
static int mv_romA000_enabled = 1;
static int mv_romE000_enabled = 1;
static int mv_gameA000_enabled = 0;
static int mv_gameE000_enabled = 0;

static int mv_mapped_game = 1, mv_mapped_exrom = 1;

static int mv_game8000_enabled = 0; /* gamecart at passthrough enabled */
static int mv_game8000_atB000_enabled = 0;
static int mv_gameA000_at3000_enabled = 0;

static int mv_passthrough_addr = 0;

static void ga_memconfig_changed(int mode);

static void set_int(unsigned int int_num, int value);

/*****************************************************************************
 FIFO (CD 40105 BE)
*****************************************************************************/

/* IRQ bits according to TPI Port C */
#define NMI_APD         1
#define NMI_EOS         2
#define NMI_DTRD        3

#define FIFO_LEN        (32)
static unsigned int fifo_buffer = 0;
static int readptr, writeptr;
static int datainfifo = 0;
static int fifo_reset = 0;

static int DTRD = 0;

void update_dtrd(void)
{
#if 1
    if (datainfifo > (FIFO_LEN - 4)) {
        DTRD = 0;
    } else {
        DTRD = 1 & t6721->dtrd;
    }
#endif
}

/* hooked to callback of t6721 chip */
static BYTE read_data(t6721_state *t6721, unsigned int *bit)
{
    *bit = 0;

    if (datainfifo < 1) {
        return 0;
    }

    datainfifo--;
    update_dtrd();
    if (fifo_buffer & (1 << readptr)) {
        *bit = 1;
    }
    readptr++;

    if (readptr == FIFO_LEN) {
        readptr = 0;
    }
    return 1;
}

/*
   writes one bit to the FIFO
*/
static BYTE write_bit_to_fifo(BYTE bit)
{
    if (fifo_reset) {
        /* DBG(("SPEECH: first bit %04x %d\n", writeptr, bit)); */
        datainfifo = 0;
        readptr = 0;
        writeptr = 0;
    }
#if 0
    /* if dtrd==0, then run 1 tick, which makes dtrd==1 */
    if (!t6721->dtrd) {
        t6721_update_ticks(t6721, 1);
        update_dtrd(0);
        return 1;
    }
#endif
    if (datainfifo >= FIFO_LEN) {
        update_dtrd();
        t6721_update_ticks(t6721, 1);
        return 1;
    }

    if (bit) {
        bit = 1;
    }

    fifo_buffer &= ~(1 << writeptr);
    fifo_buffer |= (bit << writeptr);
    writeptr++;

    datainfifo++;
    update_dtrd();
    fifo_reset = 0; /* unset FIFO reset condition on first written byte */

    if (writeptr == FIFO_LEN) {
        writeptr = 0;
    }

    t6721_update_ticks(t6721, 1); /* run 1 tick, which gives the chip time to read 1 bit */
    return 0;
}

/*
   writes one nibble to the FIFO
*/
static void write_data_nibble(BYTE nibble)
{
    int i;
    BYTE mask;

#ifdef FIFODEBUG
/* DBG(("SPEECH: wr byte %04x\n", nibble)); */
    DBG(("[%x]", nibble));
#endif

    for (i = 0, mask = 1; i < 4; ++i, mask <<= 1) {
        if (write_bit_to_fifo((BYTE)(nibble & mask))) {
#ifdef FIFODEBUG
            DBG(("<!"));
#endif
            return;
        }
    }
}

/* hooked to callback of t6721 chip */
static void set_dtrd(t6721_state *t6721)
{
    static int old;
    if (old != t6721->dtrd) {
#ifdef IRQDEBUG
        DBG(("MV: set dtrd IRQ:%x\n", t6721->dtrd));
#endif
/*        DTRD = t6721->dtrd; */
        update_dtrd();
        tpicore_set_int(tpi_context, NMI_DTRD, t6721->dtrd);
        tpicore_set_int(tpi_context, NMI_DTRD, t6721->dtrd ^ 1);
#if 0
        if (t6721->dtrd) {
            cart_trigger_nmi();
        } else {
            cartridge_release_freeze();
        }
#endif
        old = t6721->dtrd;
    }
}

/* hooked to callback of t6721 chip */
static void set_apd(t6721_state *t6721)
{
    if (t6721->apd) {
        fifo_reset = 1; /* set FIFO reset condition */

        /* reset FIFO */
        writeptr = 0;
        readptr = 0;
        datainfifo = 0;

        update_dtrd();
    }
    DBG(("MV: set apd:%x\n", t6721->apd));
    /* tpicore_set_int(tpi_context, NMI_APD, t6721->apd ^ 1); */
}

/* hooked to callback of t6721 chip */
static void set_eos(t6721_state *t6721)
{
    DBG(("MV: set eos:%x\n", t6721->eos));
    tpicore_set_int(tpi_context, NMI_EOS, t6721->eos ^ 1);
    tpicore_set_int(tpi_context, NMI_EOS, t6721->eos);
}

/*****************************************************************************
 LA05-124 Gate Array

 4bit parallel to serial converter/buffer:
 
18 in   t6721 DTRD
20 in   t6721 phi2
 6 in   t6721 APD (reset, will also reset FIFO)

 2 in   FIFO Q0
 3 in   FIFO Q1
 4 in   FIFO Q2
 5 in   FIFO Q3

 1 out  FIFO CI
19 out  t6721 DI

 Address decoder:

25 in   C64 Cartridge Port IO2
26 in   C64 Cartridge Port ROML
21 in   C64 Cartridge Port ROMH
16 in   C64 Cartridge Port A15
13 in   C64 Cartridge Port A14
15 in   C64 Cartridge Port A13
12 in   C64 Cartridge Port A12
23 in   C64 Cartridge Port phi2

 7 in   6525 PC6 (CA) (with pullup) (toggles rom on/off ?)
 8 in   6525 PB5 (with pullup)
 9 in   6525 PB6 (with pullup)

10 out  chip select for 6525
22 out  chip select for MV ROM
17 out  MV Cartridge Port Multiplexer (LS257)
        (select wether A12..A15 for MV Cart Port comes from C64 Cart Port or PB0..PB3 of the 6525)

24 out  C64 Cartridge Port GAME

11 out  MV Cartridge Port ROMH
27 out  MV Cartridge Port ROML

exrom - does not go into the GA but due to the way we do the fake mapping it
        goes into the equations here too

reset goes to configs: 8, 11, 15, 2 (normal and with game)
then for game: 6, 14, 10, 2, 0

*****************************************************************************/
static int ga_pc6;
static int ga_pb5;
static int ga_pb6;

void ga_reset(void)
{
    ga_pc6 = 0;
    ga_pb5 = 0;
    ga_pb6 = 0;
}

static void ga_memconfig_changed(int mode)
{
#ifdef CFGDEBUG
    int n = 1;
    int this;
    static int last;
#endif
    mv_game8000_atB000_enabled = 0;
    mv_gameA000_at3000_enabled = 0;
    if (((mv_exrom == 0) && (ga_pc6 == 0) && (ga_pb5 == 0) && (ga_pb6 == 0))) { /* 0 */
        /* only with cart */
        /* game, before reading from memory */
        mv_game8000_atB000_enabled = 1;
        mv_gameA000_at3000_enabled = 1;
        mv_romA000_enabled = 1; /* ! */ /* code switching to this cfg is at 0xa... in mv rom */
        mv_romE000_enabled = 0; /* ! */
        mv_game = 0;
        mv_game8000_enabled = 1; /* ! */
        mv_gameA000_enabled = 0; /* ! */ /* code switching to this cfg is at 0xa... in mv rom */
        mv_gameE000_enabled = 1; /* ? */
    } else if (((mv_exrom == 0) && (ga_pc6 == 0) && (ga_pb5 == 1) && (ga_pb6 == 0))) { /* 2 */
        /* in init with "no cart", after "turn off basic" */
        /* game, after reading from memory */
        mv_romA000_enabled = 0;  /* ! */
        mv_romE000_enabled = 0;  /* ! */
        mv_game = 0;
        mv_game8000_enabled = 0;
        mv_gameA000_enabled = 0; /* ! */
        mv_gameE000_enabled = 0; /* ! */
#if 1
    } else if (((mv_exrom == 0) && (ga_pc6 == 0) && (ga_pb5 == 1) && (ga_pb6 == 1))) { /* 3 */
#if 0
        n = 0;
#endif
        /* used once in init in a loop ? */
        mv_romE000_enabled = 0;
        mv_romA000_enabled = 0;
        mv_game = 0;
        mv_game8000_enabled = 0; /* ? */
        mv_gameA000_enabled = 0; /* ? */
        mv_gameE000_enabled = 0; /* ? */
#endif
    } else if (((mv_exrom == 0) && (ga_pc6 == 1) && (ga_pb5 == 1) && (ga_pb6 == 0))) { /* 6 */
        /* only with cart */
        /* "game */
        mv_romA000_enabled = 0;
        mv_romE000_enabled = 0;
        mv_game = 0;
        mv_game8000_enabled = 1; /* ? */
        mv_gameA000_enabled = 1; /* ? */
        mv_gameE000_enabled = 1; /* ? */
    } else if (((mv_exrom == 0) && (ga_pc6 == 1) && (ga_pb5 == 1) && (ga_pb6 == 1))) { /* 7 */
#if 0
        n = 0;
#endif
        /* used once in init in a loop ? */
        mv_romA000_enabled = 1; /* ! */
        mv_romE000_enabled = 1; /* ! */
        mv_game = 0;
        mv_game8000_enabled = 0; /* ? */
        mv_gameA000_enabled = 0; /* ! */
        mv_gameE000_enabled = 0; /* ! */
    } else if (((mv_exrom == 1) && (ga_pc6 == 0) && (ga_pb5 == 0) && (ga_pb6 == 0))) { /* 8 */
        /* in init with "no cart" */
        mv_romA000_enabled = 1;
        mv_romE000_enabled = 1;
        mv_game = 0;
        mv_game8000_enabled = 0; /* ? */
        mv_gameA000_enabled = 0; /* ? */
        mv_gameE000_enabled = 0; /* ? */
    } else if (((mv_exrom == 1) && (ga_pc6 == 0) && (ga_pb5 == 1) && (ga_pb6 == 0))) { /* 10 */
        /* only with cart */
        /* (intermediate in "turn on/off basic") */
        mv_romA000_enabled = 0;
        mv_romE000_enabled = 0;
        mv_game = 1;
        mv_game8000_enabled = 1; /* ? */
        mv_gameA000_enabled = 1; /* ? */
        mv_gameE000_enabled = 0; /* ? */
    } else if (((mv_exrom == 1) && (ga_pc6 == 0) && (ga_pb5 == 1) && (ga_pb6 == 1))) { /* 11 */
        /* in init with "no cart" */
        mv_romE000_enabled = 1;
        mv_romA000_enabled = 1;
        mv_game = 0;
        mv_game8000_enabled = 0; /* ? */
        mv_gameA000_enabled = 0; /* ? */
        mv_gameE000_enabled = 0; /* ? */
#if 0
    } else if (((mv_exrom == 1) && (ga_pc6 == 1) && (ga_pb5 == 0) && (ga_pb6 == 0))) { /* 12 */
        /* NMI with cart ? */
        mv_romA000_enabled = 1;
        mv_romE000_enabled = 0;
        mv_game = 1;
        mv_game8000_enabled = 0;
        mv_gameA000_enabled = 0; /* ? */
        mv_gameE000_enabled = 0; /* ? */
#endif
    } else if (((mv_exrom == 1) && (ga_pc6 == 1) && (ga_pb5 == 1) && (ga_pb6 == 0))) { /* 14 */
        /* only with cart */
        /* before running "16k cart", after "turn on basic" */
        mv_romA000_enabled = 0;
        mv_romE000_enabled = 0;
        mv_game = 1;
        mv_game8000_enabled = 1;
        mv_gameA000_enabled = 1;
        mv_gameE000_enabled = 0; /* ? */
    } else if (((mv_exrom == 1) && (ga_pc6 == 1) && (ga_pb5 == 1) && (ga_pb6 == 1))) { /* 15 */
        /* in init with "no cart" */
        mv_romA000_enabled = 1;
        mv_romE000_enabled = 1;
        mv_game = 0;
        mv_game8000_enabled = 0; /* ? */
        mv_gameA000_enabled = 0; /* ? */
        mv_gameE000_enabled = 0; /* ? */
    } else {
#if 0
        n = 2;
#endif
        mv_romA000_enabled = 0;
        mv_romE000_enabled = 0;
        mv_game = 0;
        mv_game8000_enabled = 0;
        mv_gameA000_enabled = 0;
        mv_gameE000_enabled = 0;
    }

    cart_config_changed_slot0((BYTE)((mv_mapped_game) | ((mv_mapped_exrom) << 1)), (BYTE)((mv_mapped_game) | ((mv_mapped_exrom) << 1)), mode);

#ifdef CFGDEBUG
    this = (ga_pb6 << 0) | (ga_pb5 << 1) | (ga_pc6 << 2) | (mv_exrom << 3);

    if (last != this) {
        if (n == 2) {
            DBG(("-->"));
        }
        if (n) {
            DBG(("MV: @$%04x config (%2d) exrom %d pc6: %d pb5: %d pb6: %d | game: %d mv A000: %d E000: %d game 8000: %d  A000: %d E000: %d ",
                 reg_pc, this, mv_exrom, ga_pc6, ga_pb5, ga_pb6, mv_game, mv_romA000_enabled, mv_romE000_enabled, mv_game8000_enabled, mv_gameA000_enabled, mv_gameE000_enabled));
            if (mv_game8000_atB000_enabled || mv_gameA000_at3000_enabled) {
                DBG(("(%04x)", mv_passthrough_addr));
            }
            switch ((mv_exrom << 1) | mv_game) {
                case 0:
                    DBG(("(ram)\n"));
                    break;
                case 1:
                    DBG(("(8k game)\n"));
                    break;
                case 2:
                    DBG(("(ultimax)\n"));
                    break;
                case 3:
                    DBG(("(16k game)\n"));
                    break;
            }
        }
    }
    last = this;
#endif
}

/*****************************************************************************
 callbacks for the TPI
*****************************************************************************/

static void set_int(unsigned int int_num, int value)
{
    static int old;
    int isirq;

    isirq = (((tpi_context->c_tpi[TPI_ILR]) & tpi_context->c_tpi[TPI_IMR]) & 0x0f);
    if (old != isirq) {
#ifdef IRQDEBUG
        DBG(("MV: TPI set NMI %d  num:%02x val:%02x ILR:%02x IMR:%02x\n", isirq, int_num, value, tpi_context->c_tpi[TPI_ILR], tpi_context->c_tpi[TPI_IMR]));
#endif
#if 1
        if (isirq) {
            cart_trigger_nmi();
        } else {
            cartridge_release_freeze();
        }
#endif
    }

    old = isirq;
}

static void restore_int(unsigned int int_num, int value)
{
    DBG(("MV: tpi restore int %02x %02x\n", int_num, value));
}

static void reset(tpi_context_t *tpi_context)
{
    DBG(("MV: tpi reset\n"));
}

/*
    Port A (df80)

    PA0..3      OUT: D0..D3 Data -> FIFO -> Gate Array -> T6721 DI (highest Nibble first)
    PA4         OUT: -> FIFO SI Shift in to FIFO (L->H "Pretty Please")
    PA5         IN:  !GAME of the Magic Voice Cartridge Passthrough Port (with pullup)
    PA6         IN:  !EOS <- T6721 (End of Speech)
    PA7         IN:  <- FIFO CO (Data in Ready)

    eos LOW:  End of Speech (LOW for one Frame only, about 10 or 20ms)
        HIGH: No voice is synthesized
    dir LOW: FIFO is full/busy
        HIGH: FIFO is ready to accept data
*/

#ifdef USEPASSTHROUGHHACK
#define MV_GAME_NOCART  1
#define MV_EXROM_NOCART 1
#define MV_GAME_GAMECART  0
#define MV_EXROM_GAMECART 0
#endif

static BYTE read_pa(tpi_context_t *tpi_context)
{
    BYTE byte = 0;
#ifdef USEPASSTHROUGHHACK
    if (cart_getid_slotmain() != CARTRIDGE_NONE) {
        /* passthrough */
        byte |= (MV_GAME_GAMECART << 5); /* passthrough !GAME */
    } else {
        byte |= (MV_GAME_NOCART << 5); /* passthrough !GAME */
    }
#else
    /* DBG(("MV: read pa extgame %d\n", mv_extgame)); */
    byte |= ((mv_extgame ^ 1) << 5);
#endif

    byte |= ((t6721->eos ^ 1) << 6); /* !EOS = End of Speech from T6721 */
    byte |= (DTRD << 7); /* DIR = Data in Ready from 40105 */

    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPA]) | (tpi_context->c_tpi[TPI_PA] & tpi_context->c_tpi[TPI_DDPA]);
    /* DBG(("MV: read pa %02x\n", byte)); */

    return byte;
}

static void store_pa(tpi_context_t *tpi_context, BYTE byte)
{
    static BYTE last;
/* DBG(("MV: store pa %02x\n", byte)); */
    if ((byte & 0x10)) {
        /* out: PB3..PB0 go to D3..D0*/
        write_data_nibble((BYTE)(last & 0x0f)); /* write nibble to FIFO */
    }
    last = byte;
}

/*
    Port B (df81)

    PB0..3      OUT: D0..D3 Data -> T6721 D0..D3 (highest Nibble first)
                IN: T6721 D0..D3 (Status)
                OUT: A15,14,13,12 for passthrough port (if enabled)
    PB4         OUT: !WR -> T6721 WR (write to T6721, L->H "Pretty Please")
    PB5         OUT? -> Gate Array (with pullup)
    PB6         OUT? -> Gate Array (with pullup)
    PB7         IN:  !EXROM <- Exrom of the MV Cartridge Port (with pullup)
*/
static void store_pb(tpi_context_t *tpi_context, BYTE byte)
{
/* DBG(("MV: store pp %02x\n", byte)); */
    t6721->wr = (byte >> 4) & 1; /* wr line */
    /* out: PB3..PB0 go to D3..D0*/
    t6721_store(t6721, (BYTE)(byte & 0x0f));

    mv_passthrough_addr = (int)(byte & 0x0f) << 12;
    ga_pb5 = (byte >> 5) & 1;
    ga_pb6 = (byte >> 6) & 1;
    ga_memconfig_changed(CMODE_READ);
}

static BYTE read_pb(tpi_context_t *tpi_context)
{
    BYTE byte = 0;

    byte |= t6721_read(t6721) & 0x0f;
#if 0
    byte |= (1 << 5); /* ? pullup */
    byte |= (1 << 6); /* ? pullup */
#endif

#ifdef USEPASSTHROUGHHACK
    if (cart_getid_slotmain() != CARTRIDGE_NONE) {
        /* passthrough */
        byte |= (MV_EXROM_GAMECART << 7); /* passthrough !EXROM */
    } else {
        byte |= (MV_EXROM_NOCART << 7); /* passthrough !EXROM */
    }
#else
    /* DBG(("MV: read pb extexrom %d\n", mv_extexrom)); */
    byte |= ((mv_extexrom ^ 1) << 7);
#endif
    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPB]) | (tpi_context->c_tpi[TPI_PB] & tpi_context->c_tpi[TPI_DDPB]);

    return byte;
}

/*
    Port C

    PC0              unused ?
    PC1              unused ?
    PC2   IRQ   IN:  !EOS <- T6721 (End of Speech)
    PC3   IRQ   IN:  <- FIFO CO (Data in Ready)
    PC4              unused ?
    PC5         OUT: !NMI -> Cartridge Port (automatically generated if /EOS or DIR occurs)
    PC6         OUT: (CA) CA ? <-> Gate Array (with pullup) (toggles rom on/off ?)
    PC7         OUT: (CB) !EXROM -> C64 Cartridge Port
*/
static void store_pc(tpi_context_t *tpi_context, BYTE byte)
{
    /* this function is actually never used ? */
    DBG(("MV: store pc %02x\n", byte));
#if 0
    if ((byte & 0x20) == 0) {
        DBG(("MV: triggered NMI ?\n"));
        /* OUT: !NMI (automatically generated if /EOS or DIR occurs) */
        /* cartridge_trigger_freeze_nmi_only(); */
    } else {
        DBG(("MV: untriggered NMI ?\n"));
    }
#endif
#if 0
    ga_pc6 = (byte >> 6) & 1;
    ga_memconfig_changed(CMODE_READ);
#endif
}

static BYTE read_pc(tpi_context_t *tpi_context)
{
    static BYTE byte = 0;
#if 0
    /* IRQ inputs */
    byte |= ((t6721->eos ^ 1) << 2); /* !EOS (End of Speech) */
    byte |= (DTRD << 3); /* DIR (Data in Ready) */
#endif
    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPC]) | (tpi_context->c_tpi[TPI_PC] & tpi_context->c_tpi[TPI_DDPC]);
    DBG(("MV: read pc %02x\n", byte));
    return byte;
}

static void set_ca(tpi_context_t *tpi_context, int a)
{
/* DBG(("MV: set ca %02x\n", a)); */
    ga_pc6 = (a != 0);
    ga_memconfig_changed(CMODE_READ);
}

static void set_cb(tpi_context_t *tpi_context, int a)
{
/* DBG(("MV: set cb %02x\n", a)); */
    mv_exrom = (a == 0);
    ga_memconfig_changed(CMODE_READ);
}

static void undump_pa(tpi_context_t *tpi_context, BYTE byte)
{
    DBG(("MV: undump pa %02x\n", byte));
}

static void undump_pb(tpi_context_t *tpi_context, BYTE byte)
{
    DBG(("MV: undump pb %02x\n", byte));
}

static void undump_pc(tpi_context_t *tpi_context, BYTE byte)
{
    DBG(("MV: undump pc %02x\n", byte));
}

/*****************************************************************************
 I/O Area
*****************************************************************************/

static void magicvoice_io2_store(WORD addr, BYTE data)
{
    switch (addr & 7) {
        case 5:
            DBGREG(("MV: @:%04x io2 w %04x %02x (IRQ Mask)\n", reg_pc, addr, data));
            break;
        default:
            DBGREG(("MV: @:%04x io2 w %04x %02x\n", reg_pc, addr, data));
            break;
        case 0:
        case 2:
        case 6:
            break;
    }
    tpicore_store(tpi_context, (WORD)(addr & 7), data);
}

static BYTE magicvoice_io2_read(WORD addr)
{
    BYTE value = 0;
    value = tpicore_read(tpi_context, (WORD)(addr & 7));
    switch (addr & 7) {
        case 5:
            DBGREG(("MV: @:%04x io2 r %04x %02x (IRQ Mask)\n", reg_pc, addr, value));
            break;
        case 7:
            /* FIXME: this register contains a wrong value when checked by the software */
#if 1
            value &= ~(1 << 3);
            value |= ((t6721->playing) << 3);   /* hack: dtrd */
            value &= ~(1 << 2);
            value |= ((t6721->eos) << 2);  /* hack: eos */
#endif
            DBGREG(("MV: @:%04x io2 r %04x %02x (Active IRQs)\n", reg_pc, addr, value));
            break;
        default:
            DBGREG(("MV: @:%04x io2 r %04x %02x\n", reg_pc, addr, value));
            break;
        case 0:
        case 2:
            break;
    }
    return value;
}

static BYTE magicvoice_io2_peek(WORD addr)
{
    return tpicore_peek(tpi_context, (WORD)(addr & 7));
}

static int magicvoice_io2_dump(void)
{
    mon_out("TPI\n");
    tpicore_dump(tpi_context);
    mon_out("T6721:\n");
    t6721_dump(t6721);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t magicvoice_io2_device = {
    CARTRIDGE_NAME_MAGIC_VOICE,
    IO_DETACH_CART,
    NULL,
    0xdf80, 0xdfff, 0x07,
    1, /* read is always valid */
    magicvoice_io2_store,
    magicvoice_io2_read,
    magicvoice_io2_peek,
    magicvoice_io2_dump,
    CARTRIDGE_MAGIC_VOICE,
    0,
    0
};

static io_source_list_t *magicvoice_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MAGIC_VOICE, 1, 1, NULL, &magicvoice_io2_device, CARTRIDGE_MAGIC_VOICE
};

/* ---------------------------------------------------------------------*/
/* Some prototypes are needed */
static int magicvoice_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static void magicvoice_sound_machine_close(sound_t *psid);
static int magicvoice_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void magicvoice_sound_machine_store(sound_t *psid, WORD addr, BYTE byte);
static BYTE magicvoice_sound_machine_read(sound_t *psid, WORD addr);
static void magicvoice_sound_machine_reset(sound_t *psid, CLOCK cpu_clk);

static int magicvoice_sound_machine_cycle_based(void)
{
    return 0;
}

static int magicvoice_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t magicvoice_sound_chip = {
    NULL, /* no open */
    magicvoice_sound_machine_init,
    magicvoice_sound_machine_close,
    magicvoice_sound_machine_calculate_samples,
    magicvoice_sound_machine_store,
    magicvoice_sound_machine_read,
    magicvoice_sound_machine_reset,
    magicvoice_sound_machine_cycle_based,
    magicvoice_sound_machine_channels,
    0 /* chip enabled */
};

static WORD magicvoice_sound_chip_offset = 0;

void magicvoice_sound_chip_init(void)
{
    magicvoice_sound_chip_offset = sound_chip_register(&magicvoice_sound_chip);
}

int magicvoice_cart_enabled(void)
{
    return magicvoice_sound_chip.chip_enabled;
}

/* ---------------------------------------------------------------------*/

static BYTE read_remapped(WORD addr)
{
    addr &= 0x0fff;
    addr |= mv_passthrough_addr;
    if (addr <= 0x9fff) {
        return roml_banks[addr & 0x1fff];
    }
    return romh_banks[addr & 0x1fff];
}

int magicvoice_ultimax_read(WORD addr, BYTE *value)
{
    if (mv_gameA000_at3000_enabled) {
        /* FIXME: hack! */
        if ((addr >= 0x3000) && (addr <= 0x3fff)) {
            *value = read_remapped(addr);
            return 1;
        }
    }
    /* disabled, read c64 memory */
    return CART_READ_C64MEM;
}

int magicvoice_roml_read(WORD addr, BYTE *value)
{
#if 0
    if ((mv_game8000_enabled) && (cart_getid_slotmain() != CARTRIDGE_NONE)) {
        /* "passthrough" */
        *value = roml_banks[(addr & 0x1fff)];
        return 1;
    }
    *value = ram_read(addr);
    return 1;
#else
    if (mv_game8000_enabled) {
        /* "passthrough" */
        return CART_READ_THROUGH;
    }
    /* disabled, read c64 memory */
    return CART_READ_C64MEM;
#endif
}

int magicvoice_a000_bfff_read(WORD addr, BYTE *value)
{
#if 0
    if ((mv_gameA000_enabled) && (cart_getid_slotmain() != CARTRIDGE_NONE)) {
        /* "passthrough" */
        /* return mv_rom[(addr & 0x1fff)]; */
        *value = romh_banks[(addr & 0x1fff)];
        return 1;
    } else {
        if (mv_romA000_enabled) {
            *value = mv_rom[(addr & 0x1fff)];
            return 1;
        } else {
            *value = mem_read_without_ultimax(addr);
            return 1;
        }
    }
    return 1;
#else
    if (mv_game8000_atB000_enabled) {
        /* FIXME: hack! */
        if ((addr >= 0xb000) && (addr <= 0xbfff)) {
            *value = read_remapped(addr);
            return 1;
        }
    }
    if (mv_gameA000_enabled) {
        /* "passthrough" */
        return CART_READ_THROUGH_NO_ULTIMAX;
    } else {
        if (mv_romA000_enabled) {
            *value = mv_rom[(addr & 0x1fff)];
            return CART_READ_VALID;
        }
    }
    /* disabled, read c64 memory */
    return CART_READ_C64MEM;
#endif
}

int magicvoice_romh_read(WORD addr, BYTE *value)
{
#if 0
    if (addr == 0xfffa) {
        DBG(("MV: fetch vector %04x game: %d e000: %d\n", addr, mv_game8000_enabled, mv_romE000_enabled));
    }
#endif
#if 0
    if ((mv_gameE000_enabled) && (cart_getid_slotmain() != CARTRIDGE_NONE)) {
        /* "passthrough" */
        /* return mv_rom[(addr & 0x1fff) + 0x2000]; */
        *value = romh_banks[(addr & 0x1fff)];
        return CART_READ_VALID;
    } else {
        if (mv_romE000_enabled) {
            *value = mv_rom[(addr & 0x1fff) + 0x2000];
            return CART_READ_VALID;
        } else {
            *value = mem_read_without_ultimax(addr);
            return CART_READ_VALID;
        }
    }
    return CART_READ_VALID;
#else
    if (mv_gameE000_enabled) {
        /* "passthrough" */
        return CART_READ_THROUGH;
    } else {
        if (mv_romE000_enabled) {
            *value = mv_rom[(addr & 0x1fff) + 0x2000];
            return CART_READ_VALID;
        }
    }
    /* disabled, read c64 memory */
    return CART_READ_C64MEM;
#endif
}

int magicvoice_romh_phi1_read(WORD addr, BYTE *value)
{
    if ((mv_gameE000_enabled) && (mv_extexrom == 0) && (mv_extgame == 1)) {
        /* real ultimax mode for game */
        return CART_READ_THROUGH;
    }
    return CART_READ_C64MEM;
}

int magicvoice_romh_phi2_read(WORD addr, BYTE *value)
{
    if ((mv_gameE000_enabled) && (mv_extexrom == 0) && (mv_extgame == 1)) {
        /* real ultimax mode for game */
        return CART_READ_THROUGH;
    }
    return CART_READ_C64MEM;
}

int magicvoice_peek_mem(WORD addr, BYTE *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        if (mv_game8000_enabled) {
            /* "passthrough" */
            return CART_READ_THROUGH;
        }
        /* disabled, read c64 memory */
    } else if ((addr >= 0xa000) && (addr <= 0xbfff)) {
        if (mv_gameA000_enabled) {
            /* "passthrough" */
            return CART_READ_THROUGH_NO_ULTIMAX;
        } else {
            if (mv_romA000_enabled) {
                *value = mv_rom[(addr & 0x1fff)];
                return CART_READ_VALID;
            }
        }
        /* disabled, read c64 memory */
    } else if (addr >= 0xe000) {
        if (mv_gameE000_enabled) {
            /* "passthrough" */
            return CART_READ_THROUGH;
        } else {
            if (mv_romE000_enabled) {
                *value = mv_rom[(addr & 0x1fff) + 0x2000];
                return CART_READ_VALID;
            }
        }
        /* disabled, read c64 memory */
    }
    return CART_READ_C64MEM;
}

void magicvoice_passthrough_changed(export_t *export)
{
    mv_extexrom = export->exrom;
    mv_extgame = export->game;
    DBG(("MV passthrough changed exrom: %d game: %d\n", mv_extexrom, mv_extgame));

    ga_memconfig_changed(CMODE_READ);
}

/* ---------------------------------------------------------------------*/

char *magicvoice_filename = NULL;

static int set_magicvoice_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("MV: set_enabled: '%s' %d to %d\n", magicvoice_filename, magicvoice_sound_chip.chip_enabled, val));
    if (magicvoice_sound_chip.chip_enabled && !val) {
        cart_power_off();
#ifdef MVDEBUG
        if (magicvoice_io2_list_item == NULL) {
            DBG(("MV: BUG: magicvoice_sound_chip.chip_enabled == 1 and magicvoice_io2_list_item == NULL ?!\n"));
        }
#endif
        export_remove(&export_res);
        io_source_unregister(magicvoice_io2_list_item);
        magicvoice_io2_list_item = NULL;
        magicvoice_sound_chip.chip_enabled = 0;
        DBG(("MV: set_enabled unregistered\n"));
    } else if (!magicvoice_sound_chip.chip_enabled && val) {
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            if (magicvoice_filename) {
                if (*magicvoice_filename) {
                    if (cartridge_attach_image(CARTRIDGE_MAGIC_VOICE, magicvoice_filename) < 0) {
                        DBG(("MV: set_enabled did not register\n"));
                        return -1;
                    }
                    /* magicvoice_sound_chip.chip_enabled = 1; */ /* cartridge_attach_image will end up calling set_magicvoice_enabled again */
                    return 0;
                }
            }
        } else {
            cart_power_off();
            /* if the param is == NULL, then we should actually set the resource */
            if (export_add(&export_res) < 0) {
                DBG(("MV: set_enabled did not register\n"));
                return -1;
            } else {
                DBG(("MV: set_enabled registered\n"));
                magicvoice_io2_list_item = io_source_register(&magicvoice_io2_device);
                magicvoice_sound_chip.chip_enabled = 1;
            }
        }
    }

    DBG(("MV: set_enabled done: '%s' %d : %d\n", magicvoice_filename, val, magicvoice_sound_chip.chip_enabled));
    return 0;
}

static int set_magicvoice_filename(const char *name, void *param)
{
    int enabled;

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }
    DBG(("MV: set_name: %d '%s'\n", magicvoice_sound_chip.chip_enabled, magicvoice_filename));

    util_string_set(&magicvoice_filename, name);
    resources_get_int("MagicVoiceCartridgeEnabled", &enabled);

    if (set_magicvoice_enabled(enabled, (void*)1) < 0) {
        lib_free(magicvoice_filename);
        magicvoice_filename = NULL;
        DBG(("MV: set_name done: %d '%s'\n", magicvoice_sound_chip.chip_enabled, magicvoice_filename));
        return -1;
    }
    DBG(("MV: set_name done: %d '%s'\n", magicvoice_sound_chip.chip_enabled, magicvoice_filename));
    return 0;
}

static const resource_string_t resources_string[] = {
    { "MagicVoiceImage", "", RES_EVENT_NO, NULL,
      &magicvoice_filename, set_magicvoice_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MagicVoiceCartridgeEnabled", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &magicvoice_sound_chip.chip_enabled, set_magicvoice_enabled, (void *)1 },
    RESOURCE_INT_LIST_END
};

int magicvoice_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void magicvoice_resources_shutdown(void)
{
    lib_free(magicvoice_filename);
    magicvoice_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-magicvoiceimage", SET_RESOURCE, 1,
      NULL, NULL, "MagicVoiceImage", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_MAGICVOICE_IMAGE_NAME,
      NULL, NULL },
    { "-magicvoice", SET_RESOURCE, 0,
      NULL, NULL, "MagicVoiceCartridgeEnabled", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_MAGICVOICE,
      NULL, NULL },
    { "+magicvoice", SET_RESOURCE, 0,
      NULL, NULL, "MagicVoiceCartridgeEnabled", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_MAGICVOICE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int magicvoice_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

void magicvoice_shutdown(void)
{
    if (tpi_context) {
        tpicore_shutdown(tpi_context);
        tpi_context = NULL;
    }
    if (t6721) {
        lib_free(t6721);
        t6721 = NULL;
    }
}

void magicvoice_setup_context(machine_context_t *machine_context)
{
    DBG(("MV: setup_context\n"));

    tpi_context = lib_calloc(1, sizeof(tpi_context_t));

    tpi_context->prv = NULL;

    tpi_context->context = (void *)machine_context;

    tpi_context->rmw_flag = &maincpu_rmw_flag;
    tpi_context->clk_ptr = &maincpu_clk;

    tpi_context->myname = lib_msprintf("TPI");

    tpicore_setup_context(tpi_context);

    tpi_context->tpi_int_num = IK_NMI;

    tpi_context->store_pa = store_pa;
    tpi_context->store_pb = store_pb;
    tpi_context->store_pc = store_pc;
    tpi_context->read_pa = read_pa;
    tpi_context->read_pb = read_pb;
    tpi_context->read_pc = read_pc;
    tpi_context->undump_pa = undump_pa;
    tpi_context->undump_pb = undump_pb;
    tpi_context->undump_pc = undump_pc;
    tpi_context->reset = reset;
    tpi_context->set_ca = set_ca;
    tpi_context->set_cb = set_cb;
    tpi_context->set_int = set_int;
    tpi_context->restore_int = restore_int;

    /* init t6721 chip */
    t6721 = lib_calloc(1, sizeof(t6721_state));
    t6721->read_data = read_data;
    t6721->set_apd = set_apd;
    t6721->set_eos = set_eos;
    t6721->set_dtrd = set_dtrd;
    t6721_reset(t6721);
}

#ifdef NMIDEBUG
static void mv_ack_nmi(void)
{
    DBG(("MV: ack nmi\n"));
}
#endif

int magicvoice_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xf000) {
        case 0xf000:
        case 0xe000:
            if (mv_gameE000_enabled) {
                return CART_READ_THROUGH; /* "passthrough" */
            } else {
                if (mv_romE000_enabled) {
                    *base = (BYTE *)(mv_rom - (BYTE *)0xc000);
                    *start = 0xe000;
                    *limit = 0xfffd;
                    return CART_READ_VALID;
                }
            }
            return CART_READ_C64MEM; /* disabled, read c64 memory */
        case 0xb000:
        case 0xa000:
            if (mv_game8000_atB000_enabled) {
                /* FIXME: proper mapping */
                *base = NULL;
                *start = 0;
                *limit = 0;
                return CART_READ_VALID;
            }
            if (mv_gameA000_enabled) {
                return CART_READ_THROUGH_NO_ULTIMAX; /* "passthrough" */
            } else {
                if (mv_romA000_enabled) {
                    *base = (BYTE *)(mv_rom - (BYTE *)0xa000);
                    *start = 0xa000;
                    *limit = 0xbffd;
                    return CART_READ_VALID;
                }
            }
            /* disabled, read c64 memory */
            return CART_READ_C64MEM;
        case 0x9000:
        case 0x8000:
            if (mv_game8000_enabled) { /* "passthrough" */
                return CART_READ_THROUGH;
            }
            return CART_READ_C64MEM; /* disabled, read c64 memory */
        case 0x3000:
            if (mv_gameA000_at3000_enabled) {
                /* FIXME: proper mapping */
                *base = NULL;
                *start = 0;
                *limit = 0;
                return CART_READ_VALID;
            } /* fall through */
        case 0x7000:
        case 0x6000:
        case 0x5000:
        case 0x4000:
        case 0x2000:
        case 0x1000:
            /* disabled, read c64 memory */
            return CART_READ_C64MEM;
        default:
            break;
    }
    return CART_READ_THROUGH;
}

/* called at reset */
void magicvoice_config_init(export_t *export)
{
    DBG(("MV: magicvoice_config_init\n"));

    mv_extexrom = export->exrom;
    mv_extgame = export->game;

    if (magicvoice_sound_chip.chip_enabled) {
        mv_exrom = 1;
        ga_reset();
        ga_memconfig_changed(CMODE_READ);
#ifdef NMIDEBUG
        interrupt_set_nmi_trap_func(maincpu_int_status, mv_ack_nmi);
#endif
    }
}

void magicvoice_config_setup(BYTE *rawcart)
{
    DBG(("MV: magicvoice_config_setup\n"));
    memcpy(mv_rom, rawcart, 0x4000);
}

/* ---------------------------------------------------------------------*/

const char *magicvoice_get_file_name(void)
{
    return magicvoice_filename;
}

static int magicvoice_common_attach(void)
{
    DBG(("MV: attach\n"));
    return set_magicvoice_enabled(1, NULL);
}

int magicvoice_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, MV_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return magicvoice_common_attach();
}

/*
 * (old) wrong formats:
 *
 * cartconv produced this until 2011:
 *
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $2000 $2010
 * $002050 CHIP ROM   #001 $8000 $2000 $2010
 *
 * cartconv produced this from 2011 to 12/2015:
 *
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $2000 $2010
 * $002050 CHIP ROM   #000 $a000 $2000 $2010
 *
 * (new) correct format (since 12/2015):
 *
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $4000 $4010
 *
 */
int magicvoice_crt_attach(FILE *fd, BYTE *rawcart)
{
    int i;
    crt_chip_header_t chip;

    for (i = 0; i < 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if ((chip.size == 0x4000) && (chip.start == 0x8000)) {
            if (crt_read_chip(rawcart, 0, &chip, fd)) {
                return -1;
            }
        } else if ((chip.size == 0x2000) && ((chip.start == 0x8000) || (chip.start == 0xa000))) {
            if (crt_read_chip(rawcart, (chip.start & 0x2000) + (chip.bank << 13), &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }

    }
    if (i != 1 && i != 2) {
        return -1;
    }
    return magicvoice_common_attach();
}

void magicvoice_detach(void)
{
    DBG(("MV: detach %d %p\n", magicvoice_sound_chip.chip_enabled, magicvoice_io2_list_item));
    set_magicvoice_enabled(0, NULL);
}

int magicvoice_enable(void)
{
    DBG(("MV: enable\n"));
    return set_magicvoice_enabled(1, (void*)1);
}

void magicvoice_init(void)
{
    DBG(("MV: init\n"));
    tpi_context->log = log_open(tpi_context->myname);
    mv_extgame = 0;
    mv_extexrom = 0;
}

void magicvoice_reset(void)
{
    DBG(("MV: reset\n"));
    if (magicvoice_sound_chip.chip_enabled) {
        mv_game8000_enabled = 0;
        mv_exrom = 1;
        ga_reset();
        t6721_reset(t6721);
        tpicore_reset(tpi_context);
        ga_memconfig_changed(CMODE_READ);
    }
}

/* ---------------------------------------------------------------------*/

/* FIXME: what are these two about anyway ? */
static BYTE magicvoice_sound_machine_read(sound_t *psid, WORD addr)
{
    DBG(("MV: magicvoice_sound_machine_read\n"));

    return 0; /* ? */
}

static void magicvoice_sound_machine_store(sound_t *psid, WORD addr, BYTE byte)
{
    DBG(("MV: magicvoice_sound_machine_store\n"));
}

/*
    called periodically for every sound fragment that is played
*/
static int magicvoice_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    SWORD *buffer;

    buffer = lib_malloc(nr * sizeof(SWORD));

    t6721_update_output(t6721, buffer, nr);

    /* mix generated samples to output */
    for (i = 0; i < nr; i++) {
        pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], buffer[i]);
        if (soc > 1) {
            pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], buffer[i]);
        }
    }

    lib_free(buffer);

    return nr;
}

static void magicvoice_sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
    DBG(("MV: magicvoice_sound_machine_reset\n"));
}

static int magicvoice_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    DBG(("MV: speech_sound_machine_init: speed %d cycles/sec: %d\n", speed, cycles_per_sec));
    t6721_sound_machine_init(t6721, speed, cycles_per_sec);

    return 1;
}

static void magicvoice_sound_machine_close(sound_t *psid)
{
    DBG(("MV: magicvoice_sound_machine_close\n"));
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTMAGICVOICE"

/* FIXME: implement snapshot support */
int magicvoice_snapshot_write_module(snapshot_t *s)
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

int magicvoice_snapshot_read_module(snapshot_t *s)
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

    if (magicvoice_common_attach() < 0) {
        return -1;
    }
    return 0;
#endif
}
