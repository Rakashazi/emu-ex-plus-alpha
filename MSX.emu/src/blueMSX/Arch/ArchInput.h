/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchInput.h,v $
**
** $Revision: 1.7 $
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
#ifndef ARCH_INPUT_H
#define ARCH_INPUT_H

#include "MsxTypes.h"

typedef enum { AM_DISABLE = 0, AM_ENABLE_MOUSE, AM_ENABLE_LASER } AmEnableMode;

void  archMouseEmuEnable(AmEnableMode mode);
void archMouseSetForceLock(int lock);
void archMouseGetState(int* dx, int* dy);
int  archMouseGetButtonState(int checkAlways);

int   archPollEvent();

void  archPollInput();
void  archKeyboardSetSelectedKey(int keyCode);
char* archGetSelectedKey();
char* archGetMappedKey();
int   archKeyboardIsKeyConfigured(int msxKeyCode);
int   archKeyboardIsKeySelected(int msxKeyCode);
char* archKeyconfigSelectedKeyTitle();
char* archKeyconfigMappedToTitle();
char* archKeyconfigMappingSchemeTitle();

#endif
