/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <neopop.h>
#include <flash.h>
#include "TLCS900h_interpret.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "interrupt.h"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2018\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2004\nthe NeoPop Team\nwww.nih.at";
uint32 frameskip_active = 0;
static const int ngpResX = SCREEN_WIDTH, ngpResY = SCREEN_HEIGHT;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
static EmuSystemTask *emuSysTask{};
static EmuVideo *emuVideo{};
static IG::Pixmap srcPix{{{ngpResX, ngpResY}, pixFmt}, cfb};

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "ngc") ||
				string_hasDotExtension(name, "ngp") ||
				string_hasDotExtension(name, "npc");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;

const char *EmuSystem::shortSystemName()
{
	return "NGP";
}

const char *EmuSystem::systemName()
{
	return "Neo Geo Pocket";
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	::reset();
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.ngs", statePath, gameName, saveSlotCharUpper(slot));
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	if(!state_store(path))
		return makeFileWriteError();
	else
		return {};
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	if(!state_restore(path))
		return makeFileReadError();
	else
		return {};
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
	auto ec = writeToNewFile(saveStr.data(), buffer, len);
	return !ec;
}

void EmuSystem::saveBackupMem()
{
	logMsg("saving flash");
	flash_commit();
}

void EmuSystem::closeSystem()
{
	rom_unload();
	logMsg("closing game %s", gameName().data());
}

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

EmuSystem::Error EmuSystem::loadGame(IO &io, OnLoadProgressDelegate)
{
	if(!romLoad(io))
	{
		return EmuSystem::makeError("Error loading game");
	}
	rom_loaded();
	logMsg("loaded NGP rom: %s, catalog %d,%d", rom.name, rom_header->catalog, rom_header->subCatalog);
	::reset();
	rom_bootHacks();
	return {};
}

void EmuSystem::onPrepareVideo(EmuVideo &video)
{
	video.setFormat({{ngpResX, ngpResY}, pixFmt});
}

void EmuSystem::configAudioRate(double frameTime, int rate)
{
	double mixRate = std::round(rate * (60. * frameTime));
	sound_init(mixRate);
}

void system_sound_chipreset(void)
{
	EmuSystem::configAudioPlayback();
}

void system_VBL(void)
{
	if(likely(emuVideo))
	{
		emuVideo->startFrame(emuSysTask, srcPix);
		emuVideo = {};
		emuSysTask = {};
	}
}

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, bool renderAudio)
{
	emuSysTask = task;
	emuVideo = video;
	frameskip_active = video ? 0 : 1;

	#ifndef NEOPOP_DEBUG
	emulate();
	#else
	emulate_debug(0, 1);
	#endif
	// video rendered in emulate()

	if(renderAudio)
	{
		auto audioFrames = audioFramesForThisFrame();
		uint16 destBuff[audioFrames];
		sound_update(destBuff, audioFrames*2);
		writeSound(destBuff, audioFrames);
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

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
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

EmuSystem::Error EmuSystem::onInit()
{
	EmuSystem::pcmFormat.channels = 1;
	gfx_buildMonoConvMap();
	gfx_buildColorConvMap();
	system_colour = COLOURMODE_AUTO;
	bios_install();
	return {};
}
