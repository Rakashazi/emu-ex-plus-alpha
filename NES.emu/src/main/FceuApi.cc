#define LOGTAG "main"

#include <imagine/logger/logger.h>
#include <imagine/fs/sys.hh>
#include <imagine/io/api/stdio.hh>
#include <stdio.h>
#include <fceu/driver.h>
#include <fceu/video.h>

bool turbo = 0;
int closeFinishedMovie = 0;

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	logMsg("opening file %s mode %s", fn, mode);
	auto file = fopen(fn,mode);
//	if(!file)
//		logErr("error opening %s", fn);
	return file;
}

void FCEUD_PrintError(const char *errormsg) { logErr("%s", errormsg); }

#ifndef NDEBUG
void FCEUD_Message(const char *s) { logger_printf(0, "%s", s); }

void FCEU_DispMessageOnMovie(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	logger_vprintf(0, format, ap);
	va_end(ap);
	logger_printf(0, "\n");
}

void FCEU_DispMessage(const char *format, int disppos=0, ...)
{
	va_list ap;
	va_start(ap, disppos);
	logger_vprintf(0, format, ap);
	va_end(ap);
	logger_printf(0, "\n");
}
#endif

const char *FCEUD_GetCompilerString() { return ""; }

void FCEUD_DebugBreakpoint() { return; }

int FCEUD_ShowStatusIcon(void) { return 0; }

void FCEUD_NetworkClose(void) { }

void FCEUD_VideoChanged() { }

bool FCEUI_AviIsRecording(void) { return 0; }

bool FCEUI_AviDisableMovieMessages() { return 1; }

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *fn, const char *m)
{
	EMUFILE_FILE *f = new EMUFILE_FILE(fn, m);
	if(!f->is_open())
	{
		delete f;
		return 0;
	}
	else
		return f;
}

bool FCEUD_PauseAfterPlayback() { return false; }

bool FCEUD_ShouldDrawInputAids() { return 0; }

void FCEUI_UseInputPreset(int preset) { }

int FCEUD_SendData(void *data, uint32 len) { return 1; }

int FCEUD_RecvData(void *data, uint32 len) { return 1; }

void FCEUD_NetplayText(uint8 *text) { }

void FCEUD_SetEmulationSpeed(int cmd) { }

void FCEUD_SoundVolumeAdjust(int n) { }

void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }

void FCEUD_HideMenuToggle(void) { }

void FCEUD_AviRecordTo() { }
void FCEUD_AviStop() { }

void FCEUD_MovieReplayFrom() { }

void FCEUD_TurboOn(void) {  }
void FCEUD_TurboOff(void) {  }
void FCEUD_TurboToggle(void) {  }

void FCEUD_SoundToggle(void) { }

void FCEUD_ToggleStatusIcon() { }

void FCEUD_MovieRecordTo() { }

void FCEUD_SaveStateAs() { }

void FCEUD_LoadStateFrom() { }

void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp)
{
	logMsg("called set input");
}

// for boards/transformer.cpp
unsigned int *GetKeyboard(void)
{
	static unsigned int k[256] {0};
	return k;
}

// from video.cpp
void FCEUI_ToggleShowFPS() { }
int FCEU_InitVirtualVideo(void) { return 1; }
void FCEU_KillVirtualVideo(void) { }
void FCEU_ResetMessages() { }
void FCEU_PutImageDummy(void) { }
void FCEU_PutImage(void) { }
void FCEUI_SaveSnapshot(void) { }
void ResetScreenshotsCounter() { }
int ClipSidesOffset = 0;
GUIMESSAGE subtitleMessage;

// from drawing.cpp
void DrawTextLineBG(uint8 *dest) { }
void DrawMessage(bool beforeMovie) { }
void FCEU_DrawRecordingStatus(uint8* XBuf) { }
void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur) { }
void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor) { }
void DrawTextTransWH(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor, int max_w, int max_h, int border) { }

// from nsf.cpp
#include <fceu/nsf.h>
NSF_HEADER NSFHeader;
void DoNSFFrame(void) { }
int NSFLoad(const char *name, FCEUFILE *fp) { return 0; }

// from debug.cpp
volatile int datacount, undefinedcount;
unsigned char *cdloggerdata;
int GetPRGAddress(int A) { return 0; }
int debug_loggingCD = 0;

// from netplay.cpp
int FCEUnetplay=0;
int FCEUNET_SendCommand(uint8, uint32) { return 0; }
void NetplayUpdate(uint8 *joyp) { }

// from movie.cpp
void FCEUI_MakeBackupMovie(bool dispMessage) { }

// from fceu.cpp
bool CheckFileExists(const char* filename)
{
	return FsSys::fileExists(filename);
}

//The code in this function is a modified version
//of Chris Covell's work - I'd just like to point that out
void EncodeGG(char *str, int a, int v, int c)
{
	uint8 num[8];
	static char lets[16]={'A','P','Z','L','G','I','T','Y','E','O','X','U','K','S','V','N'};
	int i;
	if(a > 0x8000)a-=0x8000;

	num[0]=(v&7)+((v>>4)&8);
	num[1]=((v>>4)&7)+((a>>4)&8);
	num[2]=((a>>4)&7);
	num[3]=(a>>12)+(a&8);
	num[4]=(a&7)+((a>>8)&8);
	num[5]=((a>>8)&7);

	if (c == -1){
		num[5]+=v&8;
		for(i = 0;i < 6;i++)str[i] = lets[num[i]];
		str[6] = 0;
	} else {
		num[2]+=8;
		num[5]+=c&8;
		num[6]=(c&7)+((c>>4)&8);
		num[7]=((c>>4)&7)+(v&8);
		for(i = 0;i < 8;i++)str[i] = lets[num[i]];
		str[8] = 0;
	}
	return;
}

void EMUFILE_IO::truncate(s32 length)
{
	io.truncate(length);
}

int EMUFILE_IO::fgetc()
{
	return ::fgetc(io);
}

size_t EMUFILE_IO::_fread(const void *ptr, size_t bytes)
{
	ssize_t ret = io.read((void*)ptr, bytes);
	if(ret < (ssize_t)bytes)
		failbit = true;
	return ret;
}

int EMUFILE_IO::fseek(int offset, int origin)
{
	return ::fseek(io, offset, origin);
}

int EMUFILE_IO::ftell()
{
	return (int)::ftell(io);
}

int EMUFILE_IO::size()
{
	return io.size();
}
