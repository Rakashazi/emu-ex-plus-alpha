/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/VideoManager.h,v $
**
** $Revision: 1.10 $
**
** $Date: 2008-03-30 18:38:47 $
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
#ifndef VIDEO_MANAGER_H
#define VIDEO_MANAGER_H

#include "MsxTypes.h"
#include "FrameBuffer.h"

typedef struct {
    void (*enable)(void*);
    void (*disable)(void*);
} VideoCallbacks;

typedef enum {
    VIDEO_INTERNAL = 1,
    VIDEO_MIX      = 2,
    VIDEO_EXTERNAL = 4,
    VIDEO_NONE     = 8,
    VIDEO_MASK_ALL = VIDEO_INTERNAL | VIDEO_MIX | VIDEO_EXTERNAL | VIDEO_NONE
} VideoMode;

int videoManagerGetCount();
int videoManagerGetActive();
void videoManagerSetActive(int index);
void videoManagerSetMode(int index, VideoMode videoMode, VideoMode modeMask);
int videoManagerIsActive(int index);
char* videoManagerGetName(int index);

void videoManagerReset();

void videoManagerLoadState();
void videoManagerSaveState();

int videoManagerRegister(const char* name, FrameBufferData* frameBuffer, 
                        VideoCallbacks* callbacks, void* ref);
void videoManagerUnregister(int handle);

#endif
