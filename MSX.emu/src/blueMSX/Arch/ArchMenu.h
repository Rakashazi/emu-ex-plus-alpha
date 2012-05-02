/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchMenu.h,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-03-30 18:38:39 $
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
#ifndef ARCH_MENU_H
#define ARCH_MENU_H

void archUpdateMenu(int show);

void archShowMenuSpecialCart1(int x, int y);
void archShowMenuSpecialCart2(int x, int y);
void archShowMenuReset(int x, int y);
void archShowMenuHelp(int x, int y);
void archShowMenuRun(int x, int y);
void archShowMenuFile(int x, int y);
void archShowMenuCart1(int x, int y);
void archShowMenuCart2(int x, int y);
void archShowMenuHarddisk(int x, int y);
void archShowMenuDiskA(int x, int y);
void archShowMenuDiskB(int x, int y);
void archShowMenuCassette(int x, int y);
void archShowMenuPrinter(int x, int y);
void archShowMenuZoom(int x, int y);
void archShowMenuOptions(int x, int y);
void archShowMenuTools(int x, int y);
void archShowMenuJoyPort1(int x, int y);
void archShowMenuJoyPort2(int x, int y);

#endif
