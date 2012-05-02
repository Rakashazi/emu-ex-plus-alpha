/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Emulator/CommandLine.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008/03/30 18:38:40 $
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
#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "Properties.h"

int emuCheckResetArgument(char* szLine);
char* emuCheckThemeArgument(char* szLine);
void emuCheckFullscreenArgument(Properties* properties, char* szLine);
int emuTryStartWithArguments(Properties* properties, char* cmdLine, char *gamedir);

#endif

