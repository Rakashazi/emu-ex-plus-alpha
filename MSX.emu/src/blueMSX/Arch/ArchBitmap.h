/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchBitmap.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:38 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
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
#ifndef ARCH_BITMAP_H
#define ARCH_BITMAP_H

typedef struct ArchBitmap ArchBitmap;

ArchBitmap* archBitmapCreate(int width, int height);
ArchBitmap* archBitmapCreateFromFile(const char* filename);
ArchBitmap* archBitmapCreateFromId(int id);
void archBitmapDestroy(ArchBitmap* bm);
int archBitmapGetWidth(ArchBitmap* bm);
int archBitmapGetHeight(ArchBitmap* bm);
void archBitmapDraw(ArchBitmap* bm, void* dcDest, int xDest, int yDest, 
                    int xSrc, int ySrc, int width, int height);
void archBitmapCopy(ArchBitmap* dst, int xDest, int yDest, 
                    ArchBitmap* src, int xSrc, int ySrc, int width, int height);

int archRGB(int r, int g, int b);

#endif
