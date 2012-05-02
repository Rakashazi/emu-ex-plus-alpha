/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Led.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:40 $
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
#include "Led.h"

static int ledCapslock = 0;
static int ledKana     = 0;
static int ledTurboR   = 0;
static int ledPause    = 0;
static int ledRensha   = 0;
static int ledFdd1     = 0;
static int ledFdd2     = 0;
static int ledHd       = 0;
static int ledCas      = 0;

void ledSetAll(int enable) {
    enable = enable ? 1 : 0;
    
    ledCapslock = enable;
    ledKana     = enable;
    ledTurboR   = enable;
    ledPause    = enable;
    ledRensha   = enable;
    ledFdd1     = enable;
    ledFdd2     = enable;
    ledHd       = enable;
    ledCas      = enable;
}

void ledSetCapslock(int enable) {
    ledCapslock = enable ? 1 : 0;
}

int ledGetCapslock() {
    return ledCapslock;
}

void ledSetKana(int enable) {
    ledKana = enable ? 1 : 0;
}

int ledGetKana() {
    return ledKana;
}

void ledSetTurboR(int enable) {
    ledTurboR = enable ? 1 : 0;
}

int ledGetTurboR() {
    return ledTurboR;
}

void ledSetPause(int enable) {
    ledPause = enable ? 1 : 0;
}

int ledGetPause() {
    return ledPause;
}

void ledSetRensha(int enable) {
    ledRensha = enable ? 1 : 0;
}

int ledGetRensha() {
    return ledRensha;
}

void ledSetFdd1(int enable) {
    ledFdd1 = enable ? 1 : 0;
}

int ledGetFdd1() {
    return ledFdd1;
}

void ledSetFdd2(int enable) {
    ledFdd2 = enable ? 1 : 0;
}

int ledGetFdd2() {
    return ledFdd2;
}

void ledSetHd(int enable) {
    ledHd = enable ? 1 : 0;
}

int ledGetHd() {
    return ledHd;
}

void ledSetCas(int enable) {
    ledCas = enable ? 1 : 0;
}

int ledGetCas() {
    return ledCas;
}

