#define LOGTAG "main"
#include <neopop.h>
#include <flash.h>
#include "TLCS900h_interpret.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "interrupt.h"
#include <emuframework/EmuSystem.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <emuframework/CommonGui.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2004\nthe NeoPop Team\nwww.nih.at";
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

const char *EmuSystem::inputFaceBtnName = "A/B";
const char *EmuSystem::inputCenterBtnName = "Option";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 1;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = true;
const char *EmuSystem::configFilename = "NgpEmu.config";
const uint EmuSystem::maxPlayers = 1;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"20:19 (Original)", 20, 19},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);

const char *EmuSystem::shortSystemName()
{
	return "NGP";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo Pocket";
}

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded() {}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_NGPKEY_LANGUAGE: optionNGPLanguage.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionNGPLanguage.writeWithKeyIfNotDefault(io);
}

static bool hasNGPExtension(const char *name)
{
	return string_hasDotExtension(name, "ngc") ||
			string_hasDotExtension(name, "ngp") ||
			string_hasDotExtension(name, "npc");
}

EmuNameFilterFunc EmuFilePicker::defaultFsFilter = hasNGPExtension;
EmuNameFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = hasNGPExtension;

static const int ngpResX = SCREEN_WIDTH, ngpResY = SCREEN_HEIGHT;

static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;

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
	ctrlBits = IG::setOrClearBits(ctrlBits, (uchar)emuKey, state == Input::PUSHED);
}

static bool renderToScreen = 0;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	::reset();
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.ngs", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(!state_store(saveStr.data()))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr))
	{
		logMsg("loading state %s", saveStr.data());
		if(!state_restore(saveStr.data()))
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
	return readFromFile(filename, buffer, bufferLength) > 0;
}

static FS::PathString sprintSaveFilename()
{
	return FS::makePathStringPrintf("%s/%s.ngf", EmuSystem::savePath(), EmuSystem::gameName().data());
}

bool system_io_flash_read(uchar* buffer, uint32 len)
{
	auto saveStr = sprintSaveFilename();
	return readFromFile(saveStr.data(), buffer, len) > 0;
}

bool system_io_flash_write(uchar* buffer, uint32 len)
{
	if(!len)
		return 0;
	auto saveStr = sprintSaveFilename();
	logMsg("writing flash %s", saveStr.data());
	CallResult ret;
	if((ret = writeToNewFile(saveStr.data(), buffer, len)) == OK)
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
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		state_store(saveStr.data());
	}
}

void EmuSystem::closeSystem()
{
	rom_unload();
	logMsg("closing game %s", gameName().data());
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

static bool romLoad(IO &io)
{
	const uint maxRomSize = 0x400000;
	auto data = (uchar*)calloc(maxRomSize, 1);
	uint readSize = io.read(data, maxRomSize);
	if(readSize > 0)
	{
		logMsg("read 0x%X byte rom", readSize);
		rom.data = data;
		rom.length = readSize;
		return true;
	}
	free(data);
	return false;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame(true);
	emuVideo.initImage(false, ngpResX, ngpResY);
	setupGamePaths(path);
	if(!romLoad(io))
	{
		popup.postError("Error loading game");
		return 0;
	}
	rom_loaded();
	logMsg("loaded NGP rom: %s, catalog %d,%d", rom.name, rom_header->catalog, rom_header->subCatalog);
	::reset();
	rom_bootHacks();
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	ram[0x6F82] = 0;
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	double rate = std::round(optionSoundRate * (60. * frameTime));
	sound_init(rate);
}

void system_sound_chipreset(void)
{
	EmuSystem::configAudioPlayback();
}

void system_VBL(void)
{
	if(likely(renderToScreen))
	{
		updateAndDrawEmuVideo();
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
	{
		uint16 destBuff[audioFramesPerVideoFrame];
		sound_update(destBuff, audioFramesPerVideoFrame*2);
		writeSound(destBuff, audioFramesPerVideoFrame);
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
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start(args, format);
	logger_vprintf(LOG_M, format, args);
	va_end( args );
	logger_printf(LOG_M, "\n");
}
#endif

#ifdef NEOPOP_DEBUG
void system_debug_message(const char* format, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start(args, format);
	logger_vprintfn(LOG_M, format, args);
	va_end( args );
	logger_printfn(LOG_M, "\n");
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

bool EmuSystem::hasInputOptions() { return false; }

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((34./255.) * .4, (15./255.) * .4, (64./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

CallResult EmuSystem::onInit()
{
	EmuSystem::pcmFormat.channels = 1;
	emuVideo.initPixmap((char*)cfb, pixFmt, ngpResX, ngpResY);
	gfx_buildMonoConvMap();
	gfx_buildColorConvMap();
	system_colour = COLOURMODE_AUTO;
	bios_install();
	return OK;
}
