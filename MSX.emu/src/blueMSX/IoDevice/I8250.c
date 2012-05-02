/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8250.c,v $
**
** $Revision: 1.10 $
**
** $Date: 2008-03-31 19:42:19 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
// TODO:
// - Interrupt handling
// - Handshaking, Flow control
// - Dynamic config of baud rate, data bits, etc.
// - Receive buffers
#include "I8250.h"
#include "SaveState.h"
#include "Board.h"
#include <stdlib.h>

#define LCR_DIVISOR_LATCH_ACCESS_BIT 0x80
#define LSR_DATA_READY 0x01
#define LSR_OVERRUN_ERROR 0x02
#define LSR_TRANSMITTER_HOLDING_REGISTER_EMPTY 0x20
#define LSR_TRANSMITTER_EMPTY 0x40
#define MCR_LOOPBACK_TEST 0x10

typedef enum
{
    I8250PORT_RBR_THR_DLL,  // Receiver Buffer Register (read only), Divisor Latch LSB (DLAB)
                            // Transmitter Holding Register (write only), Divisor Latch LSB (DLAB)
    I8250PORT_IER_DLM,      // Interrupt Enable Register, Divisor Latch MSB (DLAB)
    I8250PORT_IIR,          // Interrupt Identification register (read only)
    I8250PORT_LCR,          // Line Control Register
    I8250PORT_MCR,          // Modem Control register
    I8250PORT_LSR,          // Line Status Register (read only)
    I8250PORT_MSR,          // Modem Status Register (read only)
    I8250PORT_SCR           // Scratch Register
} i8250Ports;

typedef enum { I8250REG_RBR, I8250REG_THR, I8250REG_DLL, I8250REG_IER, I8250REG_DLM, I8250REG_IIR,
               I8250REG_LCR, I8250REG_MCR, I8250REG_LSR, I8250REG_MSR, I8250REG_SCR } i8250Registers;

struct I8250
{
    I8250Transmit transmit;
    I8250Signal   signal;
    I8250Set      setDataBits;
    I8250Set      setStopBits;
    I8250Set      setParity;
    I8250Set      setRxReady;
    I8250Set      setDtr;
    I8250Set      setRts;
    I8250Get      getDtr;
    I8250Get      getRts;
    void* ref;

    UInt8 reg[11];

    UInt32        baudRate;
    BoardTimer*   timerBaudRate;   
    UInt32        timeBaudRate;
};

static int transmitDummy(void* ref, UInt8 value) {
    return 0;
}
static int signalDummy(void* ref) {
    return 0;
}
static void setDataBitsDummy(void* ref, int value) {
}
static void setStopBitsDummy(void* ref, int value) {
}
static void setParityDummy(void* ref, int value) {
}
static void setRxReadyDummy(void* ref, int status) {
}
static void setDtrDummy(void* ref, int status) {
}
static void setRtsDummy(void* ref, int status) {
}
static int getDtrDummy(void* ref) {
    return 0;
}
static int getRtsDummy(void* ref) {
    return 0;
}

#define I8250_RX_BUFFER_SIZE 256
#define I8250_RX_BUFFER_MASK (I8250_RX_BUFFER_SIZE - 1)
static UInt8 i8250RxBuffer[I8250_RX_BUFFER_SIZE];
static UInt16 i8250RxBufferHead;
static UInt16 i8250RxBufferTail;
static short int i8250RxBufferDataAvailable;

void i8250RxData(I8250* uart, UInt8 value)
{
    UInt16 unTempRxHead = 0;

    unTempRxHead = (i8250RxBufferHead + 1) & I8250_RX_BUFFER_MASK;

    if(unTempRxHead != i8250RxBufferTail) {
    	i8250RxBuffer[unTempRxHead] = value;
    	i8250RxBufferHead = unTempRxHead;
    	i8250RxBufferDataAvailable = 1;
    }
    else {
    	i8250RxBufferDataAvailable = -1;
    }
}

static short int i8250RxBufferGetByte(UInt8* value)
{
    UInt16 unTempRxTail = 0;

    if(i8250RxBufferHead == i8250RxBufferTail)
        return 0;

    unTempRxTail = (i8250RxBufferTail + 1) & I8250_RX_BUFFER_MASK;
    *value = i8250RxBuffer[unTempRxTail];
    i8250RxBufferTail = unTempRxTail;

    return 1;
}

static UInt16 i8250RxBufferGetLength(void)
{
    return ((i8250RxBufferHead - i8250RxBufferTail) & I8250_RX_BUFFER_MASK);
}

static void i8250RxBufferClear(void)
{
    i8250RxBufferHead = 0;
    i8250RxBufferTail = 0;
    i8250RxBufferDataAvailable = 0;
}

void i8250Receive(I8250* i8250, UInt8 value)
{
    i8250->reg[I8250REG_RBR] = value;
    if(i8250->reg[I8250REG_LSR] & LSR_DATA_READY)
        i8250->reg[I8250REG_LSR] |= LSR_OVERRUN_ERROR;
    i8250->reg[I8250REG_LSR] |= LSR_DATA_READY;
}

static void i8250Transmit(I8250* i8250, UInt8 value)
{
    if (i8250->transmit(i8250->ref, value))
        i8250->reg[I8250REG_LSR] &= ~LSR_TRANSMITTER_EMPTY;
}

UInt8 i8250Read(I8250* i8250, UInt16 port)
{
    UInt8 value = 0xff;

    switch (port) {
    case I8250PORT_RBR_THR_DLL:
        if (i8250->reg[I8250REG_LCR] & LCR_DIVISOR_LATCH_ACCESS_BIT)
            value = i8250->reg[I8250REG_DLL];
        else {
            value = i8250->reg[I8250REG_RBR];
            if(i8250->reg[I8250REG_LSR] & LSR_DATA_READY)
                i8250->reg[I8250REG_LSR] &= ~LSR_DATA_READY;
        }
        break;

    case I8250PORT_IER_DLM:
        if (i8250->reg[I8250REG_LCR] & LCR_DIVISOR_LATCH_ACCESS_BIT)
            value = i8250->reg[I8250REG_DLM];
        else
            value = i8250->reg[I8250REG_IER];
        break;

    case I8250PORT_IIR:
        value = i8250->reg[I8250REG_IIR];
        break;

    case I8250PORT_LCR:
        value = i8250->reg[I8250REG_LCR];
        break;

    case I8250PORT_MCR:
        value = i8250->reg[I8250REG_MCR];
        break;

    case I8250PORT_LSR:
//        i8250Receive(i8250);
        i8250->reg[I8250REG_LSR] |= LSR_TRANSMITTER_HOLDING_REGISTER_EMPTY;
        value = i8250->reg[I8250REG_LSR];
        if(i8250->reg[I8250REG_LSR] & 0x1f)
            i8250->reg[I8250REG_LSR] &= 0xe1; // Clear FE, PE and OE and BREAK bits
        break;

    case I8250PORT_MSR:
        if (i8250->reg[I8250REG_MCR] & MCR_LOOPBACK_TEST) {
	    value = i8250->reg[I8250REG_MCR] << 4;
	    i8250->reg[I8250REG_MSR] = (i8250->reg[I8250REG_MSR] ^ value) >> 4;
	    i8250->reg[I8250REG_MSR] |= value;
        }
        value = i8250->reg[I8250REG_MSR];
        i8250->reg[I8250REG_MSR] &= 0xf0; // Reset delta values
        break;

    case I8250PORT_SCR:
        value = i8250->reg[I8250REG_SCR];
        break;
    }

    return value;
}

void i8250Write(I8250* i8250, UInt16 port, UInt8 value)
{
    switch (port) {
    case I8250PORT_RBR_THR_DLL:
        if (i8250->reg[I8250REG_LCR] & LCR_DIVISOR_LATCH_ACCESS_BIT)
            i8250->reg[I8250REG_DLL] = value;
        else {
            i8250->reg[I8250REG_THR] = value;
            i8250Transmit(i8250, value);
        }
        break;

    case I8250PORT_IER_DLM:
        if (i8250->reg[I8250REG_LCR] & LCR_DIVISOR_LATCH_ACCESS_BIT)
            i8250->reg[I8250REG_DLM] = value;
        else
            i8250->reg[I8250REG_IER] = value;
        break;

    case I8250PORT_LCR:
        i8250->reg[I8250REG_LCR] = value;
        break;

    case I8250PORT_MCR:
        i8250->reg[I8250REG_MCR] = value;
        break;

    case I8250PORT_SCR:
        i8250->reg[I8250REG_SCR] = value;
        break;
    }
}

/* Counter for the buad rate generator */

static void i8250CounterOnTimer(I8250* i8250, UInt32 time)
{
    if (i8250RxBufferDataAvailable)
    {
        UInt8 value;

        if (i8250RxBufferGetByte(&value))
            i8250Receive(i8250, value);
    }
    i8250->timeBaudRate = time + boardFrequency() / i8250->baudRate;
    boardTimerAdd(i8250->timerBaudRate, i8250->timeBaudRate);
}

static void i8250CounterDestroy(I8250* i8250)
{
    boardTimerDestroy(i8250->timerBaudRate);
}

static void i8250CounterCreate(I8250* i8250, UInt32 frequency) 
{
    UInt16 divisor;

    divisor = i8250->reg[I8250REG_DLM]<<8 | i8250->reg[I8250REG_DLL];
    divisor = (divisor != 0) ? divisor : 1;

    i8250->timerBaudRate = boardTimerCreate(i8250CounterOnTimer, i8250);
    // Fixme: start + stop + parity bit
    i8250->baudRate = (frequency/16/divisor/10);
    if (i8250->baudRate > 0) {
        i8250->timeBaudRate = boardSystemTime() + boardFrequency() / i8250->baudRate;
        boardTimerAdd(i8250->timerBaudRate, i8250->timeBaudRate);
    }
}

void i8250SaveState(I8250* uart)
{
    SaveState* state = saveStateOpenForWrite("i8250");

    saveStateClose(state);
}

void i8250LoadState(I8250* uart)
{
    SaveState* state = saveStateOpenForRead("i8250");

    saveStateClose(state);
}

void i8250Reset(I8250* i8250)
{
    i8250->reg[I8250REG_IER] = 0;
    i8250->reg[I8250REG_IIR] = 1;
    i8250->reg[I8250REG_LCR] = 0;
    i8250->reg[I8250REG_MCR] = 0;
    i8250->reg[I8250REG_LSR] = 0x60;

    i8250RxBufferClear();
}

void i8250Destroy(I8250* uart) 
{
    i8250CounterDestroy(uart);
    free(uart);
}

I8250* i8250Create(UInt32 frequency, I8250Transmit transmit,    I8250Signal   signal,
                   I8250Set      setDataBits, I8250Set      setStopBits,
                   I8250Set      setParity,   I8250Set      setRxReady,
                   I8250Set      setDtr,      I8250Set      setRts,
                   I8250Get      getDtr,      I8250Get      getRts,
                   void* ref)
{
    I8250* i8250 = calloc(1, sizeof(I8250));
    
    i8250->transmit    = transmit    ? transmit    : transmitDummy;
    i8250->signal      = signal      ? signal      : signalDummy;
    i8250->setDataBits = setDataBits ? setDataBits : setDataBitsDummy;
    i8250->setStopBits = setStopBits ? setStopBits : setStopBitsDummy;
    i8250->setParity   = setParity   ? setParity   : setParityDummy;
    i8250->setRxReady  = setRxReady  ? setRxReady  : setRxReadyDummy;
    i8250->setDtr      = setDtr      ? setDtr      : setDtrDummy;
    i8250->setRts      = setRts      ? setRts      : setRtsDummy;
    i8250->getDtr      = getDtr      ? getDtr      : getDtrDummy;
    i8250->getRts      = getRts      ? getRts      : getRtsDummy;
    
    i8250->ref = ref;
    
    i8250CounterCreate(i8250, frequency);

    return i8250;
}
