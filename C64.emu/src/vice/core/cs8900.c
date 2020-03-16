/*
 * cs8900.c - CS8900 Ethernet Core
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifdef HAVE_RAWNET

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DOS_TFE
#include <pcap.h>       /* FIXME: remove? */
#endif

#include "archdep.h"
#include "cs8900.h"
#include "crc32.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "rawnetarch.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

/* FIXME:
    - add register dump
*/

/* warn illegal behaviour */
/* #define CS8900_DEBUG_WARN_REG 1 */   /* warn about invalid register accesses */
/* #define CS8900_DEBUG_WARN_RXTX 1 */  /* warn about invalid rx or tx conditions */

/** #define CS8900_DEBUG_INIT 1  **/
/** #define CS8900_DEBUG_LOAD 1  **/          /* enable to see tfe port reads */
/** #define CS8900_DEBUG_STORE 1 **/          /* enable to see tfe port writes */
/** #define CS8900_DEBUG_REGISTERS 1 **/      /* enable to see CS8900a register I/O */
/** #define CS8900_DEBUG_IGNORE_RXEVENT 1 **/ /* enable to ignore RXEVENT in DEBUG_REGISTERS */
/** #define CS8900_DEBUG_RXTX_STATE 1 **/     /* enable to see tranceiver state changes */
/** #define CS8900_DEBUG_RXTX_DATA 1 **/      /* enable to see data in/out flow */
/** #define RAWNET_DEBUG_FRAMES 1 **/         /* enable to see arch frame send/recv - might be defined in rawnetarch.h ! */

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

static log_t cs8900_log = LOG_ERR;

/* status which received packages to accept
   This is used in cs8900_should_accept().
*/
static uint8_t cs8900_ia_mac[6] = { 0, 0, 0, 0, 0, 0 };

/* remember the value of the hash mask */
static uint32_t cs8900_hash_mask[2];

/* reveiver setup */
static uint16_t cs8900_recv_control     = 0; /* copy of CC_RXCTL (contains all bits below) */
static int  cs8900_recv_broadcast   = 0; /* broadcast */
static int  cs8900_recv_mac         = 0; /* individual address (IA) */
static int  cs8900_recv_multicast   = 0; /* multicast if address passes the hash filter */
static int  cs8900_recv_correct     = 0; /* accept correct frames */
static int  cs8900_recv_promiscuous = 0; /* promiscuous mode */
static int  cs8900_recv_hashfilter  = 0; /* accept if IA passes the hash filter */

/* TFE registers */
/* these are the 8 16-bit-ports for "I/O space configuration"
   (see 4.10 on page 75 of cs8900a-4.pdf, the cs8900a data sheet)

   REMARK: The TFE operates the cs8900a in IO space configuration, as
   it generates I/OW and I/OR signals.
*/
#define CS8900_COUNT_IO_REGISTER 0x10 /* we have 16 I/O register */

static uint8_t *cs8900 = NULL;
/*
    RW: RXTXDATA   = DE00/DE01
    RW: RXTXDATA2  = DE02/DE03 (for 32-bit-operation)
    -W: TXCMD      = DE04/DE05 (TxCMD, Transmit Command)   mapped to PP + 0144 (Reg. 9, Sec. 4.4, page 46)
    -W: TXLENGTH   = DE06/DE07 (TxLenght, Transmit Length) mapped to PP + 0146
    R-: INTSTQUEUE = DE08/DE09 (Interrupt Status Queue)    mapped to PP + 0120 (ISQ, Sec. 5.1, page 78)
    RW: PP_PTR     = DE0A/DE0B (PacketPage Pointer)        (see. page 75p: Read -011.---- ----.----)
    RW: PP_DATA0   = DE0C/DE0D (PacketPage Data (Port 0))
    RW: PP_DATA1   = DE0E/DE0F (PacketPage Data (Port 1))  (for 32 bit only)
*/

#define CS8900_ADDR_RXTXDATA   0x00 /* RW */
#define CS8900_ADDR_RXTXDATA2  0x02 /* RW 32 bit only! */
#define CS8900_ADDR_TXCMD      0x04 /* -W Maps to PP+0144 */
#define CS8900_ADDR_TXLENGTH   0x06 /* -W Maps to PP+0146 */
#define CS8900_ADDR_INTSTQUEUE 0x08 /* R- Interrupt status queue, maps to PP + 0120 */
#define CS8900_ADDR_PP_PTR     0x0a /* RW PacketPage Pointer */
#define CS8900_ADDR_PP_DATA    0x0c /* RW PacketPage Data, Port 0 */
#define CS8900_ADDR_PP_DATA2   0x0e /* RW PacketPage Data, Port 1 - 32 bit only */

/* Makros for reading and writing the visible TFE register: */
#define GET_CS8900_8(_xxx_) (assert(_xxx_ < CS8900_COUNT_IO_REGISTER), cs8900[_xxx_])

#define SET_CS8900_8(_xxx_, _val_)                \
    do {                                          \
        assert(_xxx_ < CS8900_COUNT_IO_REGISTER); \
        cs8900[_xxx_] = (_val_) & 0xff;           \
    } while (0)

#define GET_CS8900_16(_xxx_) (assert(_xxx_ < CS8900_COUNT_IO_REGISTER), cs8900[_xxx_] | (cs8900[_xxx_ + 1] << 8))

#define SET_CS8900_16(_xxx_, _val_)               \
    do {                                          \
        assert(_xxx_ < CS8900_COUNT_IO_REGISTER); \
        cs8900[_xxx_] = (_val_) & 0xff;           \
        cs8900[_xxx_ + 1] = (_val_ >> 8) & 0xff;     \
    } while (0)

/* The PacketPage register */
/* note: The locations 0 to MAX_PACKETPAGE_ARRAY-1 are handled in this array. */

#define MAX_PACKETPAGE_ARRAY 0x1000 /* 4 KB */

static uint8_t *cs8900_packetpage = NULL;

static uint16_t cs8900_packetpage_ptr = 0;

/* Makros for reading and writing the PacketPage register: */

#define GET_PP_8(_xxx_) (assert(_xxx_ < MAX_PACKETPAGE_ARRAY), cs8900_packetpage[_xxx_])

#define GET_PP_16(_xxx_) (assert(_xxx_ < MAX_PACKETPAGE_ARRAY), assert((_xxx_ & 1) == 0), ((uint16_t)cs8900_packetpage[_xxx_]) | ((uint16_t)cs8900_packetpage[_xxx_ + 1] << 8))

#define GET_PP_32(_xxx_)                                             \
    (assert(_xxx_ < MAX_PACKETPAGE_ARRAY), assert((_xxx_ & 3) == 0), \
     (((long)cs8900_packetpage[_xxx_])) | (((long)cs8900_packetpage[_xxx_ + 1]) << 8) | (((long)cs8900_packetpage[_xxx_ + 2]) << 16) | (((long)cs8900_packetpage[_xxx_ + 3]) << 24))

#define SET_PP_8(_xxx_, _val_)                  \
    do {                                        \
        assert(_xxx_ < MAX_PACKETPAGE_ARRAY);   \
        cs8900_packetpage[_xxx_] = (_val_) & 0xFF; \
    } while (0)

#define SET_PP_16(_xxx_, _val_)                          \
    do {                                                 \
        assert(_xxx_ < MAX_PACKETPAGE_ARRAY);            \
        assert((_xxx_ & 1) == 0),                        \
        cs8900_packetpage[_xxx_] = (_val_) & 0xFF;          \
        cs8900_packetpage[_xxx_ + 1] = (_val_ >> 8) & 0xFF; \
    } while (0)

#define SET_PP_32(_xxx_, _val_)                           \
    do {                                                  \
        assert(_xxx_ < MAX_PACKETPAGE_ARRAY);             \
        assert((_xxx_ & 3) == 0),                         \
        cs8900_packetpage[_xxx_] = (_val_) & 0xFF;           \
        cs8900_packetpage[_xxx_ + 1] = (_val_ >> 8) & 0xFF;  \
        cs8900_packetpage[_xxx_ + 2] = (_val_ >> 16) & 0xFF; \
        cs8900_packetpage[_xxx_ + 3] = (_val_ >> 24) & 0xFF; \
    } while (0)

/* The packetpage register: see p. 39f */
#define CS8900_PP_ADDR_PRODUCTID       0x0000 /*   R- - 4.3., p. 41 */
#define CS8900_PP_ADDR_IOBASE          0x0020 /* i RW - 4.3., p. 41 - 4.7., p. 72 */
#define CS8900_PP_ADDR_INTNO           0x0022 /* i RW - 3.2., p. 17 - 4.3., p. 41 */
#define CS8900_PP_ADDR_DMA_CHAN        0x0024 /* i RW - 3.2., p. 17 - 4.3., p. 41 */
#define CS8900_PP_ADDR_DMA_SOF         0x0026 /* ? R- - 4.3., p. 41 - 5.4., p. 89 */
#define CS8900_PP_ADDR_DMA_FC          0x0028 /* ? R- - 4.3., p. 41, "Receive DMA" */
#define CS8900_PP_ADDR_RXDMA_BC        0x002a /* ? R- - 4.3., p. 41 - 5.4., p. 89 */
#define CS8900_PP_ADDR_MEMBASE         0x002c /* i RW - 4.3., p. 41 - 4.9., p. 73 */
#define CS8900_PP_ADDR_BPROM_BASE      0x0030 /* i RW - 3.6., p. 24 - 4.3., p. 41 */
#define CS8900_PP_ADDR_BPROM_MASK      0x0034 /* i RW - 3.6., p. 24 - 4.3., p. 41 */

/* 0x0038 - 0x003F: reserved */
#define CS8900_PP_ADDR_EEPROM_CMD      0x0040 /* i RW - 3.5., p. 23 - 4.3., p. 41 */
#define CS8900_PP_ADDR_EEPROM_DATA     0x0042 /* i RW - 3.5., p. 23 - 4.3., p. 41 */

/* 0x0044 - 0x004F: reserved */
#define CS8900_PP_ADDR_REC_FRAME_BC    0x0050 /*   RW - 4.3., p. 41 - 5.2.9., p. 86 */

/* 0x0052 - 0x00FF: reserved */
#define CS8900_PP_ADDR_CONF_CTRL       0x0100 /* - RW - 4.4., p. 46; see below */
#define CS8900_PP_ADDR_STATUS_EVENT    0x0120 /* - R- - 4.4., p. 46; see below */

/* 0x0140 - 0x0143: reserved */
#define CS8900_PP_ADDR_TXCMD           0x0144 /* # -W - 4.5., p. 70 - 5.7., p. 98 */
#define CS8900_PP_ADDR_TXLENGTH        0x0146 /* # -W - 4.5., p. 70 - 5.7., p. 98 */

/* 0x0148 - 0x014F: reserved */
#define CS8900_PP_ADDR_LOG_ADDR_FILTER 0x0150 /*   RW - 4.6., p. 71 - 5.3., p. 86 */
#define CS8900_PP_ADDR_MAC_ADDR        0x0158 /* # RW - 4.6., p. 71 - 5.3., p. 86 */

/* 0x015E - 0x03FF: reserved */
#define CS8900_PP_ADDR_RXSTATUS        0x0400 /*   R- - 4.7., p. 72 - 5.2., p. 78 */
#define CS8900_PP_ADDR_RXLENGTH        0x0402 /*   R- - 4.7., p. 72 - 5.2., p. 78 */
#define CS8900_PP_ADDR_RX_FRAMELOC     0x0404 /*   R- - 4.7., p. 72 - 5.2., p. 78 */

/* here, the received frame is stored */
#define CS8900_PP_ADDR_TX_FRAMELOC     0x0A00 /*   -W - 4.7., p. 72 - 5.7., p. 98 */

/* here, the frame to transmit is stored */
#define CS8900_PP_ADDR_END             0x1000 /* memory to TFE_PP_ADDR_END-1 is used */

/* TFE_PP_ADDR_CONF_CTRL is subdivided: */
#define CS8900_PP_ADDR_CC_RXCFG        0x0102 /* # RW - 4.4.6.,  p. 52 - 0003 */
#define CS8900_PP_ADDR_CC_RXCTL        0x0104 /* # RW - 4.4.8.,  p. 54 - 0005 */
#define CS8900_PP_ADDR_CC_TXCFG        0x0106 /*   RW - 4.4.9.,  p. 55 - 0007 */
#define CS8900_PP_ADDR_CC_TXCMD        0x0108 /*   R- - 4.4.11., p. 57 - 0009 */
#define CS8900_PP_ADDR_CC_BUFCFG       0x010A /*   RW - 4.4.12., p. 58 - 000B */
#define CS8900_PP_ADDR_CC_LINECTL      0x0112 /* # RW - 4.4.16., p. 62 - 0013 */
#define CS8900_PP_ADDR_CC_SELFCTL      0x0114 /*   RW - 4.4.18., p. 64 - 0015 */
#define CS8900_PP_ADDR_CC_BUSCTL       0x0116 /*   RW - 4.4.20., p. 66 - 0017 */
#define CS8900_PP_ADDR_CC_TESTCTL      0x0118 /*   RW - 4.4.22., p. 68 - 0019 */

/* CS8900_PP_ADDR_STATUS_EVENT is subdivided: */
#define CS8900_PP_ADDR_SE_ISQ          0x0120 /*   R- - 4.4.5.,  p. 51 - 0000 */
#define CS8900_PP_ADDR_SE_RXEVENT      0x0124 /* # R- - 4.4.7.,  p. 53 - 0004 */
#define CS8900_PP_ADDR_SE_TXEVENT      0x0128 /*   R- - 4.4.10., p. 56 - 0008 */
#define CS8900_PP_ADDR_SE_BUFEVENT     0x012C /*   R- - 4.4.13., p. 59 - 000C */
#define CS8900_PP_ADDR_SE_RXMISS       0x0130 /*   R- - 4.4.14., p. 60 - 0010 */
#define CS8900_PP_ADDR_SE_TXCOL        0x0132 /*   R- - 4.4.15., p. 61 - 0012 */
#define CS8900_PP_ADDR_SE_LINEST       0x0134 /*   R- - 4.4.17., p. 63 - 0014 */
#define CS8900_PP_ADDR_SE_SELFST       0x0136 /*   R- - 4.4.19., p. 65 - 0016 */
#define CS8900_PP_ADDR_SE_BUSST        0x0138 /* # R- - 4.4.21., p. 67 - 0018 */
#define CS8900_PP_ADDR_SE_TDR          0x013C /*   R- - 4.4.23., p. 69 - 001C */

/* ------------------------------------------------------------------------- */
/*    more variables needed                                                  */

static uint16_t tx_buffer = CS8900_PP_ADDR_TX_FRAMELOC;
static uint16_t rx_buffer = CS8900_PP_ADDR_RXSTATUS;

static uint16_t tx_count = 0;
static uint16_t rx_count = 0;
static uint16_t tx_length = 0;
static uint16_t rx_length = 0;

#define CS8900_TX_IDLE       0
#define CS8900_TX_GOT_CMD    1
#define CS8900_TX_GOT_LEN    2
#define CS8900_TX_READ_BUSST 3

#define CS8900_RX_IDLE      0
#define CS8900_RX_GOT_FRAME 1

/* tranceiver state */
static int tx_state = CS8900_TX_IDLE;
static int rx_state = CS8900_RX_IDLE;
static int tx_enabled = 0;
static int rx_enabled = 0;

static int rxevent_read_mask = 3; /* set if L and/or H byte was read in RXEVENT? */

/* ------------------------------------------------------------------------- */
/*    some parameter definitions                                             */

#define MAX_TXLENGTH 1518
#define MIN_TXLENGTH 4

#define MAX_RXLENGTH 1518
#define MIN_RXLENGTH 64

/* ------------------------------------------------------------------------- */
/*    debugging functions                                                    */

#ifdef RAWNET_DEBUG_FRAMES

#define MAXLEN_DEBUG 1600

static int cs8900DebugMaxFrameLengthToDump = 150;

static char *debug_outbuffer(const int length, const unsigned char * const buffer)
{
    int i;
    static char outbuffer[MAXLEN_DEBUG * 4 + 1];
    char *p = outbuffer;

    assert(cs8900DebugMaxFrameLengthToDump <= MAXLEN_DEBUG);

    *p = 0;

    for (i = 0; i < cs8900DebugMaxFrameLengthToDump; i++) {
        if (i >= length) {
            break;
        }
        sprintf( p, "%02X%c", buffer[i], ((i + 1) % 16 == 0) ? '*' : (((i + 1) % 8 == 0) ? '-' : ' '));
        p += 3;
    }

    return outbuffer;
}
#endif

/* ------------------------------------------------------------------------- */
/*    initialization and deinitialization functions                          */

static void cs8900_set_tx_status(int ready, int error)
{
    uint16_t old_status = GET_PP_16(CS8900_PP_ADDR_SE_BUSST);

    /* mask out TxBidErr and Rdy4TxNOW */
    uint16_t new_status = old_status & ~0x180;
    if (ready) {
        new_status |= 0x100; /* set Rdy4TxNOW */
    }
    if (error) {
        new_status |= 0x080; /* set TxBidErr */
    }

    if (new_status != old_status) {
        SET_PP_16(CS8900_PP_ADDR_SE_BUSST, new_status);
#ifdef CS8900_DEBUG_RXTX_STATE
        log_message(cs8900_log, "TX: set status Rdy4TxNOW=%d TxBidErr=%d", ready, error);
#endif
    }
}

static void cs8900_set_receiver(int enabled)
{
    rx_enabled = enabled;
    rx_state = CS8900_RX_IDLE;

    rxevent_read_mask = 3; /* was L or H byte read in RXEVENT? */
}

static void cs8900_set_transmitter(int enabled)
{
    tx_enabled = enabled;
    tx_state = CS8900_TX_IDLE;

    cs8900_set_tx_status(0, 0);
}

void cs8900_reset(void)
{
    int i;

    assert(cs8900);
    assert(cs8900_packetpage);

    rawnet_arch_pre_reset();

    /* initialize visible IO register and PacketPage registers */
    memset(cs8900, 0, CS8900_COUNT_IO_REGISTER);
    memset(cs8900_packetpage, 0, MAX_PACKETPAGE_ARRAY);

    /* according to page 19 unless stated otherwise */
    SET_PP_32(CS8900_PP_ADDR_PRODUCTID, 0x0900630E ); /* p.41: 0E630009 for Rev. D; reversed order! */
    SET_PP_16(CS8900_PP_ADDR_IOBASE, 0x0300);
    SET_PP_16(CS8900_PP_ADDR_INTNO, 0x0004); /* xxxx xxxx xxxx x100b */
    SET_PP_16(CS8900_PP_ADDR_DMA_CHAN, 0x0003); /* xxxx xxxx xxxx xx11b */

    /* according to descriptions of the registers, see definitions of
    CS8900_PP_ADDR_CC_... and CS8900_PP_ADDR_SE_... above! */

    SET_PP_16(CS8900_PP_ADDR_CC_RXCFG, 0x0003);
    SET_PP_16(CS8900_PP_ADDR_CC_RXCTL, 0x0005);
    SET_PP_16(CS8900_PP_ADDR_CC_TXCFG, 0x0007);
    SET_PP_16(CS8900_PP_ADDR_CC_TXCMD, 0x0009);
    SET_PP_16(CS8900_PP_ADDR_CC_BUFCFG, 0x000B);
    SET_PP_16(CS8900_PP_ADDR_CC_LINECTL, 0x0013);
    SET_PP_16(CS8900_PP_ADDR_CC_SELFCTL, 0x0015);
    SET_PP_16(CS8900_PP_ADDR_CC_BUSCTL, 0x0017);
    SET_PP_16(CS8900_PP_ADDR_CC_TESTCTL, 0x0019);

    SET_PP_16(CS8900_PP_ADDR_SE_ISQ, 0x0000);
    SET_PP_16(CS8900_PP_ADDR_SE_RXEVENT, 0x0004);
    SET_PP_16(CS8900_PP_ADDR_SE_TXEVENT, 0x0008);
    SET_PP_16(CS8900_PP_ADDR_SE_BUFEVENT, 0x000C);
    SET_PP_16(CS8900_PP_ADDR_SE_RXMISS, 0x0010);
    SET_PP_16(CS8900_PP_ADDR_SE_TXCOL, 0x0012);
    /* according to specs the reset value is 0x0014, however we also set
       bit  7 - Link OK
       bit  9 - 10Base-T
       bit 12 - Polarity OK
       which makes 0x1294 ... because some software might check these and
       expects them to be "Up".
       FIXME: we should perhaps maintain these bits elsewhere
    */
    SET_PP_16(CS8900_PP_ADDR_SE_LINEST, 0x1294);
    SET_PP_16(CS8900_PP_ADDR_SE_SELFST, 0x0016);
    SET_PP_16(CS8900_PP_ADDR_SE_BUSST, 0x0018);
    SET_PP_16(CS8900_PP_ADDR_SE_TDR, 0x001C);

    SET_PP_16(CS8900_PP_ADDR_TXCMD, 0x0009);

    /* 4.4.19 Self Status Register, p. 65
        Important: set INITD (Bit 7) to signal device is ready */
    SET_PP_16(CS8900_PP_ADDR_SE_SELFST, 0x0896);

    cs8900_recv_control = GET_PP_16(CS8900_PP_ADDR_CC_RXCTL);

    /* spec: mac address is undefined after reset.
        real HW: keeps the last set address. */
    for (i = 0; i < 6; i++) {
        SET_PP_8(CS8900_PP_ADDR_MAC_ADDR + i, cs8900_ia_mac[i]);
    }

    /* reset state */
    cs8900_set_transmitter(0);
    cs8900_set_receiver(0);

    rawnet_arch_post_reset();

    log_message(cs8900_log, "CS8900a rev.D reset");
}

int cs8900_activate(const char *net_interface)
{
    assert(cs8900 == NULL);
    assert(cs8900_packetpage == NULL);

#ifdef CS8900_DEBUG
    log_message(cs8900_log, "cs8900_activate().");
#endif

    /* allocate memory for visible IO register */
    cs8900 = lib_malloc(CS8900_COUNT_IO_REGISTER);
    if (cs8900 == NULL) {
#ifdef CS8900_DEBUG_INIT
        log_message(cs8900_log, "cs8900_activate: Allocating cs8900 failed.");
#endif
        return -1;
    }

    /* allocate memory for PacketPage register */
    cs8900_packetpage = lib_malloc(MAX_PACKETPAGE_ARRAY);
    if (cs8900_packetpage == NULL) {
#ifdef CS8900_DEBUG_INIT
        log_message(cs8900_log, "cs8900_activate: Allocating cs8900_packetpage failed.");
#endif
        lib_free(cs8900);
        cs8900 = NULL;
        return -1;
    }

#ifdef CS8900_DEBUG_INIT
    log_message(cs8900_log, "cs8900_activate: Allocated memory successfully.");
    log_message(cs8900_log, "\tcs8900 at $%08X, cs8900_packetpage at $%08X", cs8900, cs8900_packetpage);
#endif

    if (!rawnet_arch_activate(net_interface)) {
        lib_free(cs8900_packetpage);
        lib_free(cs8900);
        cs8900 = NULL;
        cs8900_packetpage = NULL;
        return -2;
    }

    /* virtually reset the LAN chip */
    cs8900_reset();
    return 0;
}

int cs8900_deactivate(void)
{
#ifdef CS8900_DEBUG
    log_message(cs8900_log, "cs8900_deactivate().");
#endif

    assert(cs8900 && cs8900_packetpage);

    rawnet_arch_deactivate();

    lib_free(cs8900);
    cs8900 = NULL;
    lib_free(cs8900_packetpage);
    cs8900_packetpage = NULL;
    return 0;
}

void cs8900_shutdown(void)
{
#ifdef CS8900_DEBUG
    log_message(cs8900_log, "cs8900_shutdown().");
#endif

    assert((cs8900 && cs8900_packetpage) || (!cs8900 && !cs8900_packetpage));

    if (cs8900) {
#ifdef CS8900_DEBUG
        log_message(cs8900_log, "...1");
#endif
        cs8900_deactivate();
    }

#ifdef CS8900_DEBUG
    log_message(cs8900_log, "cs8900_shutdown() done.");
#endif
}

int cs8900_init(void)
{
    cs8900_log = log_open("CS8900");
    if (!rawnet_arch_init()) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/*    reading and writing CS8900 register functions                             */

/*
These registers are currently fully or partially supported:

CS8900_PP_ADDR_CC_RXCFG        0x0102 * # RW - 4.4.6.,  p. 52 - 0003 *
CS8900_PP_ADDR_CC_RXCTL        0x0104 * # RW - 4.4.8.,  p. 54 - 0005 *
CS8900_PP_ADDR_CC_LINECTL      0x0112 * # RW - 4.4.16., p. 62 - 0013 *
CS8900_PP_ADDR_SE_RXEVENT      0x0124 * # R- - 4.4.7.,  p. 53 - 0004 *
CS8900_PP_ADDR_SE_BUSST        0x0138 * # R- - 4.4.21., p. 67 - 0018 *
CS8900_PP_ADDR_TXCMD           0x0144 * # -W - 4.5., p. 70 - 5.7., p. 98 *
CS8900_PP_ADDR_TXLENGTH        0x0146 * # -W - 4.5., p. 70 - 5.7., p. 98 *
CS8900_PP_ADDR_MAC_ADDR        0x0158 * # RW - 4.6., p. 71 - 5.3., p. 86 *
                            0x015a
                            0x015c
*/

#ifdef RAWNET_DEBUG_FRAMES
#define return( _x_ )                                                                                                          \
    {                                                                                                                          \
        int retval = _x_;                                                                                                      \
                                                                                                                               \
        log_message(cs8900_log, "%s correct_mac=%u, broadcast=%u, multicast=%u, hashed=%u, hash_index=%u",                        \
                    (retval ? "+++ ACCEPTED" : "--- rejected"), *pcorrect_mac, *pbroadcast, *pmulticast, *phashed, *phash_index); \
        return retval;                                                                                                         \
    }
#endif

/*
 This is a helper for cs8900_receive() to determine if the received frame should be accepted
 according to the settings.

 This function is even allowed to be called in rawnetarch.c from rawnet_arch_receive()
 (via rawnet_should_accept) if necessary, and must be registered using rawnet_set_should_accept_func,
 which is the reason why its prototype is included in cs8900.h.
*/
int cs8900_should_accept(unsigned char *buffer, int length, int *phashed, int *phash_index, int *pcorrect_mac, int *pbroadcast, int *pmulticast)
{
    int hashreg; /* Hash Register (for hash computation) */

    assert(length >= 6); /* we need at least 6 octets since the DA has this length */

    /* first of all, delete any status */
    *phashed = 0;
    *phash_index = 0;
    *pcorrect_mac = 0;
    *pbroadcast = 0;
    *pmulticast = 0;

#ifdef RAWNET_DEBUG_FRAMES
    log_message(cs8900_log, "cs8900_should_accept called with %02X:%02X:%02X:%02X:%02X:%02X, length=%4u and buffer %s",
                cs8900_ia_mac[0], cs8900_ia_mac[1], cs8900_ia_mac[2], cs8900_ia_mac[3], cs8900_ia_mac[4], cs8900_ia_mac[5], length, debug_outbuffer(length, buffer));
#endif

    if (buffer[0] == cs8900_ia_mac[0] && buffer[1] == cs8900_ia_mac[1] && buffer[2] == cs8900_ia_mac[2] && buffer[3] == cs8900_ia_mac[3] && buffer[4] == cs8900_ia_mac[4] && buffer[5] == cs8900_ia_mac[5]) {
        /* this is our individual address (IA) */

        *pcorrect_mac = 1;

        /* if we don't want "correct MAC", we might have the chance
         * that this address fits the hash index
         */
        if (cs8900_recv_mac || cs8900_recv_promiscuous) {
            return(1);
        }
    }

    if (buffer[0] == 0xFF && buffer[1] == 0xFF && buffer[2] == 0xFF && buffer[3] == 0xFF && buffer[4] == 0xFF && buffer[5] == 0xFF) {
        /* this is a broadcast address */
        *pbroadcast = 1;

        /* broadcasts cannot be accepted by the hash filter */
        return ((cs8900_recv_broadcast || cs8900_recv_promiscuous) ? 1 : 0);
    }

    /* now check if DA passes the hash filter */
    hashreg = (~crc32_buf((char *)buffer, 6) >> 26) & 0x3F;

    *phashed = (cs8900_hash_mask[(hashreg >= 32) ? 1 : 0] & (1 << (hashreg & 0x1F))) ? 1 : 0;
    if (*phashed) {
        *phash_index = hashreg;

        if (buffer[0] & 0x80) {
            /* we have a multicast address */
            *pmulticast = 1;

            /* if the multicast address fits into the hash filter,
             * the hashed bit has to be clear
             */
            *phashed = 0;

            return ((cs8900_recv_multicast || cs8900_recv_promiscuous) ? 1 : 0);
        }
        return ((cs8900_recv_hashfilter || cs8900_recv_promiscuous) ? 1 : 0);
    }

    return(cs8900_recv_promiscuous ? 1 : 0);
}

#ifdef RAWNET_DEBUG_FRAMES
    #undef return
#endif

/* buffer       - where to store a frame */
/* &len         - length of received frame */
/* &hashed      - set if the dest. address is accepted by the hash filter */
/* &hash_index  - hash table index if hashed == TRUE */
/* &rx_ok       - set if good CRC and valid length */
/* &correct_mac - set if dest. address is exactly our IA */
/* &broadcast   - set if dest. address is a broadcast address */
/* &crc_error   - set if received frame had a CRC error */

static uint16_t cs8900_receive(void)
{
    uint16_t ret_val = 0x0004;

    uint8_t buffer[MAX_RXLENGTH];

    int len;
    int hashed;
    int hash_index;
    int rx_ok;
    int correct_mac;
    int broadcast;
    int multicast = 0; /* avoid warning */
    int crc_error;

    int newframe;

    int ready;

    do {
        len = MAX_RXLENGTH;

        ready = 1;  /* assume we will find a good frame */

        newframe = rawnet_arch_receive(buffer, &len, &hashed, &hash_index, &rx_ok, &correct_mac, &broadcast, &crc_error);

        assert((len & 1) == 0); /* length has to be even! */

        if (newframe) {
            if (hashed || correct_mac || broadcast) {
                /* we already know the type of frame: Trust it! */
#ifdef RAWNET_DEBUG_FRAMES
                log_message( cs8900_log, "+++ cs8900_receive(): *** hashed=%u, correct_mac=%u, broadcast=%u", hashed, correct_mac, broadcast);
#endif
            } else {
                /* determine ourself the type of frame */
                if (!cs8900_should_accept(buffer, len, &hashed, &hash_index, &correct_mac, &broadcast, &multicast)) {
                    /* if we should not accept this frame, just do nothing
                     * now, look for another one */
                    ready = 0; /* try another frame */
                    continue;
                }
            }

            /* we did receive a frame, return that status */
            ret_val |= rx_ok ? 0x0100 : 0;
            ret_val |= multicast ? 0x0200 : 0;

            if (!multicast) {
                ret_val |= hashed ? 0x0040 : 0;
            }

            if (hashed && rx_ok) {
                /* we have the 2nd, special format with hash index: */
                assert(hash_index < 64);
                ret_val |= hash_index << 9;
            } else {
                /* we have the regular format */
                ret_val |= correct_mac ? 0x0400 : 0;
                ret_val |= broadcast ? 0x0800 : 0;
                ret_val |= crc_error ? 0x1000 : 0;
                ret_val |= (len < MIN_RXLENGTH) ? 0x2000 : 0;
                ret_val |= (len > MAX_RXLENGTH) ? 0x4000 : 0;
            }

            /* discard any octets that are beyond the MAX_RXLEN */
            if (len > MAX_RXLENGTH) {
                len = MAX_RXLENGTH;
            }

            if (rx_ok) {
                int i;

                /* set relevant parts of the PP area to correct values */
                SET_PP_16(CS8900_PP_ADDR_RXLENGTH, len);

                for (i = 0; i < len; i++) {
                    SET_PP_8(CS8900_PP_ADDR_RX_FRAMELOC + i, buffer[i]);
                }

                /* set rx_buffer to where start reading *
                 * According to 4.10.9 (pp. 76-77), we start with RxStatus and RxLength!
                 */
                rx_buffer = CS8900_PP_ADDR_RXSTATUS;
                rx_length = len;
                rx_count = 0;
#ifdef CS8900_DEBUG_WARN_RXTX
                if (rx_state != CS8900_RX_IDLE) {
                    log_message(cs8900_log, "WARNING! New frame overwrites pending one!");
                }
#endif
                rx_state = CS8900_RX_GOT_FRAME;
#ifdef CS8900_DEBUG_RXTX_STATE
                log_message(cs8900_log, "RX: recvd frame (length=%04x,status=%04x)", rx_length, ret_val);
#endif
            }
        }
    } while (!ready);

#ifdef RAWNET_DEBUG_FRAMES
    if (ret_val != 0x0004) {
        log_message( cs8900_log, "+++ cs8900_receive(): ret_val=%04X", ret_val);
    }
#endif

    return ret_val;
}

/* ------------------------------------------------------------------------- */
/* TX/RX buffer handling */

static void cs8900_write_tx_buffer(uint8_t value, int odd_address)
{
    /* write tx data only if valid buffer is ready */
    if (tx_state != CS8900_TX_READ_BUSST) {
#ifdef CS8900_DEBUG_WARN_RXTX
        log_message(cs8900_log, "WARNING! Ignoring TX Write without correct Transmit Condition! (odd=%d,value=%02x)", odd_address, value);
#endif
        /* ensure correct tx state (needed if transmit < 4 was started) */
        cs8900_set_tx_status(0, 0);
    } else {
#ifdef CS8900_DEBUG_RXTX_STATE
        if (tx_count == 0) {
            log_message(cs8900_log, "TX: write frame (length=%04x)", tx_length);
        }
#endif

        /* always write LH, LH... to tx buffer */
        uint16_t addr = tx_buffer;
        if (odd_address) {
            addr++;
            tx_buffer += 2;
        }
        tx_count++;
        SET_PP_8(addr, value);

#ifdef CS8900_DEBUG_RXTX_DATA
        log_message(cs8900_log, "TX: %04x/%04x: %02x (buffer=%04x,odd=%d)", tx_count, tx_length, value, addr, odd_address);
#endif

        /* full frame transmitted? */
        if (tx_count == tx_length) {
#ifdef RAWNET_DEBUG_FRAMES
            log_message(cs8900_log, "rawnet_arch_transmit() called with:                 length=%4u and buffer %s", tx_length, debug_outbuffer(tx_length, &cs8900_packetpage[CS8900_PP_ADDR_TX_FRAMELOC]));
#endif

            if (!tx_enabled) {
#ifdef CS8900_DEBUG_WARN_RXTX
                log_message(cs8900_log, "WARNING! Can't transmit frame (Transmitter is not enabled)!");
#endif
            } else {
                /* send frame */
                uint16_t txcmd = GET_PP_16(CS8900_PP_ADDR_CC_TXCMD);
                rawnet_arch_transmit(
                    txcmd & 0x0100 ? 1 : 0,   /* FORCE: Delete waiting frames in transmit buffer */
                    txcmd & 0x0200 ? 1 : 0,   /* ONECOLL: Terminate after just one collision */
                    txcmd & 0x1000 ? 1 : 0,   /* INHIBITCRC: Do not append CRC to the transmission */
                    txcmd & 0x2000 ? 1 : 0,   /* TXPADDIS: Disable padding to 60/64 octets */
                    tx_length, &cs8900_packetpage[CS8900_PP_ADDR_TX_FRAMELOC]);
            }

            /* reset transmitter state */
            tx_state = CS8900_TX_IDLE;

#ifdef CS8900_DEBUG_RXTX_STATE
            log_message(cs8900_log, "TX: sent  frame (length=%04x)", tx_length);
#endif

            /* reset tx status */
            cs8900_set_tx_status(0, 0);
        }
    }
}

static uint8_t cs8900_read_rx_buffer(int odd_address)
{
    if (rx_state != CS8900_RX_GOT_FRAME) {
#ifdef CS8900_DEBUG_WARN_RXTX
        log_message(cs8900_log, "WARNING! RX Read without frame available! (odd=%d)", odd_address);
#endif
        /* always reads zero on HW */
        return 0;
    } else {
        /*
         According to the CS8900 spec, the handling is the following:
         first read H, then L (RX_STATUS), then H, then L (RX_LENGTH).
         Inside the RX frame data, we always get L then H, until the end is reached.

                                     even    odd
         CS8900_PP_ADDR_RXSTATUS:         -     proceed
         CS8900_PP_ADDR_RXLENGTH:         -     proceed
         CS8900_PP_ADDR_RX_FRAMELOC:      -       -
         CS8900_PP_ADDR_RX_FRAMELOC+2: proceed    -
         CS8900_PP_ADDR_RX_FRAMELOC+4: proceed    -

         */
        uint16_t addr = odd_address ? 1 : 0;
        uint8_t value;

        /* read RXSTATUS or RX_LENGTH */
        if (rx_count < 4) {
            addr += rx_buffer;
            value = GET_PP_8(addr);
            rx_count++;

            /* incr after RXSTATUS or RX_LENGTH even (L) read */
            if (!odd_address) {
                rx_buffer += 2;
            }
        } else {
            /* read frame data */

            /* incr before frame read (but not in first word) */
            if ((rx_count >= 6) && (!odd_address)) {
                rx_buffer += 2;
            }

            addr += rx_buffer;
            value = GET_PP_8(addr);
            rx_count++;
        }

#ifdef CS8900_DEBUG_RXTX_DATA
        log_message(cs8900_log, "RX: %04x/%04x: %02x (buffer=%04x,odd=%d)", rx_count, rx_length + 4, value, addr, odd_address);
#endif

        /* check frame end */
        if (rx_count >= rx_length + 4) {
            /* reset receiver state to idle */
            rx_state = CS8900_RX_IDLE;
#ifdef CS8900_DEBUG_RXTX_STATE
            log_message(cs8900_log, "RX: read  frame (length=%04x)", rx_length);
#endif
        }
        return value;
    }
}

/* ------------------------------------------------------------------------- */
/* handle side-effects of read and write operations */

#define on_off_str(x) ((x) ? on_off[0] : on_off[1])

/*
 This is called *after* the relevant octets are written
*/
static void cs8900_sideeffects_write_pp(uint16_t ppaddress, int odd_address)
{
    const char *on_off[2] = { "on", "off" };
    uint16_t content = GET_PP_16( ppaddress );

    assert((ppaddress & 1) == 0);

    switch (ppaddress) {
        case CS8900_PP_ADDR_CC_RXCFG:
            /* Skip_1 Flag: remove current (partial) tx frame and restore state */
            if (content & 0x40) {
                /* restore tx state */
                if (tx_state != CS8900_TX_IDLE) {
                    tx_state = CS8900_TX_IDLE;
#ifdef CS8900_DEBUG_RXTX_STATE
                    log_message(cs8900_log, "TX: skipping current frame");
#endif
                }

                /* reset transmitter */
                cs8900_set_transmitter(tx_enabled);

                /* this is an "act once" bit, thus restore it to zero. */
                content &= ~0x40;
                SET_PP_16(ppaddress, content);
            }
            break;
        case CS8900_PP_ADDR_CC_RXCTL:
            if (cs8900_recv_control != content) {
                cs8900_recv_broadcast = content & 0x0800; /* broadcast */
                cs8900_recv_mac = content & 0x0400; /* individual address (IA) */
                cs8900_recv_multicast = content & 0x0200; /* multicast if address passes the hash filter */
                cs8900_recv_correct = content & 0x0100; /* accept correct frames */
                cs8900_recv_promiscuous = content & 0x0080; /* promiscuous mode */
                cs8900_recv_hashfilter = content & 0x0040; /* accept if IA passes the hash filter */
                cs8900_recv_control = content;

                log_message(cs8900_log, "setup receiver: broadcast=%s mac=%s multicast=%s correct=%s promiscuous=%s hashfilter=%s",
                            on_off_str(cs8900_recv_broadcast), on_off_str(cs8900_recv_mac), on_off_str(cs8900_recv_multicast), on_off_str(cs8900_recv_correct), on_off_str(cs8900_recv_promiscuous), on_off_str(cs8900_recv_hashfilter));

                rawnet_arch_recv_ctl(cs8900_recv_broadcast, cs8900_recv_mac, cs8900_recv_multicast, cs8900_recv_correct, cs8900_recv_promiscuous, cs8900_recv_hashfilter);
            }
            break;
        case CS8900_PP_ADDR_CC_LINECTL:
            {
                int enable_tx = (content & 0x0080) == 0x0080;
                int enable_rx = (content & 0x0040) == 0x0040;

                if ((enable_tx != tx_enabled) || (enable_rx != rx_enabled)) {
                    rawnet_arch_line_ctl(enable_tx, enable_rx);
                    cs8900_set_transmitter(enable_tx);
                    cs8900_set_receiver(enable_rx);

                    log_message(cs8900_log, "line control: transmitter=%s receiver=%s", on_off_str(enable_tx), on_off_str(enable_rx));
                }
            }
            break;
        case CS8900_PP_ADDR_CC_SELFCTL:
            {
                /* reset chip? */
                if ((content & 0x40) == 0x40) {
                    cs8900_reset();
                }
            }
            break;
        case CS8900_PP_ADDR_TXCMD:
            {
                if (odd_address) {
                    uint16_t txcommand = GET_PP_16(CS8900_PP_ADDR_TXCMD);

                    /* already transmitting? */
                    if (tx_state == CS8900_TX_READ_BUSST) {
#ifdef CS8900_DEBUG_WARN_RXTX
                        log_message(cs8900_log, "WARNING! Early abort of transmitted frame");
#endif
                    }

                    /* The transmit status command gets the last transmit command */
                    SET_PP_16(CS8900_PP_ADDR_CC_TXCMD, txcommand);

                    /* set transmit state */
                    tx_state = CS8900_TX_GOT_CMD;
                    cs8900_set_tx_status(0, 0);

#ifdef CS8900_DEBUG_RXTX_STATE
                    log_message(cs8900_log, "TX: COMMAND accepted (%04x)", txcommand);
#endif
                }
            }
            break;
        case CS8900_PP_ADDR_TXLENGTH:
            {
                if (odd_address && (tx_state == CS8900_TX_GOT_CMD)) {
                    uint16_t txlength = GET_PP_16(CS8900_PP_ADDR_TXLENGTH);
                    uint16_t txcommand = GET_PP_16(CS8900_PP_ADDR_CC_TXCMD);

                    if (txlength < 4) {
                        /* frame to short */
#ifdef CS8900_DEBUG_RXTX_STATE
                        log_message(cs8900_log, "TX: LENGTH rejected - too short! (%04x)", txlength);
#endif
                        /* mask space available but do not commit */
                        tx_state = CS8900_TX_IDLE;
                        cs8900_set_tx_status(1, 0);
                    } else if ((txlength > MAX_TXLENGTH) || ((txlength > MAX_TXLENGTH - 4) && (!(txcommand & 0x1000)))) {
                        tx_state = CS8900_TX_IDLE;
#ifdef CS8900_DEBUG_RXTX_STATE
                        log_message(cs8900_log, "TX: LENGTH rejected - too long! (%04x)", txlength);
#endif
                        /* txlength too big, mark an error */
                        cs8900_set_tx_status(0, 1);
                    } else {
                        /* make sure we put the octets to transmit at the right place */
                        tx_buffer = CS8900_PP_ADDR_TX_FRAMELOC;
                        tx_count = 0;
                        tx_length = txlength;
                        tx_state = CS8900_TX_GOT_LEN;

#ifdef CS8900_DEBUG_RXTX_STATE
                        log_message(cs8900_log, "TX: LENGTH accepted (%04x)", txlength);
#endif
                        /* all right, signal that we're ready for the next frame */
                        cs8900_set_tx_status(1, 0);
                    }
                }
            }
            break;
        case CS8900_PP_ADDR_LOG_ADDR_FILTER:
        case CS8900_PP_ADDR_LOG_ADDR_FILTER + 2:
        case CS8900_PP_ADDR_LOG_ADDR_FILTER + 4:
        case CS8900_PP_ADDR_LOG_ADDR_FILTER + 6:
            {
                unsigned int pos = 8 * (ppaddress - CS8900_PP_ADDR_LOG_ADDR_FILTER + odd_address);
                uint32_t *p = (pos < 32) ? &cs8900_hash_mask[0] : &cs8900_hash_mask[1];

                *p &= ~(0xFF << pos); /* clear out relevant bits */
                *p |= GET_PP_8(ppaddress + odd_address) << pos;

                rawnet_arch_set_hashfilter(cs8900_hash_mask);

#if 0
                if (odd_address && (ppaddress == CS8900_PP_ADDR_LOG_ADDR_FILTER + 6)) {
                    log_message(cs8900_log, "set hash filter: %02x:%02x:%02x:%02x:%02x:%02x",
                                cs8900_hash_mask[0], cs8900_hash_mask[1], cs8900_hash_mask[2], cs8900_hash_mask[3], cs8900_hash_mask[4], cs8900_hash_mask[5]);
                }
#endif
            }
            break;
        case CS8900_PP_ADDR_MAC_ADDR:
        case CS8900_PP_ADDR_MAC_ADDR + 2:
        case CS8900_PP_ADDR_MAC_ADDR + 4:
            /* the MAC address has been changed */
            cs8900_ia_mac[ppaddress - CS8900_PP_ADDR_MAC_ADDR + odd_address] = GET_PP_8(ppaddress + odd_address);
            rawnet_arch_set_mac(cs8900_ia_mac);
            if (odd_address && (ppaddress == CS8900_PP_ADDR_MAC_ADDR + 4)) {
                log_message(cs8900_log, "set MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
                            cs8900_ia_mac[0], cs8900_ia_mac[1], cs8900_ia_mac[2], cs8900_ia_mac[3], cs8900_ia_mac[4], cs8900_ia_mac[5]);
            }
            break;
    }
}
#undef on_off_str

/*
 This is called *before* the relevant octets are read
*/
static void cs8900_sideeffects_read_pp(uint16_t ppaddress, int odd_address)
{
    switch (ppaddress) {
        case CS8900_PP_ADDR_SE_RXEVENT:
            /* reading this before all octets of the frame are read
               performs an "implied skip" */
            {
                int access_mask = (odd_address) ? 1 : 2;
                /* update the status register only if the full word of the last
                   status was read! unfortunately different access patterns are
                   possible: either the status is read LH, LH, LH...
                   or HL, HL, HL, or even L, L, L or H, H, H */
                if ((access_mask & rxevent_read_mask) != 0) {
                    /* receiver is not enabled */
                    if (!rx_enabled) {
#ifdef CS8900_DEBUG_WARN_RXTX
                        log_message(cs8900_log, "WARNING! Can't receive any frame (Receiver is not enabled)!");
#endif
                    } else {
                        /* perform frame reception */
                        uint16_t ret_val = cs8900_receive();

                        /* RXSTATUS and RXEVENT are the same, except that RXSTATUS buffers
                           the old value while RXEVENT sets a new value whenever it is called
                        */
                        SET_PP_16(CS8900_PP_ADDR_RXSTATUS, ret_val);
                        SET_PP_16(CS8900_PP_ADDR_SE_RXEVENT, ret_val);
                    }
                    /* reset read mask of (possible) other access */
                    rxevent_read_mask = access_mask;
                } else {
                    /* add access bit to mask */
                    rxevent_read_mask |= access_mask;
                }
            }
            break;
        case CS8900_PP_ADDR_SE_BUSST:
            if (odd_address) {
                /* read busst before transmit condition is fullfilled */
                if (tx_state == CS8900_TX_GOT_LEN) {
                    uint16_t bus_status = GET_PP_16(CS8900_PP_ADDR_SE_BUSST);

                    /* check Rdy4TXNow flag */
                    if ((bus_status & 0x100) == 0x100) {
                        tx_state = CS8900_TX_READ_BUSST;
#ifdef CS8900_DEBUG_RXTX_STATE
                        log_message(cs8900_log, "TX: Ready4TXNow set! (%04x)", bus_status);
#endif
                    } else {
#ifdef CS8900_DEBUG_RXTX_STATE
                        log_message(cs8900_log, "TX: waiting for Ready4TXNow! (%04x)", bus_status);
#endif
                    }
                }
            }
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* read/write from packet page register */

/* read a register from packet page */
static uint16_t cs8900_read_register(uint16_t ppaddress)
{
    uint16_t value = GET_PP_16(ppaddress);

    /* --- check the register address --- */
    if (ppaddress < 0x100) {
        /* reserved range reads 0x0300 on real HW */
        if ((ppaddress >= 0x0004) && (ppaddress < 0x0020)) {
            return 0x0300;
        }
    } else if (ppaddress < 0x120) {
        /* --- read control register range --- */
        uint16_t regNum = ppaddress - 0x100;

        regNum &= ~1;
        regNum++;
#ifdef CS8900_DEBUG_REGISTERS
        log_message(cs8900_log, "Read Control Register %04x: %04x (reg=%02x)", ppaddress, value, regNum);
#endif

        /* reserved register? */
        if ((regNum == 0x01) || (regNum == 0x11) || (regNum > 0x19)) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Read reserved Control Register %04x (reg=%02x)", ppaddress, regNum);
#endif
            /* real HW returns 0x0300 in reserved register range */
            return 0x0300;
        }

        /* make sure internal address is always valid */
        assert((value & 0x3f) == regNum);
    } else if (ppaddress < 0x140) {
        /* --- read status register range --- */
        uint16_t regNum = ppaddress - 0x120;

        regNum &= ~1;
#ifdef CS8900_DEBUG_REGISTERS
#ifdef CS8900_DEBUG_IGNORE_RXEVENT
        if (regNum != 4) /* do not show RXEVENT */
#endif
        log_message(cs8900_log, "Read  Status  Register %04x: %04x (reg=%02x)", ppaddress, value, regNum);
#endif

        /* reserved register? */
        if ((regNum == 0x02) || (regNum == 0x06) || (regNum == 0x0a) || (regNum == 0x0e) || (regNum == 0x1a) || (regNum == 0x1e)) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Read reserved Status Register %04x (reg=%02x)", ppaddress, regNum);
#endif
            /* real HW returns 0x0300 in reserved register range */
            return 0x0300;
        }

        /* make sure internal address is always valid */
        assert((value & 0x3f) == regNum);
    } else if (ppaddress < 0x150) {
        /* --- read transmit register range --- */
        if (ppaddress == 0x144) {
            /* make sure internal address is always valid */
            assert((value & 0x3f) == 0x09);
#ifdef CS8900_DEBUG_REGISTERS
            log_message(cs8900_log, "Read  TX Cmd  Register %04x: %04x", ppaddress, value);
#endif
        } else if (ppaddress == 0x146) {
#ifdef CS8900_DEBUG_REGISTERS
            log_message(cs8900_log, "Read  TX Len  Register %04x: %04x", ppaddress, value);
#endif
        } else {
            /* reserved range */
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Read reserved Initiate Transmit Register %04x", ppaddress);
#endif
            /* real HW returns 0x0300 in reserved register range */
            return 0x0300;
        }
    } else if (ppaddress < 0x160) {
        /* --- read address filter register range --- */
        /* reserved range */
        if (ppaddress >= 0x15e) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Read reserved Address Filter Register %04x", ppaddress);
#endif
            /* real HW returns 0x0300 in reserved register range */
            return 0x0300;
        }
    } else if (ppaddress < 0x400) {
        /* --- reserved range below 0x400 ---
           returns 0x300 on real HW
        */
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Read reserved Register %04x", ppaddress);
#endif
        return 0x0300;
    } else if (ppaddress < 0xa00) {
        /* --- range from 0x400 .. 0x9ff --- RX Frame */
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Read from RX Buffer Range %04x", ppaddress);
#endif
        return 0x0000;
    } else {
        /* --- range from 0xa00 .. 0xfff --- TX Frame */
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Read from TX Buffer Range %04x", ppaddress);
#endif
        return 0x0000;
    }

    /* actually read from pp memory */
    return value;
}

static void cs8900_write_register(uint16_t ppaddress, uint16_t value)
{
    /* --- write bus interface register range --- */
    if (ppaddress < 0x100) {
        int ignore = 0;

        if (ppaddress < 0x20) {
            ignore = 1;
        } else if ((ppaddress >= 0x26) && (ppaddress < 0x2c)) {
            ignore = 1;
        } else if (ppaddress == 0x38) {
            ignore = 1;
        } else if (ppaddress >= 0x44) {
            ignore = 1;
        }
        if (ignore) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Ignoring write to read only/reserved Bus Interface Register %04x", ppaddress);
#endif
            return;
        }
    } else if (ppaddress < 0x120) {
        /* --- write to control register range --- */
        uint16_t regNum = ppaddress - 0x100;

        regNum &= ~1;
        regNum += 1;
        /* validate internal address */
        if ((value & 0x3f) != regNum) {
            /* fix internal address */
            value &= ~0x3f;
            value |= regNum;
        }
#ifdef CS8900_DEBUG_REGISTERS
        log_message(cs8900_log, "Write Control Register %04x: %04x (reg=%02x)", ppaddress, value, regNum);
#endif

        /* invalid register? -> ignore! */
        if ((regNum == 0x01) || (regNum == 0x11) || (regNum > 0x19)) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Ignoring write to reserved Control Register %04x (reg=%02x)", ppaddress, regNum);
#endif
            return;
        }
    } else if (ppaddress < 0x140) {
        /* --- write to status register range --- */
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Ignoring write to read-only Status Register %04x", ppaddress);
#endif
        return;
    } else if (ppaddress < 0x150) {
        /* --- write to initiate transmit register range --- */
        /* check tx_cmd register */
        if (ppaddress == 0x144) {
            /* validate internal address */
            if ((value & 0x3f) != 0x09) {
                /* fix internal address */
                value &= ~0x3f;
                value |= 0x09;
            }
            /* mask out reserved bits */
            value &= 0x33ff;
#ifdef CS8900_DEBUG_REGISTERS
            log_message(cs8900_log, "Write TX Cmd  Register %04x: %04x", ppaddress, value);
#endif
        } else if (ppaddress == 0x146) {
            /* check tx_length register */
            /* HW always masks 0x0fff */
            value &= 0x0fff;
#ifdef CS8900_DEBUG_REGISTERS
            log_message(cs8900_log, "Write TX Len  Register %04x: %04x", ppaddress, value);
#endif
        } else if ((ppaddress < 0x144) || (ppaddress > 0x147)) {
            /* reserved range */
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Ignoring write to reserved Initiate Transmit Register %04x", ppaddress);
#endif
            return;
        }
    } else if (ppaddress < 0x160) {
        /* --- write to address filter register range --- */
        /* reserved range */
        if (ppaddress >= 0x15e) {
#ifdef CS8900_DEBUG_WARN_REG
            log_message(cs8900_log, "WARNING! Ingoring write to reserved Address Filter Register %04x", ppaddress);
#endif
            return;
        }
    } else if (ppaddress < 0x400) {
        /* --- ignore write outside --- */
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Ingoring write to reserved Register %04x", ppaddress);
#endif
        return;
    } else if (ppaddress < 0xa00) {
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Ignoring write to RX Buffer Range %04x", ppaddress);
#endif
        return;
    } else {
#ifdef CS8900_DEBUG_WARN_REG
        log_message(cs8900_log, "WARNING! Ignoring write to TX Buffer Range %04x", ppaddress);
#endif
        return;
    }

    /* actually set value */
    SET_PP_16(ppaddress, value);
}

#define PP_PTR_AUTO_INCR_FLAG 0x8000 /* auto increment flag in package pointer */
#define PP_PTR_FLAG_MASK      0xf000 /* is always : x y 1 1 (with x=auto incr) */
#define PP_PTR_ADDR_MASK      0x0fff /* address portion of packet page pointer */

static void cs8900_auto_incr_pp_ptr(void)
{
    /* perform auto increment of packet page pointer */
    if ((cs8900_packetpage_ptr & PP_PTR_AUTO_INCR_FLAG) == PP_PTR_AUTO_INCR_FLAG) {
        /* pointer is always increment by one on real HW */
        uint16_t ptr = cs8900_packetpage_ptr & PP_PTR_ADDR_MASK;
        uint16_t flags = cs8900_packetpage_ptr & PP_PTR_FLAG_MASK;

        ptr++;
        cs8900_packetpage_ptr = ptr | flags;
    }
}

/* ------------------------------------------------------------------------- */
/* read/write CS8900 registers from VICE */

#define LO_uint8_t(x)      (uint8_t)((x) & 0xff)
#define HI_uint8_t(x)      (uint8_t)(((x) >> 8) & 0xff)
#define LOHI_uint16_t(x, y) ((uint16_t)(x) | (((uint16_t)(y)) << 8 ))

/* ----- read byte from I/O range in VICE ----- */
uint8_t cs8900_read(uint16_t io_address)
{
    uint8_t retval, lo, hi;
    uint16_t word_value;
    uint16_t reg_base;

    assert(cs8900);
    assert(cs8900_packetpage);
    assert(io_address < 0x10);

    /* register base addr */
    reg_base = io_address & ~1;

    /* RX register is special as it reads from RX buffer directly */
    if ((reg_base == CS8900_ADDR_RXTXDATA) || (reg_base == CS8900_ADDR_RXTXDATA2)) {
        return cs8900_read_rx_buffer(io_address & 0x01);
    }

    /* read packet page pointer */
    if (reg_base == CS8900_ADDR_PP_PTR) {
        word_value = cs8900_packetpage_ptr;
    } else {
        /* read a register from packet page */
        uint16_t ppaddress = 0;

        /* determine read addr in packet page */
        switch (reg_base) {
            /* PP_DATA2 behaves like PP_DATA on real HW
               both show the contents at the page pointer */
            case CS8900_ADDR_PP_DATA:
            case CS8900_ADDR_PP_DATA2:
                /* mask and align address of packet pointer */
                ppaddress = cs8900_packetpage_ptr & PP_PTR_ADDR_MASK;
                ppaddress &= ~1;
                /* if flags match then auto incr pointer */
                cs8900_auto_incr_pp_ptr();
                break;
            case CS8900_ADDR_INTSTQUEUE:
                ppaddress = CS8900_PP_ADDR_SE_ISQ;
                break;
            case CS8900_ADDR_TXCMD:
                ppaddress = CS8900_PP_ADDR_TXCMD;
                break;
            case CS8900_ADDR_TXLENGTH:
                ppaddress = CS8900_PP_ADDR_TXLENGTH;
                break;
            default:
                /* invalid! */
                assert(0);
                break;
        }

        /* do side effects before access */
        cs8900_sideeffects_read_pp(ppaddress, io_address & 1);

        /* read register value */
        word_value = cs8900_read_register(ppaddress);

#ifdef CS8900_DEBUG_LOAD
        log_message(cs8900_log, "reading PP Ptr: $%04X => $%04X.", ppaddress, word_value);
#endif
    }

    /* extract return value from word_value */
    lo = LO_uint8_t(word_value);
    hi = HI_uint8_t(word_value);
    if ((io_address & 1) == 0) {
        /* low byte on even address */
        retval = lo;
    } else {
        /* high byte on odd address */
        retval = hi;
    }

#ifdef CS8900_DEBUG_LOAD
    log_message(cs8900_log, "read [$%02X] => $%02X.", io_address, retval);
#endif

    /* update _word_ value in register bank */
    cs8900[reg_base] = lo;
    cs8900[reg_base + 1] = hi;

    return retval;
}

/* ----- peek byte without side effects from I/O range in VICE ----- */
uint8_t cs8900_peek(uint16_t io_address)
{
    uint8_t retval, lo, hi;
    uint16_t word_value;
    uint16_t reg_base;

    assert(cs8900);
    assert(cs8900_packetpage);
    assert(io_address < 0x10);

    /* register base addr */
    reg_base = io_address & ~1;

    /* RX register is special as it reads from RX buffer directly */
    if ((reg_base == CS8900_ADDR_RXTXDATA) || (reg_base == CS8900_ADDR_RXTXDATA2)) {
        return 0; /* FIXME: peek rx buffer */
    }

    /* read packet page pointer */
    if (reg_base == CS8900_ADDR_PP_PTR) {
        word_value = cs8900_packetpage_ptr;
    } else {
        /* read a register from packet page */
        uint16_t ppaddress = 0;

        /* determine read addr in packet page */
        switch (reg_base) {
            /* PP_DATA2 behaves like PP_DATA on real HW
               both show the contents at the page pointer */
            case CS8900_ADDR_PP_DATA:
            case CS8900_ADDR_PP_DATA2:
                /* mask and align address of packet pointer */
                ppaddress = cs8900_packetpage_ptr & PP_PTR_ADDR_MASK;
                ppaddress &= ~1;
                /* if flags match then auto incr pointer */
                cs8900_auto_incr_pp_ptr();
                break;
            case CS8900_ADDR_INTSTQUEUE:
                ppaddress = CS8900_PP_ADDR_SE_ISQ;
                break;
            case CS8900_ADDR_TXCMD:
                ppaddress = CS8900_PP_ADDR_TXCMD;
                break;
            case CS8900_ADDR_TXLENGTH:
                ppaddress = CS8900_PP_ADDR_TXLENGTH;
                break;
            default:
                /* invalid! */
                assert(0);
                break;
        }

        /* read register value */
        word_value = cs8900_read_register(ppaddress);
    }

    /* extract return value from word_value */
    lo = LO_uint8_t(word_value);
    hi = HI_uint8_t(word_value);
    if ((io_address & 1) == 0) {
        /* low byte on even address */
        retval = lo;
    } else {
        /* high byte on odd address */
        retval = hi;
    }
    return retval;
}

/* ----- write byte to I/O range of VICE ----- */
void cs8900_store(uint16_t io_address, uint8_t byte)
{
    uint16_t reg_base;
    uint16_t word_value;

    assert(cs8900);
    assert(cs8900_packetpage);
    assert(io_address < 0x10);

#ifdef CS8900_DEBUG_STORE
    log_message(cs8900_log, "store [$%02X] <= $%02X.", io_address, (int)byte);
#endif

    /* register base addr */
    reg_base = io_address & ~1;

    /* TX Register is special as it writes to TX buffer directly */
    if ((reg_base == CS8900_ADDR_RXTXDATA) || (reg_base == CS8900_ADDR_RXTXDATA2)) {
        cs8900_write_tx_buffer(byte, io_address & 1);
        return;
    }

    /* combine stored value with new written byte */
    if ((io_address & 1) == 0) {
        /* overwrite low byte */
        word_value = LOHI_uint16_t(byte, cs8900[reg_base + 1]);
    } else {
        /* overwrite high byte */
        word_value = LOHI_uint16_t(cs8900[reg_base], byte);
    }

    if (reg_base == CS8900_ADDR_PP_PTR) {
        /*  we store the full package pointer in cs8900_packetpage_ptr variable.
            this includes the mask area (0xf000) and the addr range (0x0fff).
            we ensure that the bits 0x3000 are always set (as in real HW).
            odd values of the pointer are valid and supported.
            only register read and write have to be mapped to word boundary. */
        word_value |= 0x3000;
        cs8900_packetpage_ptr = word_value;

#ifdef CS8900_DEBUG_STORE
        log_message(cs8900_log, "set PP Ptr to $%04X.", cs8900_packetpage_ptr);
#endif
    } else {
        /* write a register */

        /*! \TODO: Find a reasonable default */
        uint16_t ppaddress = CS8900_PP_ADDR_PRODUCTID;

        /* now determine address of write in packet page */
        switch (reg_base) {
            case CS8900_ADDR_PP_DATA:
            case CS8900_ADDR_PP_DATA2:
                /* mask and align ppaddress from page pointer */
                ppaddress = cs8900_packetpage_ptr & (MAX_PACKETPAGE_ARRAY - 1);
                ppaddress &= ~1;
                /* auto increment pp ptr */
                cs8900_auto_incr_pp_ptr();
                break;
            case CS8900_ADDR_TXCMD:
                ppaddress = CS8900_PP_ADDR_TXCMD;
                break;
            case CS8900_ADDR_TXLENGTH:
                ppaddress = CS8900_PP_ADDR_TXLENGTH;
                break;
            case CS8900_ADDR_INTSTQUEUE:
                ppaddress = CS8900_PP_ADDR_SE_ISQ;
                break;
            case CS8900_ADDR_PP_PTR:
                break;
            default:
                /* invalid */
                assert(0);
                break;
        }

#ifdef CS8900_DEBUG_STORE
        log_message(cs8900_log, "before writing to PP Ptr: $%04X <= $%04X.", ppaddress, word_value);
#endif

        /* perform the write */
        cs8900_write_register(ppaddress, word_value);

        /* handle sideeffects */
        cs8900_sideeffects_write_pp(ppaddress, io_address & 1);

        /* update word value if it was changed in write register or by side effect */
        word_value = GET_PP_16(ppaddress);

#ifdef CS8900_DEBUG_STORE
        log_message(cs8900_log, "after  writing to PP Ptr: $%04X <= $%04X.", ppaddress, word_value);
#endif
    }

    /* update cs8900 registers */
    cs8900[reg_base] = LO_uint8_t(word_value);
    cs8900[reg_base + 1] = HI_uint8_t(word_value);
}

int cs8900_dump(void)
{
    /* FIXME: this is incomplete */
    mon_out("Link status: %s\n", (GET_PP_16(CS8900_PP_ADDR_SE_LINEST) & 0x80) ? "up" : "no link");
    mon_out("Package Page Ptr: $%04X (autoincrement %s)\n",
            (unsigned int)(cs8900_packetpage_ptr & PP_PTR_ADDR_MASK),
            (cs8900_packetpage_ptr & PP_PTR_AUTO_INCR_FLAG) != 0 ? "enabled" : "disabled");
    return 0;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CS8900"

/* FIXME: implement snapshot support */
int cs8900_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }
    snapshot_set_error(SNAPSHOT_MODULE_NOT_IMPLEMENTED);
    return -1;
#if 0
    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int cs8900_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    uint8_t vmajor, vminor;
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

#endif /* #ifdef HAVE_RAWNET */
