/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/RTC.c,v $
**
** $Revision: 1.8 $
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
#include "RTC.h"
#include "IoPort.h"
#include "Board.h"
#include "SaveState.h"
#include "DebugDeviceManager.h"
#include "Language.h"
#include "ziphelper.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const UInt8 mask[4][13] = {
    { 0x0f, 0x07, 0x0f, 0x07, 0x0f, 0x03, 0x07, 0x0f, 0x03, 0x0f, 0x01, 0x0f, 0x0f },
    { 0x00, 0x00, 0x0f, 0x07, 0x0f, 0x03, 0x07, 0x0f, 0x03, 0x00, 0x01, 0x03, 0x00 },
    { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f },
    { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f }
};

static const int daysInMonth[4][12] = {
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

#define MAX_SCALED_TIME  (((UInt64)1) << (32 + 14))

#define MODE_BLOCKSELECT 0x03
#define MODE_ALARMENABLE 0x04
#define MODE_TIMERENABLE 0x08

#define TEST_SECONDS     0x01
#define TEST_MINUTES     0x02
#define TEST_DAYS        0x04
#define TEST_YEARS       0x08

#define RESET_ALARM      0x01
#define RESET_FRACTION   0x02

#define OFFSETOF(s, a) ((int)(&((s*)0)->a))

struct RTC {
    int debugHandle;

    char cmosName[512];
    UInt8 modeReg;
    UInt8 testReg;
    UInt8 resetReg;
    UInt8 registers[4][13]; 
    UInt32 refTime;
    UInt32 refFrag;
    int fraction;
    int seconds;
    int minutes;
    int hours;
    int dayWeek;
    int days;
    int months;
    int years;
    int leapYear;
    
    UInt8 latch;
};

void rtcLoadState(RTC* rtc)
{
    SaveState* state = saveStateOpenForRead("rtc");

    rtc->refTime  =         saveStateGet(state, "refTime",  boardSystemTime());
    rtc->refFrag  =         saveStateGet(state, "refFrag",  0);
    rtc->fraction =         saveStateGet(state, "fraction", 0);
    rtc->seconds  =         saveStateGet(state, "seconds",  0);
    rtc->minutes  =         saveStateGet(state, "minutes",  0);
    rtc->hours    =         saveStateGet(state, "hours",    0);
    rtc->dayWeek  =         saveStateGet(state, "dayWeek",  0);
    rtc->days     =         saveStateGet(state, "days",     0);
    rtc->months   =         saveStateGet(state, "months",   0);
    rtc->years    =         saveStateGet(state, "years",    0);
    rtc->leapYear =         saveStateGet(state, "leapYear", 0);
    rtc->latch    = (UInt8) saveStateGet(state, "latch",    0);

    saveStateClose(state);
}

void rtcSaveState(RTC* rtc)
{
    SaveState* state = saveStateOpenForWrite("rtc");

    saveStateSet(state, "refTime",  rtc->refTime);
    saveStateSet(state, "refFrag",  rtc->refFrag);
    saveStateSet(state, "fraction", rtc->fraction);
    saveStateSet(state, "seconds",  rtc->seconds);
    saveStateSet(state, "minutes",  rtc->minutes);
    saveStateSet(state, "hours",    rtc->hours);
    saveStateSet(state, "dayWeek",  rtc->dayWeek);
    saveStateSet(state, "days",     rtc->days);
    saveStateSet(state, "months",   rtc->months);
    saveStateSet(state, "years",    rtc->years);
    saveStateSet(state, "leapYear", rtc->leapYear);
    saveStateSet(state, "latch",    rtc->latch);

    saveStateClose(state);
}

static void rtcSetTime(RTC* rtc)
{
    rtc->seconds  = rtc->registers[0][0]  + 10 * rtc->registers[0][1];
    rtc->minutes  = rtc->registers[0][2]  + 10 * rtc->registers[0][3];
    rtc->hours    = rtc->registers[0][4]  + 10 * rtc->registers[0][5];
    rtc->dayWeek  = rtc->registers[0][6];
    rtc->days     = rtc->registers[0][7]  + 10 * rtc->registers[0][8]  - 1;
    rtc->months   = rtc->registers[0][9]  + 10 * rtc->registers[0][10] - 1;
    rtc->years    = rtc->registers[0][11] + 10 * rtc->registers[0][12];
    rtc->leapYear = rtc->registers[1][11];

    if (!rtc->registers[1][10]) {
        if (rtc->hours >= 20) {
            rtc->hours = (rtc->hours - 20) + 12;
        }
    }
}
    
static void rtcSetRegisters(RTC* rtc)
{
    int hours = rtc->hours;
    if (!rtc->registers[1][10]) {
        if (hours >= 12) {
            hours = (hours - 12) + 20;
        }
    }
    
    rtc->registers[0][0]   =  rtc->seconds % 10;
    rtc->registers[0][1]   =  rtc->seconds / 10;
    rtc->registers[0][2]   =  rtc->minutes % 10;
    rtc->registers[0][3]   =  rtc->minutes / 10;
    rtc->registers[0][4]   =  hours % 10;
    rtc->registers[0][5]   =  hours / 10;
    rtc->registers[0][6]   =  rtc->dayWeek;
    rtc->registers[0][7]   = (rtc->days + 1) % 10;
    rtc->registers[0][8]   = (rtc->days + 1) / 10;
    rtc->registers[0][9]   = (rtc->months + 1) % 10;
    rtc->registers[0][10]  = (rtc->months + 1) / 10;
    rtc->registers[0][11]  =  rtc->years % 10;
    rtc->registers[0][12]  =  rtc->years / 10;
    rtc->registers[1][11] =  rtc->leapYear;
}

static void rtcUpdateRegs(RTC* rtc)
{
    int carryDays;
    int carryYears;
    UInt64 elapsed;
    UInt32 elapsedTime;
    UInt32 systemTime = boardSystemTime();

    elapsed      = 16384 * (UInt64)(systemTime - rtc->refTime) + rtc->refFrag;
    rtc->refTime = systemTime;
    rtc->refFrag = (UInt32)(elapsed % boardFrequency());
    elapsedTime  = (UInt32)(elapsed / boardFrequency());
    
    rtc->fraction += (rtc->modeReg & MODE_TIMERENABLE) ? elapsedTime : 0;
    rtc->seconds  += (rtc->testReg & TEST_SECONDS) ? elapsedTime : rtc->fraction / 16384; 
    rtc->fraction %= 16384;
    rtc->minutes  += (rtc->testReg & TEST_MINUTES) ? elapsedTime : rtc->seconds / 60; 
    rtc->seconds  %= 60;
    rtc->hours    += rtc-> minutes / 60;   
    rtc->minutes  %= 60;
    carryDays      = (rtc->testReg & TEST_DAYS) ? elapsedTime : rtc->hours / 24; 
    rtc->days     += carryDays;
    rtc->hours    %= 24;
    rtc->dayWeek   = (rtc->dayWeek + carryDays) % 7;
    
    while (rtc->days >= daysInMonth[rtc->leapYear][rtc->months]) {
        rtc->days -= daysInMonth[rtc->leapYear][rtc->months];
        rtc->months++;
    }

    carryYears    = (rtc->testReg & TEST_YEARS) ? elapsedTime : rtc->months / 12;
    rtc->years    = (rtc->years + carryYears) % 100; 
    rtc->months  %= 12;
    rtc->leapYear = (rtc->leapYear + carryYears) % 4;
    
    rtcSetRegisters(rtc);
}

static UInt8 rtcReadData(RTC* rtc, UInt16 ioPort)
{
    int block;

    switch (rtc->latch) {
    case 0x0d:
        return rtc->modeReg | 0xf0;

    case 0x0e:
    case 0x0f:
        return 0xff;
    }

    block = rtc->modeReg & MODE_BLOCKSELECT;

    if (block == 0) {
        rtcUpdateRegs(rtc);
    }

    return (rtc->registers[block][rtc->latch] & mask[block][rtc->latch]) | 0xf0;
}

static UInt8 rtcPeekData(RTC* rtc, UInt16 ioPort)
{
    int block;

    switch (rtc->latch) {
    case 0x0d:
        return rtc->modeReg | 0xf0;

    case 0x0e:
    case 0x0f:
        return 0xff;
    }

    block = rtc->modeReg & MODE_BLOCKSELECT;

    return (rtc->registers[block][rtc->latch] & mask[block][rtc->latch]) | 0xf0;
}

static void rtcWriteData(RTC* rtc, UInt16 ioPort, UInt8 value)
{
    int block;

    switch (rtc->latch) {
    case 0x0d:
        rtcUpdateRegs(rtc);
        rtc->modeReg = value;
        return;

    case 0x0e:
        rtcUpdateRegs(rtc);
        rtc->testReg = value;
        return;

    case 0x0f:
        rtc->resetReg = value;

        if (value & RESET_ALARM) {
            int i;

            for (i = 2; i <= 8; i++) {
                rtc->registers[1][i] = 0;
            }
        }
        if (value & RESET_FRACTION) {
            rtc->fraction = 0;
        }
        return;
    }

    block = rtc->modeReg & MODE_BLOCKSELECT;

    if (block == 0) {
        rtcUpdateRegs(rtc);
    }

    rtc->registers[block][rtc->latch] = value & mask[block][rtc->latch];

    if (block == 0) {
        rtcSetTime(rtc);
    }
}

static void rtcWriteLatch(RTC* rtc, UInt16 ioPort, UInt8 value)
{
    rtc->latch = value & 0x0f;
}

static void getDebugInfo(RTC* rtc, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevRtc(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0xb4, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 1, 0xb5, DBG_IO_READWRITE, rtcPeekData(rtc, 0xb5));
}

RTC* rtcCreate(int enable, char* cmosName)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RTC* rtc = (RTC*)calloc(1, sizeof(RTC));

    rtc->modeReg = MODE_TIMERENABLE;

    if (cmosName != NULL) {
        struct tm* tm;
        time_t t;
        FILE* file;

        strcpy(rtc->cmosName, cmosName);

		#ifndef NDEBUG
        fprintf(stderr, "loading cmos %s\n", cmosName);
		#endif
        file = fopenHelper(cmosName, "r");

        if (file != NULL) {
            fread(rtc->registers, 1, sizeof(rtc->registers), file);
            fclose(file);
        }

        t = time(NULL);
        tm = localtime(&t);

        rtc->fraction = 0;
        rtc->seconds  = tm->tm_sec;
        rtc->minutes  = tm->tm_min;
        rtc->hours    = tm->tm_hour;
        rtc->dayWeek  = tm->tm_wday;
        rtc->days     = tm->tm_mday - 1;
        rtc->months   = tm->tm_mon;
        rtc->years    = tm->tm_year - 80;
        rtc->leapYear = tm->tm_year % 4;
    }

    if (enable) {
        rtc->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevRtc(), &dbgCallbacks, rtc);
        
        ioPortRegister(0xb4, NULL,        rtcWriteLatch, rtc);
        ioPortRegister(0xb5, rtcReadData, rtcWriteData,  rtc);
    }

    rtcUpdateRegs(rtc);

    return rtc;
}

void rtcDestroy(RTC* rtc)
{
    debugDeviceUnregister(rtc->debugHandle);

    ioPortUnregister(0xb4);
    ioPortUnregister(0xb5);

    if (rtc->cmosName[0]) {
		#ifndef NDEBUG
        fprintf(stderr, "saving cmos %s\n", rtc->cmosName);
		#endif
        FILE* file = fopenHelper(rtc->cmosName, "w");

        if(file != NULL) {
					fwrite(rtc->registers, 1, sizeof(rtc->registers), file);
					fclose(file);
        }
    }

    free(rtc);
}
