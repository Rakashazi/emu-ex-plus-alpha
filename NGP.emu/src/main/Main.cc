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
#include <emuframework/EmuSystemInlines.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <mednafen/state-driver.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/ngp/neopop.h>
#include <mednafen/ngp/flash.h>
#include <mednafen/ngp/sound.h>
#include <mednafen-emuex/MDFNUtils.hh>

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2023\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nNeoPop Team\nwww.nih.at";
bool EmuApp::needsGlobalInstance = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".ngc", ".ngp", ".npc", ".ngpc");
	};

const char *EmuSystem::shortSystemName() const
{
	return "NGP";
}

const char *EmuSystem::systemName() const
{
	return "Neo Geo Pocket";
}

void NgpSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	MDFN_IEN_NGP::reset();
}

FS::FileString NgpSystem::stateFilename(int slot, std::string_view name) const
{
	return stateFilenameMDFN(*MDFNGameInfo, slot, name, 'a');
}

void NgpSystem::saveState(IG::CStringView path)
{
	if(!MDFNI_SaveState(path, 0, 0, 0, 0))
		throwFileWriteError();
}

void NgpSystem::loadState(EmuApp &, IG::CStringView path)
{
	if(!MDFNI_LoadState(path, 0))
		throwFileReadError();
}

static FS::PathString saveFilename(const EmuApp &app)
{
	return app.contentSaveFilePath(".ngf");
}

void NgpSystem::loadBackupMemory(EmuApp &)
{
	logMsg("loading flash");
	MDFN_IEN_NGP::FLASH_LoadNV();
}

void NgpSystem::onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags)
{
	logMsg("saving flash");
	MDFN_IEN_NGP::FLASH_SaveNV();
}

IG::Time NgpSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(saveFilename(app).c_str());
}

void NgpSystem::closeSystem()
{
	mdfnGameInfo.CloseGame();
}

void NgpSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	mdfnGameInfo.name = std::string{EmuSystem::contentName()};
	static constexpr size_t maxRomSize = 0x400000;
	auto stream = std::make_unique<MemoryStream>(maxRomSize, true);
	auto size = io.read(stream->map(), stream->map_size());
	if(size <= 0)
		throwFileReadError();
	stream->setSize(size);
	MDFNFILE fp(&NVFS, std::move(stream));
	GameFile gf{&NVFS, std::string{contentDirectory()}, fp.stream(),
		std::string{withoutDotExtension(contentFileName())},
		std::string{contentName()}};
	mdfnGameInfo.Load(&gf);
	mdfnGameInfo.SetInput(0, "gamepad", (uint8*)&inputBuff);
	MDFN_IEN_NGP::applyVideoFormat(toMDFNSurface(mSurfacePix).format);
}

bool NgpSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	mSurfacePix = {{{vidBufferX, vidBufferY}, fmt}, pixBuff};
	if(!hasContent())
		return false;
	MDFN_IEN_NGP::applyVideoFormat(toMDFNSurface(mSurfacePix).format);
	return false;
}

void NgpSystem::configAudioRate(IG::FloatSeconds outputFrameTime, int outputRate)
{
	auto soundRate = audioMixRate(outputRate, outputFrameTime);
	logMsg("emu sound rate:%f", soundRate);
	MDFN_IEN_NGP::MDFNNGPC_SetSoundRate(soundRate);
}

void NgpSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	static constexpr size_t maxFrames = 48000 / minFrameRate;
	int16 audioBuff[maxFrames*2];
	EmulateSpecStruct espec{};
	if(audio)
	{
		espec.SoundBuf = audioBuff;
		espec.SoundBufMaxSize = maxFrames;
	}
	espec.taskCtx = taskCtx;
	espec.sys = this;
	espec.video = video;
	espec.skip = !video;
	auto mSurface = toMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	mdfnGameInfo.Emulate(&espec);
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
		{ .0, Gfx::PackedColor::format.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((34./255.) * .4, (15./255.) * .4, (64./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

namespace MDFN_IEN_NGP
{

bool system_io_flash_read(uint8_t* buffer, uint32_t len)
{
	using namespace EmuEx;
	auto saveStr = saveFilename(gApp());
	return IG::FileUtils::readFromUri(gAppContext(), saveStr, {buffer, len}) > 0;
}

void system_io_flash_write(uint8_t* buffer, uint32 len)
{
	using namespace EmuEx;
	if(!len)
		return;
	auto saveStr = saveFilename(gApp());
	logMsg("writing flash %s", saveStr.data());
	IG::FileUtils::writeToUri(gAppContext(), saveStr, {buffer, len}) != -1;
}

}

namespace Mednafen
{

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	espec->video->startFrameWithFormat(espec->taskCtx, static_cast<EmuEx::NgpSystem&>(*espec->sys).mSurfacePix);
}

}
