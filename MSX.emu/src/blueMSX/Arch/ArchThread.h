/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Arch/ArchThread.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2008/03/30 18:38:39 $
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
#ifndef ARCH_THREAD_H
#define ARCH_THREAD_H

#define THREAD_PRIO_NORMAL 0
#define THREAD_PRIO_HIGH   1

void* archThreadCreate(void (*entryPoint)(), int priority);
void* archThreadCreateEx(void (*entryPoint)(), int priority, int stacksize);
void  archThreadJoin(void* thread, int timeout);
void  archThreadDestroy(void* thread);

void archThreadSleep(int milliseconds);

#endif
