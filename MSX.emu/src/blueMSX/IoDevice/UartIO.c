/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/UartIO.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-31 19:42:20 $
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
#include <stdlib.h>
#include <string.h>
#include "UartIO.h"
#include "ArchUart.h"
#include "DAC.h"
#include "Board.h"

typedef struct UartIO {
    UartType type;
    FILE* file;
    int  uartReady;

    void (*recvCallback)(UInt8);
};

static UartType theUartType = UART_NONE;
static UartIO* theUartIO = NULL;
static char theFileName[512] = { 0 };


static void setType(UartIO* uartIO)
{
    uartIO->type = theUartType;

    switch (uartIO->type) {
    case UART_HOST:
        uartIO->uartReady = archUartCreate(uartIO->recvCallback);
        break;
    case UART_FILE:
        uartIO->file = fopen(theFileName, "w+");
        break;
    }
}

static void removeType(UartIO* uartIO)
{
    switch (uartIO->type) {
    case UART_HOST:
        archUartDestroy();
        uartIO->uartReady = 0;
        break;
    case UART_FILE:
        fclose(uartIO->file);
        break;
    }
}

void uartIOTransmit(UartIO* uartIO, UInt8 value)
{
    switch (uartIO->type) {
    case UART_HOST:
        archUartTransmit(value);
        break;
    case UART_FILE:
        fwrite(&value, 1, 1, uartIO->file);
        break;
    }
}

int uartIOGetStatus(UartIO* uartIO) 
{
    switch (uartIO->type) {
    case UART_HOST:
        return uartIO->uartReady;
    case UART_FILE:
        return uartIO->file != NULL;
    }
    return 0;
}

UartIO* uartIOCreate(void (*recvCallback)(UInt8))
{
    UartIO* uartIO = calloc(1, sizeof(UartIO));

    setType(uartIO);

    theUartIO = uartIO;

    uartIO->recvCallback = recvCallback;

    return uartIO;
}

void uartIODestroy(UartIO* uartIO)
{
    removeType(uartIO);

    free(uartIO);

    theUartIO = NULL;
}

void uartIoSetType(UartType type, const char* fileName)
{   
    theUartType = type;

    strcpy(theFileName, fileName);
    
    if (theUartIO == NULL) {
        return;
    }

    removeType(theUartIO);
    setType(theUartIO);
}
