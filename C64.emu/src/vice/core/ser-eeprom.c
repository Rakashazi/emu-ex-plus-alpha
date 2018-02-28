/*
 * ser-eeprom.c (M24C08)
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
#include <string.h> /* for memset */

#include "log.h"
#include "ser-eeprom.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

/* #define EEPROMDEBUG */
/* #define LOG_READ_BYTES */ /* log read bytes */
/* #define LOG_WRITE_BYTES */ /* log write bytes */
/* #define LOG_COMMANDS */ /* log eeprom commands */

#ifdef EEPROMDEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

/* FIXME get rid of this */
#define EEPROM_SIZE (1024)

/* Image file */
static FILE *eeprom_image_file = NULL;

#define EEPROM_IDLE         0
#define EEPROM_RESET        1
#define EEPROM_INSEQ        2
#define EEPROM_INCA0        3
#define EEPROM_INCA0_DATA   4
#define EEPROM_INCA1        5
#define EEPROM_INCA1_DATA   6
static unsigned int eeprom_mode = EEPROM_IDLE;

static BYTE eeprom_data[EEPROM_SIZE];
static unsigned int eeprom_readpos = 0;
static unsigned int eeprom_status = 0;

static unsigned int eeprom_readbit = 0;


/* TODO */
void eeprom_data_readadvance(void)
{
    eeprom_readpos++;
    eeprom_readpos &= ((EEPROM_SIZE * 8) - 1);        /* wrap at 8*1024 bit */
}

BYTE eeprom_data_readbyte(void)
{
    return eeprom_data[(eeprom_readpos >> 3) & 0x3ff];  /* FIXME: wraparound at 0x100 ?! */
}

BYTE eeprom_data_readbit(void)
{
    BYTE value;
    static BYTE bits[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
    int bitpos;
    bitpos = eeprom_readpos & 7;
    value = eeprom_data_readbyte();

#ifdef LOG_READ_BYTES
    if (bitpos == 0) {
        LOG(("EEPROM: eeprom_data_readbit[0x%04x]:0x%02x '%c'",
             eeprom_readpos >> 3, value, value));
    }
#endif

    if (value & bits[bitpos]) {
        return 1;
    }
    return 0;
}

BYTE eeprom_data_read(void)
{
    return eeprom_readbit;
}

static unsigned int eeprom_clk = 0;
static unsigned int eeprom_databit = 0;
static unsigned int eeprom_resetcount = 0;

/* TODO */
static unsigned int eeprom_writebit = 0;
static unsigned int eeprom_writeval = 0;
static unsigned int eeprom_writepos = 0;
/* TODO */
static unsigned int eeprom_cmdbit = 0;
static unsigned int eeprom_cmdval = 0;
static unsigned int eeprom_cmdpos = 0;
static unsigned char eeprom_cmdbuf[4];

void eeprom_cmd_reset (void)
{
    eeprom_cmdbit = 0;
    eeprom_cmdval = 0;
    eeprom_cmdpos = 0;
    memset(eeprom_cmdbuf, 0, 4);
}

void eeprom_cmd_write(BYTE value)
{
    static BYTE bits[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
/*LOG(("EEPROM: eeprom_cmd_write bit: %d:%02x",eeprom_cmdbit,value));*/
    if (value) {
        eeprom_cmdval |= bits[eeprom_cmdbit];
        eeprom_cmdbuf[eeprom_cmdpos] = eeprom_cmdval;
    }

    eeprom_cmdbit++;
    if (eeprom_cmdbit == 8) {
/*LOG(("EEPROM: eeprom_cmd_write: %d:%02x",eeprom_cmdpos,eeprom_cmdval));*/
        eeprom_cmdbuf[eeprom_cmdpos] = eeprom_cmdval;
        eeprom_cmdpos++;
        eeprom_cmdpos &= 3;
        eeprom_cmdval = 0;
        eeprom_cmdbit = 0;
    }
}

/* TODO */
static unsigned int eeprom_seqbit = 0;
static unsigned int eeprom_seqval = 0;
static unsigned int eeprom_seqpos = 0;
static unsigned char eeprom_seqbuf[4];

void eeprom_seq_reset(void)
{
    eeprom_seqbit = 0;
    eeprom_seqval = 0;
    eeprom_seqpos = 0;
    memset(eeprom_seqbuf, 0, 4);
}

void eeprom_seq_write(BYTE value)
{
    static BYTE bits[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
/*LOG(("EEPROM: eeprom_seq_write bit: %d:%02x",eeprom_seqbit,value));*/
    if (value) {
        eeprom_seqval |= bits[eeprom_seqbit];
        eeprom_seqbuf[eeprom_seqpos] = eeprom_seqval;
    }

    eeprom_seqbit++;
    if (eeprom_seqbit == 8) {
/*LOG(("EEPROM: eeprom_seq_write: %d:%02x",eeprom_seqpos,eeprom_seqval));*/
        eeprom_seqbuf[eeprom_seqpos] = eeprom_seqval;
        eeprom_seqpos++;
        eeprom_seqpos &= 3;
        eeprom_seqval = 0;
        eeprom_seqbit = 0;
    }
}

int eeprom_execute_command(int eeprom_mode)
{
    if ((eeprom_cmdbit != 0) || (eeprom_cmdpos == 0)) {
/*LOG(("eeprom mode:%02x %02x %02x",eeprom_mode,eeprom_cmdbit,eeprom_cmdpos)); */
        return eeprom_mode;
    }
/*LOG(("eeprom CMD:%02x mode:%02x %02x %02x",eeprom_cmdbuf[0],eeprom_mode,eeprom_cmdbit,eeprom_cmdpos)); */
    switch (eeprom_cmdbuf[0]) {
        case 0xa0:             /* set read/write position, switches to write mode */
            switch (eeprom_cmdpos) {
                case 1:
                    if (eeprom_mode != EEPROM_INSEQ) {
                        break;
                    }
/*LOG(("eeprom CMDA0     (%02x) %02x",eeprom_cmdbuf[0],eeprom_cmdbuf[1]));*/
                    eeprom_mode = EEPROM_INCA0;
                    break;
                case 2:
                    if (eeprom_mode != EEPROM_INCA0) {
                        break;
                    }
/*LOG(("eeprom CMDA0 pos %02x (%02x)",eeprom_cmdbuf[0],eeprom_cmdbuf[1])); */
                    eeprom_mode = EEPROM_INCA0_DATA;
                    eeprom_readpos = (eeprom_cmdbuf[1] << 3);
                    break;
                default:
                    if (eeprom_mode != EEPROM_INCA0_DATA) {
                        break;
                    }
#ifdef LOG_WRITE_BYTES
                    LOG(("EEPROM: CMDA0 write byte (%02x)  %02x:%02x '%c'",
                         eeprom_cmdbuf[0], eeprom_cmdbuf[1], eeprom_cmdbuf[2],
                         eeprom_cmdbuf[2]));
#endif
                    eeprom_data[(eeprom_readpos >> 3) & 0xff] = eeprom_cmdbuf[2]; /* FIXME: wraparound at 0x100 ?! */
                    break;
            }
            break;
        case 0xa1:
            switch (eeprom_cmdpos) {
                case 1:
                    if (eeprom_mode != EEPROM_INSEQ) {
                        break;
                    }
#ifdef LOG_COMMANDS
                    LOG(("eeprom CMDA1     (%02x) %02x", eeprom_cmdbuf[0],
                         eeprom_cmdbuf[1]));
#endif
                    eeprom_mode = EEPROM_INCA1_DATA;
                    eeprom_readpos = ((eeprom_cmdbuf[1]) & 0x03ff) << 3;
                    break;
                default:
                    if (eeprom_mode != EEPROM_INCA1_DATA) {
                        break;
                    }
#ifdef LOG_COMMANDS
                    LOG(("eeprom CMDA1 data [%04x]:%02x '%c'",
                         eeprom_readpos >> 3, eeprom_data_readbyte (),
                         eeprom_data_readbyte ()));
#endif
                    break;
            }
            break;
        default:
/*LOG(("eeprom cmd %d:%02x %02x",eeprom_cmdpos,eeprom_cmdbuf[0],eeprom_cmdbuf[1]));*/
            break;
    }
    return eeprom_mode;
}

void eeprom_port_write(BYTE clk, BYTE data, int ddr, int status)
{
    int nextmode;

    nextmode = eeprom_mode;
/*LOG(("eeprom_mode1 %d:%d",nextmode,eeprom_mode));*/

    /* reset state machine */
    if (data) {
        if (clk) {
            if (eeprom_clk) {
                eeprom_resetcount = 0;
            } else {
                eeprom_resetcount++;
            }
        } else {
            if (eeprom_databit) {
                if (eeprom_clk) {
                    eeprom_resetcount++;
                } else {
                    eeprom_resetcount = 0;
                }
            } else {
                eeprom_resetcount = 0;
            }
        }
    } else {
        eeprom_resetcount = 0;
    }
/*LOG(("eeprom_mode2 %x %d:%d",eeprom_resetcount,nextmode,eeprom_mode));*/
    if (eeprom_resetcount > 31) {
        if ((clk == 0) & (data != 0)) {
/*LOG(("eeprom_resetcount:%d",eeprom_resetcount));*/
            eeprom_mode = EEPROM_IDLE;
            nextmode = EEPROM_RESET;
            eeprom_writebit = 0;
            eeprom_writeval = 0;
            eeprom_writepos = 0;
            /* reset sequence */
            eeprom_seq_reset();
        }
    }
/*LOG(("eeprom_mode3 %d:%d",nextmode,eeprom_mode));*/

    switch (eeprom_mode) {
        case EEPROM_IDLE:
            break;
        case EEPROM_RESET:
            /* wait for start sequence */
            eeprom_seq_write(data);
            eeprom_seq_write(clk);
            if (eeprom_seqpos > 0) {     /* seqs >=8 bits */
                if (eeprom_seqbuf[0] == 0xb4) {  /* start sequence */
/*LOG(("eeprom start sequence"));*/
                    nextmode = EEPROM_INSEQ;
                    eeprom_seq_reset();
                }
            }
            break;
        case EEPROM_INSEQ:
        case EEPROM_INCA0:     /* write mode + count */
        case EEPROM_INCA0_DATA:        /* write mode + count */
        case EEPROM_INCA1:     /* read mode + count */
        case EEPROM_INCA1_DATA:        /* read mode + count */
            eeprom_seq_write(data);
            eeprom_seq_write(clk);

            if ((data == 1) && (clk == 1)) {
                /* FIXME */
                switch (eeprom_mode) {
                    case EEPROM_INCA1_DATA:
                        eeprom_readbit = eeprom_data_readbit();
                        break;
                    default:
                        eeprom_readbit = eeprom_status;
                        break;
                }
            }

            if (eeprom_seqpos > 0) {     /* seqs >=8 bits */
                switch (eeprom_seqbuf[0]) {
                    case 0x1e: /* (8bits) seq stop */
/*LOG(("eeprom stop sequence"));*/
                        nextmode = EEPROM_RESET;
                        eeprom_seq_reset();
                        eeprom_cmd_reset();
                        break;
                    case 0xb4: /* (8bits) seq start */
/*LOG(("eeprom start sequence"));*/
                        nextmode = EEPROM_INSEQ;
                        eeprom_seq_reset();
                        eeprom_cmd_reset();
                        break;
                    default:
/*LOG(("eeprom ==8 %d:%02x",eeprom_seqbit,eeprom_seqbuf[0]));*/
                        break;
                }
            } else if (eeprom_seqbit > 5) { /* 6 bits */
                switch (eeprom_seqbuf[0]) {
                    case 0xb8: /* (6bits) 1 bit (write) advance 1 bit (read) */
/*LOG(("eeprom bit: 1"));*/
                        eeprom_cmd_write(1);
                        eeprom_data_readadvance();
                        nextmode = eeprom_execute_command(eeprom_mode);
                        eeprom_seq_reset();
                        break;
                    case 0x10: /* (6bits) 0 bit */
/*LOG(("eeprom bit: 0"));*/
                        eeprom_cmd_write(0);
                        nextmode = eeprom_execute_command(eeprom_mode);
                        eeprom_seq_reset();
                        break;
                    case 0xb0: /* (6bits) write stop bit */
/*LOG(("eeprom stop bit"));*/
                        eeprom_seq_reset();
                        break;
                    default:
/*LOG(("eeprom <8 %d:%02x",eeprom_seqbit,eeprom_seqbuf[0]));*/
                        break;
                }
            }

            break;
    }

    eeprom_clk = clk;
    eeprom_databit = data;
    eeprom_mode = nextmode;
}

int eeprom_open_image(char *name, int rw)
{
    char *eeprom_image_filename = name;

    if (eeprom_image_filename != NULL) {
        /* FIXME */
    } else {
        /* FIXME */
        log_debug("eeprom card image name not set");
        return 0;
    }

    if (eeprom_image_file != NULL) {
        eeprom_close_image(rw);
    }

    if (rw) {
        eeprom_image_file = fopen(eeprom_image_filename, "rb+");
    }

    if (eeprom_image_file == NULL) {
        eeprom_image_file = fopen(eeprom_image_filename, "rb");

        if (eeprom_image_file == NULL) {
            log_debug("could not open eeprom card image: %s", eeprom_image_filename);
            return -1;
        } else {
            if (fread(eeprom_data, 1, EEPROM_SIZE, eeprom_image_file) == 0) {
                log_debug("could not read eeprom card image: %s", eeprom_image_filename);
            }
            fseek(eeprom_image_file, 0, SEEK_SET);
            log_debug("opened eeprom card image (ro): %s", eeprom_image_filename);
        }
    } else {
        if (fread(eeprom_data, 1, EEPROM_SIZE, eeprom_image_file) == 0) {
            log_debug("could not read eeprom card image: %s", eeprom_image_filename);
        }
        fseek(eeprom_image_file, 0, SEEK_SET);
        log_debug("opened eeprom card image (rw): %s", eeprom_image_filename);
    }
    return 0;
}

void eeprom_close_image(int rw)
{
    /* unmount EEPROM image */
    if (eeprom_image_file != NULL) {
        if (rw) {
            fseek(eeprom_image_file, 0, SEEK_SET);
            if (fwrite(eeprom_data, 1, EEPROM_SIZE, eeprom_image_file) == 0) {
                log_debug("could not write eeprom card image");
            }
        }
        fclose(eeprom_image_file);
        eeprom_image_file = NULL;
    }
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "EEPROM"

/* FIXME: implement snapshot support */
int eeprom_snapshot_write_module(snapshot_t *s)
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

int eeprom_snapshot_read_module(snapshot_t *s)
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
    return 0;
#endif
}
