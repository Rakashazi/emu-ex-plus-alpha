/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/PrinterIO.h,v $
**
** $Revision: 1.8 $
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
#ifndef PRINTER_IO_H
#define PRINTER_IO_H

#include "MsxTypes.h"

typedef struct PrinterIO PrinterIO;

typedef enum {PRN_NONE, PRN_SIMPL, PRN_FILE, PRN_HOST } PrinterType;

PrinterIO* printerIOCreate(void);
void printerIODestroy(PrinterIO* printerIO);

void printerIOWrite(PrinterIO* printerIO, UInt8 value);
int  printerIOGetStatus(PrinterIO* printerIO);
int  printerIODoStrobe(PrinterIO* printerIO);

void printerIoSetType(PrinterType type, const char* fileName);

#endif
