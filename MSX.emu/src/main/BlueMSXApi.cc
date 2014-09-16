#define LOGTAG "blueMSXApi"

#include <imagine/logger/logger.h>
#include <imagine/fs/sys.hh>
#include <imagine/util/time/sys.hh>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
	#include <blueMSX/Arch/ArchTimer.h>
	#include <blueMSX/Arch/ArchMidi.h>
	#include <blueMSX/Arch/ArchInput.h>
	#include <blueMSX/Arch/ArchFile.h>
	#include <blueMSX/Arch/ArchGlob.h>
	#include <blueMSX/Arch/ArchNotifications.h>
	#include <blueMSX/Arch/ArchEvent.h>
	#include <blueMSX/Arch/ArchVideoIn.h>
	#include <blueMSX/Arch/ArchMenu.h>
	#include <blueMSX/Arch/ArchSound.h>
	#include <blueMSX/Arch/ArchDialog.h>
	#include <blueMSX/Arch/ArchThread.h>
	#include <blueMSX/Arch/ArchPrinter.h>
	#include <blueMSX/Arch/ArchCdrom.h>
	#include <blueMSX/Arch/ArchUart.h>
	#include <blueMSX/Arch/ArchVideoIn.h>
	#include <blueMSX/IoDevice/ScsiDefs.h>
	#include <blueMSX/Debugger/Debugger.h>
	#include <blueMSX/Emulator/FileHistory.h>
	#include <blueMSX/Emulator/AppConfig.h>
	#include <blueMSX/Z80/R800Debug.h>
}

int archCreateDirectory(const char* pathname)
{
    if(FsSys::mkdir(pathname) != OK)
    	return -1;
    return 0;
}

// needed by machineGetAvailable(), should never be called
const char* appConfigGetString(const char* key, const char* defVal)
{
	assert(0);
	return 0;
}

void archMouseGetState(int* dx, int* dy) { }
int archMouseGetButtonState(int checkAlways) { return 0; }
void archMouseEmuEnable(AmEnableMode mode) { }
void archMouseSetForceLock(int lock) { }

UInt32 archGetSystemUpTime(UInt32 frequency)
{
	auto time = TimeSys::now();
	return (uint32)(time.divByUSecs(1000));
}

UInt32 archGetHiresTimer()
{
	auto time = TimeSys::now();
	return (float)time*1000000;
}

Properties* propGetGlobalProperties() { assert(0); return 0; }; // TODO: needed in Casette.c

ArchGlob* archGlob(const char* pattern, int flags) { assert(0); return 0; } //TODO
void archGlobFree(ArchGlob* globHandle) { }

void archVideoOutputChange() { logMsg("called archVideoOutputChange"); }

void* archSemaphoreCreate(int initCount) { return 0; }
void archSemaphoreWait(void* semaphore, int timeout) { assert(0); }
void archSemaphoreSignal(void* semaphore) { assert(0); }
void archSemaphoreDestroy(void* semaphore) { }

int archMidiGetNoteOn() { return 0; }
void archMidiUpdateVolume(int left, int right) {}
ArchMidi* archMidiInCreate(int device, ArchMidiInCb cb, void* ref) { assert(0); return NULL; }
void archMidiInDestroy(ArchMidi* archMidi) {}
int archMidiInGetNoteOn(ArchMidi* archMidi, int note) { return 0; }
ArchMidi* archMidiOutCreate(int device) { assert(0); return NULL; }
void archMidiOutDestroy(ArchMidi* archMidi) {}
void archMidiOutTransmit(ArchMidi* archMidi, UInt8 value) {}
void archMidiLoadState(void) {}
void archMidiSaveState(void) {}

void archCdromDestroy(ArchCdrom* cdrom) {}
void archCdromHwReset(ArchCdrom* cdrom) {}
void archCdromBusReset(ArchCdrom* cdrom) {}
void archCdromDisconnect(ArchCdrom* cdrom) {}
void archCdromLoadState(ArchCdrom* cdrom) {}
void archCdromSaveState(ArchCdrom* cdrom) {}
UInt8 archCdromGetStatusCode(ArchCdrom* cdrom) { return 0; }
int archCdromExecCmd(ArchCdrom* cdrom, const UInt8* cdb, UInt8* buffer, int bufferSize) { return 0; }
int archCdromIsXferComplete(ArchCdrom* cdrom, int* transferLength) { return 0; }

ArchCdrom* archCdromCreate(CdromXferCompCb xferCompCb, void* ref) { return NULL; }

void archPrinterWrite(UInt8 value) { }
int archPrinterCreate(void) { return 1; }
void archPrinterDestroy(void) { }
void archForceFormFeed(void) { }

void archUartTransmit(UInt8 value) { }
int archUartCreate(void (*archUartReceiveCallback) (UInt8)) { return 1; }
void archUartDestroy(void) { }

int archVideoInIsVideoConnected() { return 0; }
UInt16* archVideoInBufferGet(int width, int height) { return NULL; }

void updateExtendedDiskName(int drive, char* filename, char* zipFile) { }

/*int debuggerCheckVramAccess(void)
{
	return 0;
}*/

const char* stripPath(const char* filename)
{
	const char* ptr = filename + strlen(filename) - 1;

	while (--ptr >= filename)
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			return ptr + 1;
		}
	}

	return filename;
}

// misc framebuffer funcs

FrameBufferData* frameBufferDataCreate(int maxWidth, int maxHeight, int defaultHorizZoom)
{
	logMsg("frameBufferDataCreate %dx%d zoom %d", maxWidth, maxHeight, defaultHorizZoom);
	return (FrameBufferData*)1;
}

void frameBufferDataDestroy(FrameBufferData* frameData) {}
FrameBuffer* frameBufferGetDrawFrame() {return (FrameBuffer*)1; }
FrameBuffer* frameBufferFlipDrawFrame() { return (FrameBuffer*)1; }
FrameBufferData* frameBufferGetActive() { return (FrameBufferData*)1; }
void frameBufferSetActive(FrameBufferData* frameData) {}
void frameBufferSetMixMode(FrameBufferMixMode mode, FrameBufferMixMode mask) {}
void frameBufferClearDeinterlace() {}
void frameBufferSetInterlace(FrameBuffer* frameBuffer, int val)
{
	//logMsg("setting interlace %d", val);
}
void frameBufferSetLineCount(FrameBuffer* frameBuffer, int val)
{
	//logMsg("setting line count %d", val);
}

// Used by gunstick and asciilaser
static int fbScanLine = 0;
void frameBufferSetScanline(int scanline) { fbScanLine = scanline; }
int frameBufferGetScanline() { return fbScanLine; }

// dummy R800 debug funcs
void r800DebugCreate(R800* r800) { }
void r800DebugDestroy() { }
