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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include <mednafen/state-driver.h>
#include <mednafen/hash/md5.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/ngp/neopop.h>
#include <mednafen/ngp/flash.h>
#include <mednafen-emuex/MDFNUtils.hh>

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nNeoPop Team\nwww.nih.at";
static const unsigned vidBufferX = 160, vidBufferY = 152;
alignas(8) static uint32_t pixBuff[vidBufferX*vidBufferY]{};
static IG::Pixmap mSurfacePix;
uint8_t inputBuff{};
IG::ApplicationContext appCtx{};

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::stringEndsWithAny(name, ".ngc", ".ngp", ".npc", ".NGC", ".NGP", ".NPC");
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
	MDFN_IEN_NGP::reset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'q';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot); return 0;
	}
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.{}.nc{}", name, md5_context::asciistr(MDFNGameInfo->MD5, 0), saveSlotChar(slot));
}

void EmuSystem::saveState(IG::CStringView path)
{
	if(!MDFNI_SaveState(path, 0, 0, 0, 0))
		throwFileWriteError();
}

void EmuSystem::loadState(IG::CStringView path)
{
	if(!MDFNI_LoadState(path, 0))
		throwFileReadError();
}

static FS::PathString saveFilename(IG::ApplicationContext ctx)
{
	return EmuSystem::contentSaveFilePath(ctx, ".ngf");
}

void EmuSystem::saveBackupMem(IG::ApplicationContext)
{
	logMsg("saving flash");
	MDFN_IEN_NGP::FLASH_SaveNV();
}

void EmuSystem::closeSystem(IG::ApplicationContext)
{
	emuSys->CloseGame();
}

void EmuSystem::loadGame(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	emuSys->name = std::string{EmuSystem::contentName()};
	static constexpr size_t maxRomSize = 0x400000;
	auto stream = std::make_unique<MemoryStream>(maxRomSize, true);
	auto size = io.read(stream->map(), stream->map_size());
	if(size <= 0)
		throwFileReadError();
	stream->setSize(size);
	MDFNFILE fp(&NVFS, std::move(stream));
	GameFile gf{&NVFS, std::string{contentDirectory()}, fp.stream(),
		stringWithoutDotExtension<std::string>(contentFileName()),
		std::string{contentName()}};
	emuSys->Load(&gf);
	emuSys->SetInput(0, "gamepad", (uint8*)&inputBuff);
	EmulateSpecStruct espec{};
	auto mSurface = pixmapToMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	MDFN_IEN_NGP::applyVideoFormat(&espec);
}

bool EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	mSurfacePix = {{{vidBufferX, vidBufferY}, fmt}, pixBuff};
	if(!gameIsRunning())
		return false;
	EmulateSpecStruct espec{};
	auto mSurface = pixmapToMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	MDFN_IEN_NGP::applyVideoFormat(&espec);
	return false;
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	EmulateSpecStruct espec{};
	static constexpr double ngpFrameRate = 59.95;
	espec.SoundRate = std::round(rate * (ngpFrameRate * frameTime.count()));
	logMsg("emu sound rate:%f", (double)espec.SoundRate);
	MDFN_IEN_NGP::applySoundFormat(&espec);
}

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	unsigned maxFrames = 48000/54;
	int16 audioBuff[maxFrames*2];
	EmulateSpecStruct espec{};
	if(audio)
	{
		espec.SoundBuf = audioBuff;
		espec.SoundBufMaxSize = maxFrames;
	}
	espec.taskCtx = taskCtx;
	espec.video = video;
	espec.skip = !video;
	auto mSurface = pixmapToMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	emuSys->Emulate(&espec);
	if(audio)
	{
		assert((unsigned)espec.SoundBufSize <= audio->format().bytesToFrames(sizeof(audioBuff)));
		audio->writeFrames((uint8_t*)audioBuff, espec.SoundBufSize);
	}
}

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

void EmuSystem::onInit(IG::ApplicationContext ctx)
{
	appCtx = ctx;
}

}

namespace MDFN_IEN_NGP
{

bool system_io_flash_read(uint8_t* buffer, uint32_t len)
{
	using namespace EmuEx;
	auto saveStr = saveFilename(appCtx);
	return IG::FileUtils::readFromUri(appCtx, saveStr, {buffer, len}) > 0;
}

void system_io_flash_write(uint8_t* buffer, uint32 len)
{
	using namespace EmuEx;
	if(!len)
		return;
	auto saveStr = saveFilename(appCtx);
	logMsg("writing flash %s", saveStr.data());
	IG::FileUtils::writeToUri(appCtx, saveStr, {buffer, len}) != -1;
}

}

namespace Mednafen
{

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	espec->video->startFrameWithFormat(espec->taskCtx, EmuEx::mSurfacePix);
}

}
