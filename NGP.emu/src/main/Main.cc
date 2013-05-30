#define thisModuleName "main"
#include <neopop.h>
#include <flash.h>

#include <logger/interface.h>
#include <util/area2.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <unzip.h>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2004\nthe NeoPop Team\nwww.nih.at";
uint32 frameskip_active = 0;

// controls

enum
{
	ngpKeyIdxUp = EmuControls::systemKeyMapStart,
	ngpKeyIdxRight,
	ngpKeyIdxDown,
	ngpKeyIdxLeft,
	ngpKeyIdxLeftUp,
	ngpKeyIdxRightUp,
	ngpKeyIdxRightDown,
	ngpKeyIdxLeftDown,
	ngpKeyIdxOption,
	ngpKeyIdxA,
	ngpKeyIdxB,
	ngpKeyIdxATurbo,
	ngpKeyIdxBTurbo
};

enum {
	CFGKEY_NGPKEY_LANGUAGE = 269,
};

static Option<OptionMethodRef<template_ntype(language_english)>, uint8> optionNGPLanguage(CFGKEY_NGPKEY_LANGUAGE, 1);

const uint EmuSystem::maxPlayers = 1;
uint EmuSystem::aspectRatioX = 20, EmuSystem::aspectRatioY = 19;
#include "CommonGui.hh"

void EmuSystem::initOptions()
{

}

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_NGPKEY_LANGUAGE: optionNGPLanguage.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionNGPLanguage.writeWithKeyIfNotDefault(io);
}

static bool isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "ngc") ||
			string_hasDotExtension(name, "ngp") ||
			string_hasDotExtension(name, "npc");
}

static bool isNGPExtension(const char *name)
{
	return isROMExtension(name) || string_hasDotExtension(name, "zip");
}

static int ngpFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isNGPExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = ngpFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = ngpFsFilter;

static const int ngpResX = SCREEN_WIDTH, ngpResY = SCREEN_HEIGHT;

static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;//&PixelFormatBGRA4444;//

static const uint ctrlUpBit = 0x01, ctrlDownBit = 0x02, ctrlLeftBit = 0x04, ctrlRightBit = 0x08,
		ctrlABit = 0x10, ctrlBBit = 0x20, ctrlOptionBit = 0x40;

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	map[SysVController::F_ELEM] = ctrlABit;
	map[SysVController::F_ELEM+1] = ctrlBBit;

	map[SysVController::C_ELEM] = ctrlOptionBit;

	map[SysVController::D_ELEM] = ctrlUpBit | ctrlLeftBit;
	map[SysVController::D_ELEM+1] = ctrlUpBit;
	map[SysVController::D_ELEM+2] = ctrlUpBit | ctrlRightBit;
	map[SysVController::D_ELEM+3] = ctrlLeftBit;
	map[SysVController::D_ELEM+5] = ctrlRightBit;
	map[SysVController::D_ELEM+6] = ctrlDownBit | ctrlLeftBit;
	map[SysVController::D_ELEM+7] = ctrlDownBit;
	map[SysVController::D_ELEM+8] = ctrlDownBit | ctrlRightBit;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case ngpKeyIdxUp: return ctrlUpBit;
		case ngpKeyIdxRight: return ctrlRightBit;
		case ngpKeyIdxDown: return ctrlDownBit;
		case ngpKeyIdxLeft: return ctrlLeftBit;
		case ngpKeyIdxLeftUp: return ctrlLeftBit | ctrlUpBit;
		case ngpKeyIdxRightUp: return ctrlRightBit | ctrlUpBit;
		case ngpKeyIdxRightDown: return ctrlRightBit | ctrlDownBit;
		case ngpKeyIdxLeftDown: return ctrlLeftBit | ctrlDownBit;
		case ngpKeyIdxOption: return ctrlOptionBit;
		case ngpKeyIdxATurbo: turbo = 1;
		case ngpKeyIdxA: return ctrlABit;
		case ngpKeyIdxBTurbo: turbo = 1;
		case ngpKeyIdxB: return ctrlBBit;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uchar &ctrlBits = ram[0x6F82];
	if(state == Input::PUSHED)
		setBits(ctrlBits, emuKey);
	else
		unsetBits(ctrlBits, emuKey);
}

static bool renderToScreen = 0;

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	reset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.ngs", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(Config::envIsIOSJB)
		fixFilePermissions(saveStr);
	if(!state_store(saveStr))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(FsSys::fileExists(saveStr))
	{
		logMsg("loading state %s", saveStr);
		if(!state_restore(saveStr))
			return STATE_RESULT_IO_ERROR;
		else
		{
			return STATE_RESULT_OK;
		}
	}
	return STATE_RESULT_NO_FILE;
}

bool system_io_state_read(const char* filename, uchar* buffer, uint32 bufferLength)
{
	return IoSys::readFromFile(filename, buffer, bufferLength) ? 1 : 0;
}

template <size_t S>
static void sprintSaveFilename(char (&str)[S])
{
	snprintf(str, S, "%s/%s.ngf", EmuSystem::savePath(), EmuSystem::gameName);
}

bool system_io_flash_read(uchar* buffer, uint32 len)
{
	FsSys::cPath saveStr;
	sprintSaveFilename(saveStr);
	return IoSys::readFromFile(saveStr, buffer, len) ? 1 : 0;
}

bool system_io_flash_write(uchar* buffer, uint32 len)
{
	if(!len)
		return 0;
	FsSys::cPath saveStr;
	sprintSaveFilename(saveStr);
	logMsg("writing flash %s", saveStr);
	CallResult ret;
	if((ret = IoSys::writeToNewFile(saveStr, buffer, len)) == OK)
		return 1;
	else
		return 0;
}

void EmuSystem::saveBackupMem()
{
	logMsg("saving flash");
	flash_commit();
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		logMsg("saving auto-state");
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		if(Config::envIsIOSJB)
			fixFilePermissions(saveStr);
		state_store(saveStr);
	}
}

void EmuSystem::closeSystem()
{
	rom_unload();
	logMsg("closing game %s", gameName);
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

#define HAVE_LIBZ
static bool romLoad(const char *filename)
{
#ifdef HAVE_LIBZ
	unzFile z;
	if ((z=unzOpen(filename)) != 0)
	{
		for (int err=unzGoToFirstFile(z); err==0; err=unzGoToNextFile(z))
		{
			char name[1024];
			unz_file_info zfi;
			if (unzGetCurrentFileInfo(z, &zfi, name, sizeof(name), NULL, 0, NULL, 0) != UNZ_OK)
				continue;
			if (zfi.size_filename > sizeof(name))
				continue;
			int l = strlen(name);
			if (l < 4)
				continue;
			if (strcasecmp(name+l-4, ".ngp") == 0 || strcasecmp(name+l-4, ".ngc") == 0
					|| strcasecmp(name+l-4, ".npc") == 0)
			{
				rom.length = zfi.uncompressed_size;
				rom.data = (uchar*)calloc(rom.length, 1);

				if ((unzOpenCurrentFile(z) != UNZ_OK)
				|| (unzReadCurrentFile(z, rom.data, rom.length)
				!= (int)rom.length))
				{
					free(rom.data);
					rom.data = NULL;
					logMsg("error in unzOpenCurrentFile: %s", filename);
					unzCloseCurrentFile(z);
					unzClose(z);
					return 0;
				}
				if (unzCloseCurrentFile(z) != UNZ_OK)
				{
					free(rom.data);
					rom.data = NULL;
					logMsg("error in unzCloseCurrentFile: %s", filename);
					unzClose(z);
					return 0;
				}
				unzClose(z);
				logMsg("read 0x%X byte rom", rom.length);
				return 1;
			}
		}
		unzClose(z);
		logMsg("`%s': no rom found", filename);
		return 0;
	}
#endif

	const uint maxRomSize = 0x400000;
	rom.data = (uchar*)calloc(maxRomSize, 1);

	uint readSize = IoSys::readFromFile(filename, rom.data, maxRomSize);
	if(readSize)
	{
    	logMsg("read 0x%X byte rom", readSize);
    	rom.length = readSize;
    	return 1;
	}

	logMsg("%s `%s'", "error reading rom", filename);
	free(rom.data);
	rom.data = NULL;
	return 0;
}

#include "TLCS900h_interpret.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "interrupt.h"

int EmuSystem::loadGame(const char *path)
{
	closeGame(1);
	emuView.initImage(0, ngpResX, ngpResY);
	setupGamePaths(path);

	if(!romLoad(fullGamePath))
	{
		logMsg("failed to load game");
		popup.postError("Error loading game");
		return 0;
	}
	rom_loaded();
	logMsg("name from NGP rom: %s, catalog %d,%d", rom.name, rom_header->catalog, rom_header->subCatalog);
	reset();

	rom_bootHacks();

	logMsg("started emu");
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	ram[0x6F82] = 0;
}

static uint audioFramesPerUpdate;

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	//logMsg("set audio rate %d", Audio::pPCM.rate);
	float rate = optionSoundRate;
	#ifdef CONFIG_ENV_WEBOS
	if(optionFrameSkip != optionFrameSkipAuto)
		rate *= 42660./44100.; // better sync with Pre's refresh rate
	#endif
	sound_init(rate);
	audioFramesPerUpdate = rate/60.;
}

void system_sound_chipreset(void)
{
	EmuSystem::configAudioPlayback();
}

static void writeAudio()
{
#ifdef USE_NEW_AUDIO
	Audio::BufferContext *aBuff = Audio::getPlayBuffer(Audio::maxRate/60);
	if(!aBuff) return;
	assert(aBuff->frames >= Audio::maxRate/60);
	sound_update((uint16*)aBuff->data, audioFramesPerUpdate*2);
	Audio::commitPlayBuffer(aBuff, audioFramesPerUpdate);
#else
	uint16 destBuff[(Audio::maxRate/60)];
	uint destFrames = audioFramesPerUpdate;
	sound_update(destBuff, audioFramesPerUpdate*2);
	Audio::writePcm((uchar*)destBuff, destFrames);
#endif
}

void system_VBL(void)
{
	if(likely(renderToScreen))
	{
		emuView.updateAndDrawContent();
		renderToScreen = 0;
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(renderGfx)
		renderToScreen = 1;
	frameskip_active = processGfx ? 0 : 1;

	#ifndef NEOPOP_DEBUG
	emulate();
	#else
	emulate_debug(0, 1);
	#endif
	// video rendered in emulate()

	if(renderAudio)
		writeAudio();
}

namespace Input
{
void onInputEvent(const Input::Event &e)
{
	handleInputEvent(e);
}
}

bool system_comms_read(uchar* buffer)
{
	return 0;
}

bool system_comms_poll(uchar* buffer)
{
	return 0;
}

void system_comms_write(uchar data)
{

}

char *system_get_string(STRINGS string_id)
{
	static char s[128];
	snprintf(s, sizeof(s), "code %d", string_id);
	return s;
}

#ifndef NDEBUG
void system_message(const char* format, ...)
{
	#ifdef USE_LOGGER
	va_list args;
	va_start(args, format);
	logger_vprintf(LOG_M, format, args);
	va_end( args );
	logger_printf(LOG_M, "\n");
	#endif
}
#endif

#ifdef NEOPOP_DEBUG
void system_debug_message(const char* format, ...)
{
	#ifdef USE_LOGGER
	va_list args;
	va_start(args, format);
	logger_vprintfn(LOG_M, format, args);
	va_end( args );
	logger_printfn(LOG_M, "\n");
	#endif
}

void system_debug_message_associate_address(uint32 address)
{
	//logMsg("with associated address 0x%X", address);
}

void system_debug_stop(void) { }

void system_debug_refresh(void) { }

void system_debug_history_add(void) { }

void system_debug_history_clear(void) { }

void system_debug_clear(void) { }
#endif

void gfx_buildColorConvMap();
void gfx_buildMonoConvMap();

void EmuSystem::savePathChanged() { }

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit(int argc, char** argv)
{
	mainInitCommon();
	EmuSystem::pcmFormat.channels = 1;
	emuView.initPixmap((uchar*)cfb, pixFmt, ngpResX, ngpResY);
	gfx_buildMonoConvMap();
	gfx_buildColorConvMap();
	system_colour = COLOURMODE_AUTO;
	bios_install();
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((34./255.) * .4, (15./255.) * .4, (64./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
