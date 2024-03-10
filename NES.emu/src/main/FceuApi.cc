/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"

#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/IO.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
#include "MainSystem.hh"
#include <fceu/driver.h>
#include <fceu/video.h>
#include <fceu/fceu.h>
#include <fceu/input.h>
#include <fceu/ppu.h>
#include <fceu/cheat.h>
#include <fceu/cart.h>
#include <fceu/nsf.h>
#include <fceu/x6502.h>

bool turbo = 0;
int closeFinishedMovie = 0;
int StackAddrBackup = -1;
int KillFCEUXonFrame = 0;
int eoptions = 0;

namespace EmuEx
{
constexpr SystemLogger log{"NES.emu"};
}

void FCEUI_Emulate(EmuEx::EmuSystemTaskContext taskCtx, EmuEx::NesSystem &sys, EmuEx::EmuVideo *video, int skip, EmuEx::EmuAudio *audio)
{
	#ifdef _S9XLUA_H
	FCEU_LuaFrameBoundary();
	#endif

	FCEU_UpdateInput();
	lagFlag = 1;

	#ifdef _S9XLUA_H
	CallRegisteredLuaFunctions(LUACALL_BEFOREEMULATION);
	#endif

	if (geniestage != 1) FCEU_ApplyPeriodicCheats();
	FCEUPPU_Loop(taskCtx, sys, video, audio, skip);

	emulateSound(audio);

	#ifdef _S9XLUA_H
	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);
	#endif

	timestampbase += timestamp;
	timestamp = 0;
}

void FCEUI_Emulate(EmuEx::NesSystem &sys, EmuEx::EmuVideo *video, int skip, EmuEx::EmuAudio *audio)
{
	FCEUI_Emulate({}, sys, video, skip, audio);
}

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	EmuEx::log.info("opening file:{} mode:{}", fn, mode);
	return IG::FileUtils::fopenUri(EmuEx::gAppContext(), fn, mode);
}

void FCEU_printf(const char *format, ...)
{
	if(!Config::DEBUG_BUILD)
		return;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(0, format, ap);
	va_end(ap);
	logger_printf(0, "\n");
}

void FCEUD_PrintError(const char *errormsg)
{
	if(!Config::DEBUG_BUILD)
		return;
	EmuEx::log.error("{}", errormsg);
}

void FCEUD_Message(const char *s)
{
	if(!Config::DEBUG_BUILD)
		return;
	logger_printf(0, "%s", s);
}

void FCEU_PrintError(const char *format, ...)
{
	if(!Config::DEBUG_BUILD)
		return;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOGGER_ERROR, format, ap);
	va_end(ap);
	logger_printf(LOGGER_ERROR, "\n");
}

void FCEU_DispMessageOnMovie(const char *format, ...)
{
	if(!Config::DEBUG_BUILD)
		return;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(0, format, ap);
	va_end(ap);
	logger_printf(0, "\n");
}

void FCEU_DispMessage(const char *format, int disppos=0, ...)
{
	if(!Config::DEBUG_BUILD)
		return;
	va_list ap;
	va_start(ap, disppos);
	logger_vprintf(0, format, ap);
	va_end(ap);
	logger_printf(0, "\n");
}

const char *FCEUD_GetCompilerString() { return ""; }

void FCEUD_DebugBreakpoint() { return; }

int FCEUD_ShowStatusIcon(void) { return 0; }

void FCEUD_NetworkClose(void) {}

void FCEUD_VideoChanged() {}

bool FCEUI_AviIsRecording(void) { return 0; }

bool FCEUI_AviDisableMovieMessages() { return 1; }

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string& fname, int innerIndex, int* userCancel) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename, int* userCancel) { return 0; }
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

void FCEUI_UseInputPreset(int preset) {}

int FCEUD_SendData(void *data, uint32 len) { return 1; }

int FCEUD_RecvData(void *data, uint32 len) { return 1; }

void FCEUD_NetplayText(uint8 *text) {}

void FCEUD_SetEmulationSpeed(int cmd) {}

void FCEUD_SoundVolumeAdjust(int n) {}

void FCEUI_AviVideoUpdate(const unsigned char* buffer) {}

void FCEUD_HideMenuToggle(void) {}

void FCEUD_AviRecordTo() {}
void FCEUD_AviStop() {}

void FCEUD_MovieReplayFrom() {}

void FCEUD_TurboOn(void) {}
void FCEUD_TurboOff(void) {}
void FCEUD_TurboToggle(void) {}

void FCEUD_SoundToggle(void) {}

void FCEUD_ToggleStatusIcon() {}

void FCEUD_MovieRecordTo() {}

void FCEUD_SaveStateAs() {}

void FCEUD_LoadStateFrom() {}

void FCEUD_FlushTrace() {}

void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp)
{
	EmuEx::log.info("called set input");
}

int FCEUD_FDSReadBIOS(void *buff, uint32 size)
{
	using namespace EmuEx;
	auto &sys = static_cast<NesSystem&>(EmuEx::gSystem());
	auto appCtx = sys.appContext();
	const auto &fdsBiosPath = sys.fdsBiosPath;
	if(fdsBiosPath.empty())
	{
		sys.loaderErrorString = "No FDS BIOS set";
		return -1;
	}
	EmuEx::log.info("loading FDS BIOS:{}", fdsBiosPath);
	if(EmuApp::hasArchiveExtension(appCtx.fileUriDisplayName(fdsBiosPath)))
	{
		for(auto &entry : FS::ArchiveIterator{appCtx.openFileUri(fdsBiosPath)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			if(hasFDSBIOSExtension(entry.name()))
			{
				EmuEx::log.info("archive file entry:%s", entry.name().data());
				if(entry.size() != size)
				{
					sys.loaderErrorString = "Incompatible FDS BIOS";
					return -1;
				}
				return entry.read(buff, size);
			}
		}
		sys.loaderErrorString = "Error opening FDS BIOS";
		return -1;
	}
	else
	{
		auto io = appCtx.openFileUri(fdsBiosPath, {.accessHint = IOAccessHint::All});
		if(io.size() != size)
		{
			sys.loaderErrorString = "Incompatible FDS BIOS";
			return -1;
		}
		return io.read(buff, size);
	}
}

void RefreshThrottleFPS() {}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	bug_unreachable("called FCEUD_GetPalette()");
}

// for boards/transformer.cpp
unsigned int *GetKeyboard(void)
{
	static unsigned int k[256] {0};
	return k;
}

// from video.cpp
void FCEUI_ToggleShowFPS() {}
int FCEU_InitVirtualVideo(void) { return 1; }
void FCEU_KillVirtualVideo(void) {}
void FCEU_ResetMessages() {}
void FCEU_PutImageDummy(void) {}
void FCEU_PutImage(void) {}
void FCEUI_SaveSnapshot(void) {}
void ResetScreenshotsCounter() {}
int ClipSidesOffset = 0;
GUIMESSAGE subtitleMessage;

// from drawing.cpp
void DrawTextLineBG(uint8 *dest) {}
void DrawMessage(bool beforeMovie) {}
void FCEU_DrawRecordingStatus(uint8* XBuf) {}
void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur) {}
void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor) {}
void DrawTextTransWH(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor, int max_w, int max_h, int border) {}

// from debug.cpp
volatile int datacount, undefinedcount;
unsigned char *cdloggerdata;
int GetPRGAddress(int A) { return 0; }
void IncrementInstructionsCounters() {}
int debug_loggingCD = 0;

// from netplay.cpp
int FCEUnetplay=0;
int FCEUNET_SendCommand(uint8, uint32) { return 0; }
void NetplayUpdate(uint8 *joyp) { }

// from fceu.cpp
bool CheckFileExists(const char* filename)
{
	return IG::FS::exists(filename);
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

std::string FCEU_MakeFName(int type, int id1, const char *cd1)
{
	using namespace EmuEx;
	auto &app = gApp();
	auto &sys = static_cast<NesSystem&>(app.system());
	switch(type)
	{
		case FCEUMKF_SAV:
			return std::string{app.contentSaveFilePath(".sav")};
		case FCEUMKF_CHEAT:
			return std::string{sys.userFilePath(sys.cheatsDir, ".cht")};
		case FCEUMKF_GGROM:
			return std::string{sys.contentDirectory("gg.rom")};
		case FCEUMKF_FDS:
			return std::string{app.contentSaveFilePath(".fds.sav")};
		case FCEUMKF_PALETTE:
			return std::string{sys.userFilePath(sys.palettesDir, ".pal")};
		case FCEUMKF_IPS:
		case FCEUMKF_MOVIE:
		case FCEUMKF_STATE:
		case FCEUMKF_RESUMESTATE:
		case FCEUMKF_SNAP:
		case FCEUMKF_AUTOSTATE:
		case FCEUMKF_FDSROM:
		case FCEUMKF_MOVIEGLOB:
		case FCEUMKF_MOVIEGLOB2:
		case FCEUMKF_STATEGLOB:
			EmuEx::log.warn("unused filename type:%d", type);
	}
	return {};
}
