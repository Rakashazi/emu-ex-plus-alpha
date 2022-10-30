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
#include <mednafen/hash/md5.h>
#include <mednafen/MemoryStream.h>
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/video/surface.h>
using namespace Mednafen; // needed for following includes
#include <mednafen/wswan/gfx.h>
#include <mednafen/wswan/sound.h>
#include <mednafen/wswan/memory.h>

namespace EmuEx
{

using namespace MDFN_IEN_WSWAN;

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.sourceforge.net";

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".ws", ".wsc", ".bin");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = defaultFsFilter;
double EmuSystem::staticFrameTime = (159. * 256.) / 3072000.;
bool EmuApp::needsGlobalInstance = true;

const char *EmuSystem::shortSystemName() const { return "WS"; }
const char *EmuSystem::systemName() const { return "WonderSwan"; }

void WsSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	MDFN_DoSimpleCommand(MDFN_MSC_RESET);
}

FS::FileString WsSystem::stateFilename(int slot, std::string_view name) const
{
	return stateFilenameMDFN(*MDFNGameInfo, slot, name, 'a');
}

void WsSystem::saveState(IG::CStringView path)
{
	if(!MDFNI_SaveState(path, 0, 0, 0, 0))
		throwFileWriteError();
}

void WsSystem::loadState(EmuApp &, IG::CStringView path)
{
	if(!MDFNI_LoadState(path, 0))
		throwFileReadError();
}

void WsSystem::loadBackupMemory(EmuApp &app)
{
	WSwan_MemoryLoadNV();
}

void WsSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	WSwan_MemorySaveNV();
}

IG::Time WsSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(savePathMDFN(app, 0, "sav").c_str());
}

void WsSystem::closeSystem()
{
	mdfnGameInfo.CloseGame();
	mdfnGameInfo.rotated = MDFN_ROTATE0;
}

void WsSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	mdfnGameInfo.name = std::string{EmuSystem::contentName()};
	static constexpr size_t maxRomSize = 0x4000000;
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
	setupInput(EmuApp::get(appContext()));
	WSwan_SetPixelFormat(toMDFNSurface(mSurfacePix).format);
}

bool WsSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	mSurfacePix = {{{vidBufferX, vidBufferY}, fmt}, pixBuff};
	if(!hasContent())
		return false;
	WSwan_SetPixelFormat(toMDFNSurface(mSurfacePix).format);
	return false;
}

void WsSystem::configAudioRate(IG::FloatSeconds frameTime, int rate)
{
	if(!hasContent())
		return;
	auto soundRate = std::round(rate / staticFrameTime * frameTime.count());
	logMsg("emu sound rate:%f", soundRate);
	WSwan_SetSoundRate(soundRate);
}

void WsSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	static constexpr int maxFrames = 48000/54;
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
		assert((size_t)espec.SoundBufSize <= audio->format().bytesToFrames(sizeof(audioBuff)));
		audio->writeFrames((uint8_t*)audioBuff, espec.SoundBufSize);
	}
}

IG::Rotation WsSystem::contentRotation() const
{
	return isRotated() ? Rotation::RIGHT : Rotation::UP;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build((0./255.) * .4, (158./255.) * .4, (211./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((0./255.) * .4, (158./255.) * .4, (211./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((0./255.) * .4, (53./255.) * .4, (70./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}

namespace Mednafen
{

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	espec->video->startFrameWithFormat(espec->taskCtx, static_cast<EmuEx::WsSystem&>(*espec->sys).mSurfacePix);
}

}
