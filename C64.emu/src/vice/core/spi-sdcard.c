/*
 * spi-sdcard.c - SD Card over SPI Emulation
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
 * large parts derived from mmc64.c written by
 *  Markus Stehr <bastetfurry@ircnet.de>
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
#include <stdint.h>

#include "log.h"
#include "snapshot.h"
#include "spi-sdcard.h"
#include "types.h"
#include "util.h"

/* #define TEST_MMC_ALWAYS_NOCARD */
/* #define TEST_MMC_ALWAYS_READONLY */

/*#define SPIDEBUG*/
/*#define DEBUG_MMC*/
/*#define DEBUG_SPI*/

#ifdef SPIDEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

#define MMC_CARD_IDLE          0
#define MMC_CARD_RESET         1
#define MMC_CARD_INIT          2
#define MMC_CARD_READ          3
#define MMC_CARD_DUMMY_READ    4
#define MMC_CARD_WRITE         5
#define MMC_CARD_DUMMY_WRITE   6
#define MMC_CARD_RETURN_WRITE  7

#define MMC_CARD_INSERTED      0
#define MMC_CARD_NOTINSERTED   1

#define MMC_SPIMODE_READ       1
#define MMC_SPIMODE_WRITE      0

#define CARD_TYPE_AUTO         0
#define CARD_TYPE_MMC          1
#define CARD_TYPE_SD           2
#define CARD_TYPE_SDHC         3

typedef uint64_t LWORD;
typedef int64_t SLWORD;
#define HAVE_64BIT_TYPES

#ifdef HAVE_64BIT_TYPES
typedef LWORD sd_addr_t;
#else
typedef DWORD sd_addr_t;
#endif

void spi_mmc_trigger_mode_write(BYTE value);
BYTE spi_mmc_trigger_mode_read(void);

static int mmc_card_type = CARD_TYPE_MMC;
static int mmc_card_rw = 0;

/* Image file */
static FILE *mmc_image_file = NULL;

/* Pointer inside image */
static sd_addr_t mmc_image_pointer;

/* write sequence counter */
static unsigned int mmc_write_sequence;

static BYTE mmc_card_inserted;
static BYTE mmc_card_state;
static BYTE mmc_card_reset_count;

static DWORD mmc_block_size;

/* Gets set when dummy byte is read */
static unsigned int mmc_read_firstbyte;

/* MMC SPI data write port buffering */

/* Command buffer */
static unsigned char mmc_cmd_buffer[9];
static unsigned int mmc_cmd_buffer_pointer;

static void mmc_clear_cmd_buffer(void)
{
    int i;

    for (i = 0; i < 9; i++) {
        mmc_cmd_buffer[i] = 0;
    }
    mmc_cmd_buffer_pointer = 0;
}


/* MMC SPI data read port buffering */
static int mmc_read_buffer_readptr = 0, mmc_read_buffer_writeptr = 0;
BYTE mmc_read_buffer[0x1000];        /* FIXME */

static void mmc_read_buffer_set(BYTE * data, int size)
{
/*LOG(("MMC mmc_read_buffer_set %04x:%02x",mmc_read_buffer_writeptr,size));*/
    while (size) {
        BYTE value;

        value = *data++;
        mmc_read_buffer[mmc_read_buffer_writeptr] = value;
/*LOG(("MMC read byte set %04x:%02x",mmc_read_buffer_writeptr,value));*/
        mmc_read_buffer_writeptr++;
        mmc_read_buffer_writeptr &= 0xfff;
        /*FIXME*/ size--;
    }
}

static BYTE mmc_read_buffer_getbyte(void)
{
    BYTE value = 0;          /* FIXME */

    if (mmc_read_buffer_readptr != mmc_read_buffer_writeptr) {
        value = mmc_read_buffer[mmc_read_buffer_readptr];
#ifdef DEBUG_MMC
        LOG(("MMC read byte get %04x:%02x", mmc_read_buffer_readptr, value));
#endif
        mmc_read_buffer_readptr++;
        mmc_read_buffer_readptr &= 0xfff;
        /*FIXME*/
    } else {
#ifdef DEBUG_MMC
        LOG(("MMC stall read byte get"));
#endif
    }
/*LOG(("MMC mmc_read_buffer_getbyte %04x:%02x",mmc_read_buffer_readptr,value));*/

    return value;
}

/* Resets the card */
static void mmc_reset_card(void)
{
    spi_mmc_trigger_mode_write(0);
    spi_mmc_card_selected_write(0);
    /* mmcreplay_cport = 0; */
    /* mmcreplay_speed = 0; */

    mmc_card_reset_count = 0;
    mmc_image_pointer = 0;
    mmc_block_size = 512;
    mmc_clear_cmd_buffer();
}

/* TODO */
/* 0 = card inserted, 1 = no card inserted (R) */
BYTE spi_mmc_card_inserted(void)
{
    BYTE value = mmc_card_inserted;
#ifdef TEST_MMC_ALWAYS_NOCARD
    value = MMC_CARD_NOTINSERTED;
#endif
    return value;
}

BYTE spi_mmc_set_card_inserted(BYTE value)
{
    BYTE oldvalue = spi_mmc_card_inserted();
    mmc_card_inserted = value;
    return oldvalue;
}

BYTE mmc_set_card_type(BYTE value)
{
    BYTE oldvalue = mmc_card_type;
    mmc_card_type = value;
    return oldvalue;
}

/* TODO */
/* 0 = SPI ready, 1 = SPI busy */
BYTE spi_mmc_busy(void)
{
    BYTE value = 0;
    return value;
}

/* TODO */
/* 0 = card write enabled, 1 = card write disabled (R) */
BYTE spi_mmc_card_write_enabled(void)
{
    BYTE value = mmc_card_rw;
#ifdef TEST_MMC_ALWAYS_READONLY
    value = 1;
#endif
    return value;
}

/* TODO */
static int spi_mmc_card_selected = 0;

BYTE spi_mmc_card_selected_read(void)
{
/*LOG(("MMC spi_mmc_card_selected_read %02x",spi_mmc_card_selected));*/
    return spi_mmc_card_selected;
}

/* TODO */
void spi_mmc_card_selected_write(BYTE value)
{
    spi_mmc_card_selected = value;
/*LOG(("MMC spi_mmc_card_selected_write %02x",spi_mmc_card_selected));*/
}

/* TODO */
BYTE spi_mmc_enable_8mhz_read(void)
{
    BYTE value = 0;
    return value;
}

/* TODO */
void spi_mmc_enable_8mhz_write(BYTE value)
{
}

/* 0 = SPI write trigger mode, 1 = SPI read trigger mode */
static BYTE spi_mmc_trigger_mode = 0;
/* TODO */
BYTE spi_mmc_trigger_mode_read(void)
{
    return spi_mmc_trigger_mode;
}

/* TODO */
void spi_mmc_trigger_mode_write(BYTE value)
{
    spi_mmc_trigger_mode = value;
}


/* TODO */
/* FIXME: wrap strcpy/strcat into macros so they are removed when not debugging */
BYTE spi_mmc_data_read(void)
{
#ifdef DEBUG_SPI
    char logstr[0x100];
#endif

#ifdef DEBUG_SPI
    strcpy(logstr, "SPI: data read ");
#endif

    switch (mmc_card_state) {
        case MMC_CARD_RETURN_WRITE:
            mmc_card_state = MMC_CARD_IDLE;
#ifdef DEBUG_SPI
            LOG(("%s %02x", logstr, 0xff));
#endif
            return 0xff;
            break;
        case MMC_CARD_RESET:
#ifdef DEBUG_SPI
            strcat(logstr, "Card Reset Response! ");
#endif
            switch (mmc_card_reset_count) {
                case 0:
                    mmc_card_reset_count++;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 0");
                    LOG(("%s %02x", logstr, 0x00));
#endif
                    return 0x00;
                    break;
                case 1:
                    mmc_card_reset_count++;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 1");
                    LOG(("%s %02x", logstr, 0x01));
#endif
                    return 0x01;
                    break;
                case 2:
                    mmc_card_reset_count++;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 2");
                    LOG(("%s %02x", logstr, 0x01));
#endif
                    return 0x01;
                    break;
                case 3:
                    mmc_card_reset_count++;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 3");
                    LOG(("%s %02x", logstr, 0x00));
#endif
                    return 0x00;
                    break;
                case 4:
                    mmc_card_reset_count++;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 4");
                    LOG(("%s %02x", logstr, 0x01));
#endif
                    return 0x01;
                    break;
                case 5:
                    mmc_card_reset_count = 0;
#ifdef DEBUG_SPI
                    strcat(logstr, "Reset 5");
                    LOG(("%s %02x", logstr, 0x01));
#endif
                    return 0x01;
                    break;
            }
            break;
        case MMC_CARD_INIT:
#ifdef DEBUG_SPI
            strcat(logstr, "SPI Card Init Response!");
            LOG(("%s %02x", logstr, 0x00));
#endif
            return 0x00;
            break;
        case MMC_CARD_READ:
        case MMC_CARD_DUMMY_READ:
            if (spi_mmc_trigger_mode_read() == MMC_SPIMODE_READ) {
#ifdef DEBUG_SPI
                LOG(("%s Read Trigger Mode %04x", logstr,
                     mmc_read_firstbyte));
#endif
                /* read trigger mode */
                if (mmc_read_firstbyte != mmc_block_size + 5) {
                    mmc_read_firstbyte++;
                }

                if (mmc_read_firstbyte == mmc_block_size + 3) {
#ifdef DEBUG_SPI
                    LOG(("%s Read Trigger %04x %02x", logstr,
                         mmc_read_firstbyte, 0x00));
#endif
                    return 0x00;
                }

                if (mmc_read_firstbyte == mmc_block_size + 4) {
#ifdef DEBUG_SPI
                    LOG(("%s Read Trigger %04x %02x", logstr,
                         mmc_read_firstbyte, 0x01));
#endif
                    return 0x01;
                }

                if (mmc_read_firstbyte == mmc_block_size + 5) {
#ifdef DEBUG_SPI
                    LOG(("%s Read Trigger %04x %02x", logstr,
                         mmc_read_firstbyte, 0x00));
#endif
                    return 0x00;
                }
            } else {
#ifdef DEBUG_SPI
                LOG(("%s Write Trigger Mode %04x", logstr,
                     mmc_read_firstbyte));
#endif
                /* write trigger mode */
                if (mmc_read_firstbyte != mmc_block_size + 2) {
                    mmc_read_firstbyte++;
                }

                if (mmc_read_firstbyte == mmc_block_size + 1) {
#ifdef DEBUG_SPI
                    LOG(("%s %02x", logstr, 0x00));
#endif
                    return 0x00;
                }

                if (mmc_read_firstbyte == mmc_block_size + 2) {
#ifdef DEBUG_SPI
                    LOG(("%s %02x", logstr, 0x01));
#endif
                    return 0x01;
                }
            }

            if (mmc_read_firstbyte == 0) {
#ifdef DEBUG_SPI
                LOG(("%s firstbyte=0 %02x", logstr, 0xff));
#endif
                return 0xFf;
            }

            if (mmc_read_firstbyte == 1) {
#ifdef DEBUG_SPI
                LOG(("%s firstbyte=1 %02x", logstr, 0xfe));
#endif
                return 0xFE;
            }

            if (mmc_read_firstbyte == 2
                && spi_mmc_trigger_mode_read() == MMC_SPIMODE_READ) {
#ifdef DEBUG_SPI
                LOG(("%s firstbyte=2 %02x", logstr, 0xfe));
#endif
                return 0xFE;
            }

            if (!spi_mmc_card_inserted()
                && mmc_card_state != MMC_CARD_DUMMY_READ) {
                BYTE val;
                val = mmc_read_buffer_getbyte();
#ifdef DEBUG_SPI
                LOG(("%s %08x of %04x:", logstr,
                     (mmc_read_buffer_readptr - 1) & 0xfff,
                     mmc_read_buffer_writeptr));
#endif
                return val;
            } else {
#ifdef DEBUG_SPI
                LOG(("%s %02x", logstr, 0x00));
#endif
                return 0x00;
            }
            break;
    }

#ifdef DEBUG_SPI
    LOG(("%s %02x", logstr, 0x00));
#endif
    return 0;
}

/*
CMD0        None(0)             R1  No  GO_IDLE_STATE               Software reset.
CMD1        None(0)             R1  No  SEND_OP_COND                Initiate initialization process.
ACMD41(*1)  *2                  R1  No  APP_SEND_OP_COND            For only SDC. Initiate initialization process.
CMD8        *3                  R7  No  SEND_IF_COND                For only SDC V2. Check voltage range.
CMD9        None(0)             R1  Yes SEND_CSD                    Read CSD register.
CMD10       None(0)             R1  Yes SEND_CID                    Read CID register.
CMD12       None(0)             R1b No  STOP_TRANSMISSION           Stop to read data.
CMD16       Block length[31:0]  R1  No  SET_BLOCKLEN                Change R/W block size.
CMD17       Address[31:0]       R1  Yes READ_SINGLE_BLOCK           Read a block.
CMD18       Address[31:0]       R1  Yes READ_MULTIPLE_BLOCK         Read multiple blocks.
CMD23       No. blocks[15:0]    R1  No  SET_BLOCK_COUNT             For only MMC. Define number of blocks to transfer  with next multi-block read/write command.
ACMD23(*1)  No. blocks[22:0]    R1  No  SET_WR_BLOCK_ERASE_COUNT    For only SDC. Define number of blocks to pre-erase with next multi-block write command.
CMD24       Address[31:0]       R1  Yes WRITE_BLOCK                 Write a block.
CMD25       Address[31:0]       R1  Yes WRITE_MULTIPLE_BLOCK        Write multiple blocks.
CMD55(*1)   None(0)             R1  No  APP_CMD                     Application specific command.
CMD58       None(0)             R3  No  READ_OCR                    Read OCR.

*1:ACMD<n> means a command sequense of CMD55-CMD<n>.
*2: Rsv(0)[31], HCS[30], Rsv(0)[29:0]
*3: Rsv(0)[31:12], Supply Voltage(1)[11:8], Check Pattern(0xAA)[7:0]
*/

static sd_addr_t mmc_get_addr(int *status)
{
    sd_addr_t addr;
    *status = 0;
    if (mmc_card_type == CARD_TYPE_SDHC) {
        /* SDHC (max 2^41) */
#ifdef HAVE_64BIT_TYPES
        addr =
            (mmc_cmd_buffer[5] * 0x100) +
            (mmc_cmd_buffer[4] * 0x10000) +
            (mmc_cmd_buffer[3] * 0x1000000) +
            (mmc_cmd_buffer[2] * 0x100000000);
        addr <<= 1;
#else
        addr =
            (mmc_cmd_buffer[5] * 0x100) +
            (mmc_cmd_buffer[4] * 0x10000) +
            (mmc_cmd_buffer[3] * 0x1000000);
        addr <<= 1;
        if ((mmc_cmd_buffer[2] != 0) || (mmc_cmd_buffer[3] > 0x7f)) {
            *status = 1;
        }
#endif
    } else {
        /* MMC/SD (max 2^32) */
        addr =
            mmc_cmd_buffer[5] +
            (mmc_cmd_buffer[4] * 0x100) +
            (mmc_cmd_buffer[3] * 0x10000) +
            (mmc_cmd_buffer[2] * 0x1000000);
    }
    return addr;
}

/* Executes a command */
static void mmc_execute_cmd(void)
{
    sd_addr_t mmc_current_address_pointer;
    int status;

#ifdef DEBUG_MMC
    log_debug ("Executing CMD %02x %02x %02x %02x %02x %02x %02x %02x %02x",
               mmc_cmd_buffer[0], mmc_cmd_buffer[1],
               mmc_cmd_buffer[2], mmc_cmd_buffer[3],
               mmc_cmd_buffer[4], mmc_cmd_buffer[5],
               mmc_cmd_buffer[6], mmc_cmd_buffer[7],
               mmc_cmd_buffer[8]);
#endif
    switch (mmc_cmd_buffer[1]) {
        case 0xff:
#ifdef DEBUG_MMC
            log_debug("Hard reset received");
#endif
            mmc_card_state = MMC_CARD_IDLE;
            break;
        case 0x40:             /* CMD00 Reset */
#ifdef DEBUG_MMC
            log_debug("CMD00 Reset received");
#endif
            mmc_reset_card();
            mmc_card_state = MMC_CARD_RESET;
            break;
        case 0x41:             /* CMD01 Init */
#ifdef DEBUG_MMC
            log_debug("CMD01 Init received");
#endif
            mmc_card_state = MMC_CARD_INIT;
            break;
        case 0x48:             /* CMD8 ? */
#ifdef DEBUG_MMC
            log_debug("CMD8 ? received");
#endif
            if (mmc_card_type == CARD_TYPE_MMC) {
                /* MMC */
                BYTE cmdresp[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(cmdresp, 0x200);
            } else {
                /* SD v2 */
                BYTE cmdresp[0x10] = { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 1;
                mmc_read_buffer_set(cmdresp, 0x200);
            }
            break;
        case 0x49:             /* CMD9 send CSD */
#ifdef DEBUG_MMC
            log_debug("CMD9 send CSD received");
#endif
            if (!spi_mmc_card_inserted()) {
                BYTE csdresp[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(csdresp, 0x200);
            } else {
                mmc_card_state = MMC_CARD_DUMMY_READ;
                mmc_read_firstbyte = 0;
            }
            break;
        case 0x4a:             /* CMD9 send CID */
#ifdef DEBUG_MMC
            log_debug("CMD10 send CID received");
#endif
            if (!spi_mmc_card_inserted()) {
                BYTE cidresp[0x10] =
                    { 0, 0, 0, 0,
                    1+'v'-'a', 1+'i'-'a', 1+'c'-'a', 1+'e'-'a', '2', '3', /* "viceemu" */
                    0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;

                mmc_read_buffer_readptr = 0;
                mmc_read_buffer_writeptr = 0;
                mmc_read_buffer_set(cidresp, 0x10);
            } else {
                mmc_card_state = MMC_CARD_DUMMY_READ;
                mmc_read_firstbyte = 0;
            }
            break;
        case 0x4c:             /* CMD12 Stop */
#ifdef DEBUG_MMC
            log_debug("CMD12 Stop received");
#endif
            mmc_card_state = MMC_CARD_IDLE;
            break;
        case 0x50:             /* CMD16 Set Block Size */
#ifdef DEBUG_MMC
            log_debug("CMD16-AAAA Set Block Size received");
#endif
            mmc_card_state = MMC_CARD_IDLE;
            mmc_block_size =
                mmc_cmd_buffer[5] +
                (mmc_cmd_buffer[4] * 0x100) +
                (mmc_cmd_buffer[3] * 0x10000) +
                (mmc_cmd_buffer[2] * 0x1000000);
            break;
        case 0x51:
#ifdef DEBUG_MMC
            log_debug("CMD17-AAAA Block Read received");
#endif
            if (!spi_mmc_card_inserted()) {
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;

                mmc_current_address_pointer = mmc_get_addr(&status);
                if (status) {
#ifdef DEBUG_MMC
                    log_debug("Address Overflow: %08x", mmc_current_address_pointer);
#endif
                    mmc_card_state = MMC_CARD_DUMMY_READ;
                } else {
#ifdef DEBUG_MMC
                    log_debug("Address: %08x", mmc_current_address_pointer);
#endif
                    if (fseek(mmc_image_file, mmc_current_address_pointer, SEEK_SET) != 0) {
                        mmc_card_state = MMC_CARD_DUMMY_READ;
                    } else {
                        BYTE readbuf[0x1000];    /* FIXME */
#ifdef DEBUG_MMC
                        log_debug("Buffering: %08x", mmc_current_address_pointer);
#endif
                        fseek(mmc_image_file, mmc_current_address_pointer, SEEK_SET);
                        if (!feof(mmc_image_file)) {
                            if (fread(readbuf, 1, mmc_block_size, mmc_image_file) > 0) {
                                mmc_read_buffer_readptr = 0;
                                mmc_read_buffer_writeptr = 0;
                                mmc_read_buffer_set(readbuf, mmc_block_size);
#ifdef DEBUG_MMC
                                log_debug("Buffered: %02x %02x", readbuf[0], readbuf[1]);
#endif
                            } else {
                                /* FIXME: handle error */
                            }
                        }
                    }
                }
            } else {
                mmc_card_state = MMC_CARD_DUMMY_READ;
                mmc_read_firstbyte = 0;
            }
            break;
        case 0x58:
/*log_debug("CMD Block Write received");*/
            if (!spi_mmc_card_inserted() && mmc_block_size > 0) {
#ifdef DEBUG_MMC
                LOG(("MMC CMD Block Write Address: %08x",
                     mmc_current_address_pointer));
#endif
                mmc_current_address_pointer = mmc_get_addr(&status);
                if (status) {
                    mmc_write_sequence = 0;
                    mmc_card_state = MMC_CARD_DUMMY_WRITE;
#ifdef DEBUG_MMC
                    log_debug("Address Overflow: %08x", mmc_current_address_pointer);
#endif
                } else {
                    mmc_write_sequence = 0;
                    mmc_card_state = MMC_CARD_WRITE;
                }
            } else {
                mmc_write_sequence = 0;
                mmc_card_state = MMC_CARD_DUMMY_WRITE;
            }
            break;
        case 0x69:             /* ACMD41 ? */
            {
                BYTE cmdresp[0x10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#ifdef DEBUG_MMC
                log_debug("ACMD41 ? received");
#endif
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(cmdresp, 0x200);
            }
            break;
        case 0x77:             /* CMD77 ? */
#ifdef DEBUG_MMC
            log_debug("CMD77 ? received");
#endif
            if (mmc_card_type != CARD_TYPE_MMC) {
                /* SD v2 only */
                BYTE cmdresp[0x10] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(cmdresp, 0x200);
            }
            break;
        case 0x7a:             /* CMD58 ? */
#ifdef DEBUG_MMC
            log_debug("CMD58 ? received");
#endif
            if (mmc_card_type == CARD_TYPE_SDHC) {
                /* SDHC */
                BYTE cmdresp[0x10] = { 0, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(cmdresp, 0x200);
            } else {
                /* SD */
                BYTE cmdresp[0x10] = { 0, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                mmc_card_state = MMC_CARD_READ;
                mmc_read_firstbyte = 0;
                mmc_read_buffer_set(cmdresp, 0x200);
            }
            break;
    }
}

static void mmc_write_to_cmd_buffer(unsigned char mmcreplay_cmd_char)
{
    /* Check for 0xff sync byte */
    if (mmc_cmd_buffer_pointer == 0) {
        if (mmcreplay_cmd_char < 0xff) {
/*LOG(("SPI: NOT write to buffer 1: %04x %02x",mmc_cmd_buffer_pointer,mmcreplay_cmd_char)); */
            /* yuck! hack! hack1 */
            if (mmcreplay_cmd_char == 0x51) {
                mmc_cmd_buffer[0] = 0xff;
                mmc_cmd_buffer_pointer++;
            } else {
                return;
            }
        }
    }

    /* Check for one 0xff sync byte too much */
    if (mmc_cmd_buffer_pointer == 1) {
        if (mmcreplay_cmd_char == 0xff) {
            mmc_cmd_buffer_pointer = 0;
            return;
        }
    }

/*LOG(("SPI: write to buffer: %04x %02x",mmc_cmd_buffer_pointer,mmcreplay_cmd_char));*/
    /* Write byte to buffer */
    mmc_cmd_buffer[mmc_cmd_buffer_pointer] = mmcreplay_cmd_char;
    mmc_cmd_buffer_pointer++;

#if 0
    LOG(("SPI: buffer: %02x %02x %02x %02x %02x %02x %02x %02x",
         mmc_cmd_buffer[0],
         mmc_cmd_buffer[1],
         mmc_cmd_buffer[2],
         mmc_cmd_buffer[3],
         mmc_cmd_buffer[4],
         mmc_cmd_buffer[5],
         mmc_cmd_buffer[6],
         mmc_cmd_buffer[7],
         mmc_cmd_buffer[8],
         mmc_cmd_buffer[9]
         ));
#endif

    /* If the buffer is full, execute the buffer and clear it */
    if ((mmc_cmd_buffer_pointer > 9) ||
/*    if ((mmc_cmd_buffer_pointer > 8) || */
        (mmc_cmd_buffer_pointer > 7 && mmc_cmd_buffer[1] == 0x40) ||  /* cmd0 */
        (mmc_cmd_buffer_pointer > 8 && mmc_cmd_buffer[1] == 0x48) ||  /* cmd8 */
        (mmc_cmd_buffer_pointer > 8 && mmc_cmd_buffer[1] == 0x49) ||  /* cmd9 */
        (mmc_cmd_buffer_pointer > 8 && mmc_cmd_buffer[1] == 0x4a) ||  /* cmd10 */
        (mmc_cmd_buffer_pointer > 8 && mmc_cmd_buffer[1] == 0x50)   /* cmd16 */
/*      ||  (mmc_cmd_buffer_pointer > 8 && mmc_cmd_buffer[1] == 0x51) */    /* cmd17 */
        ) {
        mmc_execute_cmd();
        mmc_clear_cmd_buffer();
    }
}

static void mmc_write_to_mmc(BYTE value)
{
    switch (mmc_write_sequence) {
        case 0:
            if (value == 0xfe) {
                mmc_write_sequence++;
                mmc_image_pointer = 0;
            }
            break;
        case 1:
            if (mmc_card_state == MMC_CARD_WRITE) {
                if (fwrite(&value, 1, 1, mmc_image_file) != 1) {
                    LOG(("could not write to mmc image file"));
                    /* FIXME: handle error */
                }
            }
            mmc_image_pointer++;
            if (mmc_image_pointer == mmc_block_size) {
                mmc_write_sequence++;
            }
            break;
        case 2:
            mmc_write_sequence++;
            break;
        case 3:
            mmc_card_state = MMC_CARD_RETURN_WRITE;
            break;
    }
}

/* TODO */
void spi_mmc_data_write(BYTE value)
{
    if (mmc_card_state == MMC_CARD_WRITE
        || mmc_card_state == MMC_CARD_DUMMY_WRITE) {
/*LOG(("spi data write mmc: %02x",value));*/
        mmc_write_to_mmc(value);
    } else {
/*LOG(("spi data write cmd: %02x",value));*/
        mmc_write_to_cmd_buffer(value);
    }
}

int mmc_open_card_image(char *name, int rw)
{
    char *mmc_image_filename = name;

    spi_mmc_set_card_inserted(MMC_CARD_NOTINSERTED);

    if (mmc_image_filename == NULL) {
        LOG(("sd card image name not set"));
        return 1;
    }

    if (mmc_image_file != NULL) {
        mmc_close_card_image();
    }

    if (rw) {
        mmc_image_file = fopen(mmc_image_filename, "rb+");
    }

    if (mmc_image_file == NULL) {
        mmc_image_file = fopen(mmc_image_filename, "rb");

        if (mmc_image_file == NULL) {
            LOG(("could not open sd card image: %s", mmc_image_filename));
            return 1;
        } else {
            /* FIXME */
            spi_mmc_set_card_inserted(MMC_CARD_INSERTED);
            LOG(("opened sd card image (ro): %s", mmc_image_filename));
            /* mmc_image_file_readonly = 1; */
            /* mmcreplay_hw_writeprotect = 1; */
            /* mmcreplay_writeprotect = MMC_WRITEPROT; */
        }
    } else {
        /* mmc_image_file_readonly = 0; */
        spi_mmc_set_card_inserted(MMC_CARD_INSERTED);
        LOG(("opened sd card image (rw): %s", mmc_image_filename));
    }
    mmc_card_rw = rw;
    return 0;
}

void mmc_close_card_image(void)
{
    /* unmount mmc cart image */
    if (mmc_image_file != NULL) {
        fclose(mmc_image_file);
        mmc_image_file = NULL;
        spi_mmc_set_card_inserted(MMC_CARD_NOTINSERTED);
    }
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "SDCARD"

/* FIXME: implement snapshot support */
int mmc_snapshot_write_module(snapshot_t *s)
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

int mmc_snapshot_read_module(snapshot_t *s)
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
