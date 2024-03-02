/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "board"

#include <imagine/logger/logger.h>
#include <assert.h>
#include <string.h>
#include <emuframework/Option.hh>
#include "MainApp.hh"

extern "C"
{
	#include <blueMSX/Board/Board.h>
	#include <blueMSX/IoDevice/Disk.h>
	#include <blueMSX/Memory/MegaromCartridge.h>
}

using namespace EmuEx;

namespace EmuEx
{
bool fdcActive = 0;
}

static HdType hdType[MAX_HD_COUNT]{};
RomType currentRomType[2]{};
static BoardTimer* fdcTimer{};
static BoardTimer* mixerTimer;
static constexpr unsigned mixerSyncHz = 120;

Mixer* boardGetMixer()
{
	assert(mixer);
    return mixer;
}

UInt8 boardCaptureUInt8(UInt8 logId, UInt8 value)
{
    return value;
}

int boardGetY8950Oversampling() { return 1; }
int boardGetYm2413Oversampling() { return 1; }
int boardGetMoonsoundOversampling() { return 1; }
int boardGetY8950Enable() { return 1; }
int boardGetYm2413Enable() { return 1; }
int boardGetMoonsoundEnable() { return 1; }

void boardChangeDiskette(int driveId, char* fileName, const char* fileInZipFile)
{
    diskChange(driveId, fileName, fileInZipFile);
}

static void onFdcDone(void* ref, UInt32 time)
{
    fdcActive = 0;
    logMsg("ended FDC activity");
}

void boardSetFdcActive()
{
	auto &sys = static_cast<MsxSystem&>(gSystem());
	if(sys.optionSkipFdcAccess)
	{
		if(!fdcActive)
			logMsg("FDC active");
		boardTimerAdd(fdcTimer, boardSystemTime() + (UInt32)((UInt64)300 * boardFrequency() / 1000));
		fdcActive = 1;
	}
}

static void onMixerSync(void* mixer, UInt32 time)
{
    mixerSync((Mixer*)mixer);
    boardTimerAdd(mixerTimer, boardSystemTime() + boardFrequency() / mixerSyncHz);
}

void boardSetDataBus(UInt8 value, UInt8 defValue, int useDef)
{
    assert(boardInfo.setDataBus);
    boardInfo.setDataBus(boardInfo.cpuRef, value, defValue, useDef);
}

UInt32* boardSysTime;
/*static UInt64 boardSysTime64;
static UInt32 oldTime;
static const UInt64 HIRES_CYCLES_PER_LORES_CYCLE = 100000;*/

BoardType boardGetType()
{
	assert(machine);
    return BoardType(machine->board.type & BOARD_MASK);
}

/*UInt64 boardFrequency64()
{
	return HIRES_CYCLES_PER_LORES_CYCLE * boardFrequency();
}

UInt64 boardSystemTime64()
{
    UInt32 currentTime = boardSystemTime();
    boardSysTime64 += HIRES_CYCLES_PER_LORES_CYCLE * (currentTime - oldTime);
    oldTime = currentTime;
    return boardSysTime64;
}*/

UInt32 boardCalcRelativeTimeout(UInt32 timerFrequency, UInt32 nextTimeout)
{
    /*UInt64 currentTime = boardSystemTime64();
    UInt64 frequency   = boardFrequency64() / timerFrequency;
    currentTime = frequency * (currentTime / frequency);
    return (UInt32)((currentTime + nextTimeout * frequency) / HIRES_CYCLES_PER_LORES_CYCLE);*/
	UInt32 currentTime = boardSystemTime();
	UInt32 frequency   = boardFrequency() / timerFrequency;
	currentTime = frequency * (currentTime / frequency);
	return currentTime + nextTimeout * frequency;
}

int pendingInt;

UInt8* boardGetRamPage(int page)
{
    if (boardInfo.getRamPage == NULL) {
        return NULL;
    }
    return boardInfo.getRamPage(page);
}

void boardSetInt(UInt32 irq)
{
    pendingInt |= irq;
    boardInfo.setInt(boardInfo.cpuRef);
}

void boardClearInt(UInt32 irq)
{
    pendingInt &= ~irq;
    if (pendingInt == 0) {
        boardInfo.clearInt(boardInfo.cpuRef);
    }
}

UInt32 boardGetInt(UInt32 irq)
{
    return pendingInt & irq;
}

struct BoardTimer// : public MemAlloc<BoardTimer>
{
    BoardTimer*  next;
    BoardTimer*  prev;
    BoardTimerCb callback;
    void*        ref;
    UInt32       timeout;
};

static BoardTimer* timerList = NULL;
static UInt32 timeAnchor;
static int timeoutCheckBreak;

BoardTimer* boardTimerCreate(BoardTimerCb callback, void* ref)
{
	BoardTimer* timer = new BoardTimer;
    timer->next     = timer;
    timer->prev     = timer;
    timer->callback = callback;
    timer->ref      = ref ? ref : timer;
    timer->timeout  = 0;
	return timer;
}

void boardTimerDestroy(BoardTimer* timer)
{
    boardTimerRemove(timer);

    delete timer;
}

void boardTimerAdd(BoardTimer* timer, UInt32 timeout)
{
	const unsigned TEST_TIME = 0x7fffffff;
    UInt32 currentTime = boardSystemTime();
    BoardTimer* refTimer;
    BoardTimer* next = timer->next;
    BoardTimer* prev = timer->prev;

    // Remove current timer
    next->prev = prev;
    prev->next = next;

    timerList->timeout = currentTime + TEST_TIME;

    refTimer = timerList->next;

    if (timeout - timeAnchor - TEST_TIME < currentTime - timeAnchor - TEST_TIME) {
        timer->next = timer;
        timer->prev = timer;

        // Time has already expired
        return;
    }

    while (timeout - timeAnchor > refTimer->timeout - timeAnchor) {
        refTimer = refTimer->next;
    }

    timer->timeout       = timeout;
    timer->next          = refTimer;
    timer->prev          = refTimer->prev;
    refTimer->prev->next = timer;
    refTimer->prev       = timer;

    boardInfo.setCpuTimeout(boardInfo.cpuRef, timerList->next->timeout);
}

void boardTimerRemove(BoardTimer* timer)
{
    BoardTimer* next = timer->next;
    BoardTimer* prev = timer->prev;

    next->prev = prev;
    prev->next = next;

    timer->next = timer;
    timer->prev = timer;
}

const char* boardGetBaseDirectory()
{
	return EmuEx::gSystem().contentSaveDirectoryPtr();
}

void boardOnBreakpoint(UInt16 pc) { }

int boardInsertExternalDevices()
{
	//logMsg("boardInsertExternalDevices");
	return 1;
}

int boardRemoveExternalDevices()
{
	return 1;
}

int boardGetVideoAutodetect()
{
    return 1;
}

void boardTimerCheckTimeout(void* dummy)
{
	const unsigned MAX_TIME = (2 * 1368 * 313);
    UInt32 currentTime = boardSystemTime();
    timerList->timeout = currentTime + MAX_TIME;

    timeoutCheckBreak = 0;
    while (!timeoutCheckBreak) {
        BoardTimer* timer = timerList->next;
        if (timer == timerList) {
            return;
        }
        if (timer->timeout - timeAnchor > currentTime - timeAnchor) {
            break;
        }

        boardTimerRemove(timer);
        timer->callback(timer->ref, timer->timeout);
    }

    timeAnchor = boardSystemTime();

    boardInfo.setCpuTimeout(boardInfo.cpuRef, timerList->next->timeout);
}

void boardSetMachine(Machine* machine)
{
    int i;
    int hdIndex = FIRST_INTERNAL_HD_INDEX;

    // Update HD info
    for (i = FIRST_INTERNAL_HD_INDEX; i < MAX_HD_COUNT; i++) {
        hdType[i] = HD_NONE;
    }
    for (i = 0; i < machine->slotInfoCount; i++) {
        switch (machine->slotInfo[i].romType) {
        case ROM_SUNRISEIDE:  hdType[hdIndex++] = HD_SUNRISEIDE; break;
        case ROM_BEERIDE:     hdType[hdIndex++] = HD_BEERIDE;    break;
        case ROM_GIDE:        hdType[hdIndex++] = HD_GIDE;       break;
        case ROM_SVI328RSIDE: hdType[hdIndex++] = HD_RSIDE;      break;
        case ROM_NOWIND:      hdType[hdIndex++] = HD_NOWIND;     break;
        case SRAM_MEGASCSI:   hdType[hdIndex++] = HD_MEGASCSI;   break;
        case SRAM_WAVESCSI:   hdType[hdIndex++] = HD_WAVESCSI;   break;
        case ROM_GOUDASCSI:   hdType[hdIndex++] = HD_GOUDASCSI;  break;
        }
    }

    // Update RAM info
    /*boardRamSize  = 0;
    boardVramSize = machine->video.vramSize;

    for (i = 0; i < machine->slotInfoCount; i++) {
        if (machine->slotInfo[i].romType == RAM_1KB_MIRRORED) {
            boardRamSize = 0x400;
        }
        if (machine->slotInfo[i].romType == RAM_2KB_MIRRORED) {
            boardRamSize = 0x800;
        }
    }

    if (boardRamSize == 0) {
        for (i = 0; i < machine->slotInfoCount; i++) {
            if (machine->slotInfo[i].romType == RAM_NORMAL || machine->slotInfo[i].romType == RAM_MAPPER) {
                boardRamSize = 0x2000 * machine->slotInfo[i].pageCount;
            }
        }
    }

    boardType = machine->board.type;
    PatchReset(boardType);

    joystickPortUpdateBoardInfo();*/
}

HdType boardGetHdType(int hdIndex)
{
    if (hdIndex < 0 || hdIndex >= MAX_HD_COUNT) {
        return HD_NONE;
    }
    return hdType[hdIndex];
}

int boardChangeCartridge(int cartNo, RomType romType, const char* cart, const char* cartZip)
{
    /*if (romType == ROM_UNKNOWN) {
        int size;
        UInt8* buf = romLoad(cart, cartZip, &size);
        if (buf != NULL) {
            MediaType* mediaType = mediaDbGuessRom(buf, size);
            romType = mediaDbGetRomType(mediaType);
            free(buf);
        }
    }*/

    /*if (boardDeviceInfo != NULL) {
        boardDeviceInfo->carts[cartNo].inserted = cart != NULL;
        boardDeviceInfo->carts[cartNo].type = romType;

        strcpy(boardDeviceInfo->carts[cartNo].name, cart ? cart : "");
        strcpy(boardDeviceInfo->carts[cartNo].inZipName, cartZip ? cartZip : "");
    }*/

    /*useRom     -= romTypeIsRom(currentRomType[cartNo]);
    useMegaRom -= romTypeIsMegaRom(currentRomType[cartNo]);
    useMegaRam -= romTypeIsMegaRam(currentRomType[cartNo]);
    useFmPac   -= romTypeIsFmPac(currentRomType[cartNo]);*/

	if(hdType[cartNo] != HD_NONE) // eject HDs if previously HD controller
	{
		diskChange(diskGetHdDriveId(cartNo, 0), 0, 0);
		diskChange(diskGetHdDriveId(cartNo, 1), 0, 0);
		hdName[cartNo * 2] = {};
		hdName[(cartNo * 2) + 1] = {};
	}

    hdType[cartNo] = HD_NONE;
    currentRomType[cartNo] = ROM_UNKNOWN;

    if (cart != NULL) {
        currentRomType[cartNo] = romType;
        /*useRom     += romTypeIsRom(romType);
        useMegaRom += romTypeIsMegaRom(romType);
        useMegaRam += romTypeIsMegaRam(romType);
        useFmPac   += romTypeIsFmPac(romType);*/
        if (currentRomType[cartNo] == ROM_SUNRISEIDE)   hdType[cartNo] = HD_SUNRISEIDE;
        if (currentRomType[cartNo] == ROM_BEERIDE)      hdType[cartNo] = HD_BEERIDE;
        if (currentRomType[cartNo] == ROM_GIDE)         hdType[cartNo] = HD_GIDE;
        if (currentRomType[cartNo] == ROM_SVI328RSIDE)  hdType[cartNo] = HD_RSIDE;
        if (currentRomType[cartNo] == ROM_NOWIND)       hdType[cartNo] = HD_NOWIND;
        if (currentRomType[cartNo] == SRAM_MEGASCSI)    hdType[cartNo] = HD_MEGASCSI;
        if (currentRomType[cartNo] == SRAM_MEGASCSI128) hdType[cartNo] = HD_MEGASCSI;
        if (currentRomType[cartNo] == SRAM_MEGASCSI256) hdType[cartNo] = HD_MEGASCSI;
        if (currentRomType[cartNo] == SRAM_MEGASCSI512) hdType[cartNo] = HD_MEGASCSI;
        if (currentRomType[cartNo] == SRAM_MEGASCSI1MB) hdType[cartNo] = HD_MEGASCSI;
        if (currentRomType[cartNo] == SRAM_WAVESCSI)    hdType[cartNo] = HD_WAVESCSI;
        if (currentRomType[cartNo] == SRAM_WAVESCSI128) hdType[cartNo] = HD_WAVESCSI;
        if (currentRomType[cartNo] == SRAM_WAVESCSI256) hdType[cartNo] = HD_WAVESCSI;
        if (currentRomType[cartNo] == SRAM_WAVESCSI512) hdType[cartNo] = HD_WAVESCSI;
        if (currentRomType[cartNo] == SRAM_WAVESCSI1MB) hdType[cartNo] = HD_WAVESCSI;
        if (currentRomType[cartNo] == ROM_GOUDASCSI)    hdType[cartNo] = HD_GOUDASCSI;
    }

    if(hdType[cartNo] != HD_NONE)
    	logMsg("HD Type:%d", (int)hdType[cartNo]);

    bool success;
    if(cartNo < boardInfo.cartridgeCount)
    {
    	success = cartridgeInsert(cartNo, romType, cart, cartZip);
    }
    else
    {
    	logErr("cart #%d exceeds max:%d", cartNo, boardInfo.cartridgeCount);
    	success = 0;
    }

    return success;
}

void boardInit(UInt32* systemTime)
{
    boardSysTime = systemTime;
    //oldTime = *systemTime;
    //boardSysTime64 = oldTime * HIRES_CYCLES_PER_LORES_CYCLE;

    timeAnchor = *systemTime;

    static BoardTimer dummy_timer;
    if (timerList == NULL) {
        dummy_timer.next     = &dummy_timer;
        dummy_timer.prev     = &dummy_timer;
        dummy_timer.callback = NULL;
        dummy_timer.ref      = &dummy_timer;
        dummy_timer.timeout  = 0;
        timerList = &dummy_timer;
    }

    if(!fdcTimer)
    	fdcTimer = boardTimerCreate(onFdcDone, NULL);

    if(!mixerTimer)
      mixerTimer = boardTimerCreate(onMixerSync, mixer);

    boardTimerAdd(mixerTimer, boardSystemTime() + boardFrequency() / mixerSyncHz);
}
