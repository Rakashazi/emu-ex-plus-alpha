/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/PrinterIO.c,v $
**
** $Revision: 1.12 $
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
#include "PrinterIO.h"
#include "ArchPrinter.h"
#include "DAC.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>

typedef struct PrinterIO {
    PrinterType type;
    DAC* dac;
    FILE* file;
    int  printerReady;
};

static PrinterType thePrinterType = PRN_NONE;
static PrinterIO* thePrinterIO = NULL;
static char theFileName[512] = { 0 };


static void setType(PrinterIO* printerIO)
{
    printerIO->type = thePrinterType;

    switch (printerIO->type) {
    case PRN_HOST:
        printerIO->printerReady = archPrinterCreate();
        break;
    case PRN_FILE:
        printerIO->file = fopen(theFileName, "w+");
        break;
    case PRN_SIMPL:
        printerIO->dac = dacCreate(boardGetMixer(), DAC_MONO);
        break;
    }
}

static void removeType(PrinterIO* printerIO)
{
    switch (printerIO->type) {
    case PRN_HOST:
        archPrinterDestroy();
        printerIO->printerReady = 0;
        break;
    case PRN_FILE:
        fclose(printerIO->file);
        break;
    case PRN_SIMPL:
        dacDestroy(printerIO->dac);
        break;
    }
}

void printerIOWrite(PrinterIO* printerIO, UInt8 value)
{
    switch (printerIO->type) {
    case PRN_HOST:
        archPrinterWrite(value);
        break;
    case PRN_FILE:
        fwrite(&value, 1, 1, printerIO->file);
        break;
    case PRN_SIMPL:
        dacWrite(printerIO->dac, DAC_CH_MONO, value);
        break;
    }
}

int printerIOGetStatus(PrinterIO* printerIO) 
{
    switch (printerIO->type) {
    case PRN_HOST:
        return printerIO->printerReady;
    case PRN_FILE:
        return printerIO->file != NULL;
    case PRN_SIMPL:
        return 1;
    }
    return 0;
}

int  printerIODoStrobe(PrinterIO* printerIO)
{
    return printerIO->type != PRN_SIMPL;
}

PrinterIO* printerIOCreate(void)
{
    PrinterIO* printerIO = calloc(1, sizeof(PrinterIO));

    setType(printerIO);

    thePrinterIO = printerIO;

    return printerIO;
}

void printerIODestroy(PrinterIO* printerIO)
{
    removeType(printerIO);

    free(printerIO);

    thePrinterIO = NULL;
}

void printerIoSetType(PrinterType type, const char* fileName)
{   
    thePrinterType = type;

    strcpy(theFileName, fileName);
    
    if (thePrinterIO == NULL) {
        return;
    }

    removeType(thePrinterIO);
    setType(thePrinterIO);
}
