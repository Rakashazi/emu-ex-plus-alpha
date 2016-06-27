/*
 * acia.h - ACIA 6551 register number declarations.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Spiro Trikaliotis
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

#ifndef VICE_ACIA_H
#define VICE_ACIA_H

enum {
    ACIA_DR    = 0, /* Data register */
    ACIA_SR    = 1, /* R: status register W: programmed Reset */
    ACIA_CMD   = 2, /* Command register */
    ACIA_CTRL  = 3, /* Control register */
    T232_NDEF1 = 4, /* Undefined register 1, turbo232 only */
    T232_NDEF2 = 5, /* Undefined register 2, turbo232 only */
    T232_NDEF3 = 6, /* Undefined register 3, turbo232 only */
    T232_ECTRL = 7  /* Enhanced control register, turbo232 only */
};

enum {
    T232_ECTRL_BITS_EXT_BPS_MASK     = 0x03,
    T232_ECTRL_BITS_EXT_BPS_230400   = 0x00,
    T232_ECTRL_BITS_EXT_BPS_115200   = 0x01,
    T232_ECTRL_BITS_EXT_BPS_57600    = 0x02,
    T232_ECTRL_BITS_EXT_BPS_RESERVED = 0x03,

    T232_ECTRL_BITS_EXT_ACTIVE       = 0x04,

    T232_ECTRL_DEFAULT_AFTER_HW_RESET = 0
};

enum {
    ACIA_SR_BITS_PARITY_ERROR      = 0x01, /* cleared automatically by read of ACIA_DR or next error-free reception */
    ACIA_SR_BITS_FRAMING_ERROR     = 0x02, /* cleared automatically by read of ACIA_DR or next error-free reception */
    ACIA_SR_BITS_OVERRUN_ERROR     = 0x04, /* cleared automatically by read of ACIA_DR or next error-free reception */
    ACIA_SR_BITS_RECEIVE_DR_FULL   = 0x08, /* cleared automatically by read of ACIA_DR */
    ACIA_SR_BITS_TRANSMIT_DR_EMPTY = 0x10, /* cleared automatically by write of ACIA_DR */
    ACIA_SR_BITS_DSR               = 0x20, /* reflects current DSR state */
    ACIA_SR_BITS_DCD               = 0x40, /* reflects current DCD state */
    ACIA_SR_BITS_IRQ               = 0x80, /* cleared by read of status register */

    ACIA_SR_DEFAULT_AFTER_HW_RESET = ACIA_SR_BITS_TRANSMIT_DR_EMPTY
};

enum {
    ACIA_CMD_BITS_PARITY_TYPE_MASK        = 0xC0,
    ACIA_CMD_BITS_PARITY_TYPE_ODD         = 0x00,
    ACIA_CMD_BITS_PARITY_TYPE_EVEN        = 0x40,
    ACIA_CMD_BITS_PARITY_TYPE_MARK_TX     = 0x80,
    ACIA_CMD_BITS_PARITY_TYPE_SPACE_TX    = 0xC0,

    ACIA_CMD_BITS_PARITY_ENABLED          = 0x20,

    ACIA_CMD_BITS_ECHO                    = 0x10,

    ACIA_CMD_BITS_TRANSMITTER_MASK        = 0x0C,
    ACIA_CMD_BITS_TRANSMITTER_NO_RTS      = 0x00,
    ACIA_CMD_BITS_TRANSMITTER_TX_WITH_IRQ = 0x04,
    ACIA_CMD_BITS_TRANSMITTER_TX_WO_IRQ   = 0x08,
    ACIA_CMD_BITS_TRANSMITTER_BREAK       = 0x0C,

    ACIA_CMD_BITS_IRQ_DISABLED            = 0x02,
    ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ = 0x01,

    ACIA_CMD_DEFAULT_AFTER_HW_RESET       = 0
};

enum {
    ACIA_CTRL_BITS_2_STOP            = 0x80,

    ACIA_CTRL_BITS_WORD_LENGTH_MASK  = 0x60,
    ACIA_CTRL_BITS_WORD_LENGTH_8     = 0x00,
    ACIA_CTRL_BITS_WORD_LENGTH_7     = 0x20,
    ACIA_CTRL_BITS_WORD_LENGTH_6     = 0x40,
    ACIA_CTRL_BITS_WORD_LENGTH_5     = 0x60,

    ACIA_CTRL_BITS_INTERNAL_BPS      = 0x10,

    ACIA_CTRL_BITS_BPS_MASK          = 0x0F,
    ACIA_CTRL_BITS_BPS_16X_EXT_CLK   = 0x00,
    ACIA_CTRL_BITS_BPS_50            = 0x01,
    ACIA_CTRL_BITS_BPS_75            = 0x02,
    ACIA_CTRL_BITS_BPS_109_92        = 0x03,
    ACIA_CTRL_BITS_BPS_134_58        = 0x04,
    ACIA_CTRL_BITS_BPS_150           = 0x05,
    ACIA_CTRL_BITS_BPS_300           = 0x06,
    ACIA_CTRL_BITS_BPS_600           = 0x07,
    ACIA_CTRL_BITS_BPS_1200          = 0x08,
    ACIA_CTRL_BITS_BPS_1800          = 0x09,
    ACIA_CTRL_BITS_BPS_2400          = 0x0A,
    ACIA_CTRL_BITS_BPS_3600          = 0x0B,
    ACIA_CTRL_BITS_BPS_4800          = 0x0C,
    ACIA_CTRL_BITS_BPS_7200          = 0x0D,
    ACIA_CTRL_BITS_BPS_9600          = 0x0E,
    ACIA_CTRL_BITS_BPS_19200         = 0x0F,

    ACIA_CTRL_DEFAULT_AFTER_HW_RESET = 0
};

#define ACIA_MODE_NORMAL    0  /* Normal ACIA emulation */
#define ACIA_MODE_SWIFTLINK 1  /* Swiftlink ACIA emulation, baud rates are doubled */
#define ACIA_MODE_TURBO232  2  /* Turbo232 ACIA emulation, baud rates are doubled,
                                 and enhanced baud rate register */

#define ACIA_MODE_LOWEST ACIA_MODE_NORMAL

int acia_dump(void *acia_context);

#endif
