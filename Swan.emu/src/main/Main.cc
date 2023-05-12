/*  This file is part of Swan.emu.

	Swan.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Swan.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Swan.emu.  If not, see <http://www.gnu.org/licenses/> */

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
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/video/surface.h>
using namespace Mednafen; // needed for following includes
#include <mednafen/wswan/gfx.h>
#include <mednafen/wswan/sound.h>
#include <mednafen/wswan/memory.h>

namespace MDFN_IEN_WSWAN
{
uint32 GetSoundRate();
}


namespace EmuEx
{

using namespace MDFN_IEN_WSWAN;

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2023\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.github.io";
bool EmuApp::needsGlobalInstance = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".ws", ".wsc", ".bin");
	};

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
	if(!eeprom_size && !sram_size)
		return;
	logMsg("loading sram/eeprom");
	if(!saveFileIO)
		saveFileIO = staticBackupMemoryFile(savePathMDFN(app, 0, "sav"), eeprom_size + sram_size);
	if(eeprom_size)
		saveFileIO.read(wsEEPROM, eeprom_size, 0);
	if(sram_size)
		saveFileIO.read(wsSRAM, sram_size, eeprom_size);
}

void WsSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	if(!eeprom_size && !sram_size)
		return;
	logMsg("saving sram/eeprom");
	if(eeprom_size)
		saveFileIO.write(wsEEPROM, eeprom_size, 0);
	if(sram_size)
		saveFileIO.write(wsSRAM, sram_size, eeprom_size);
}

WallClockTimePoint WsSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(savePathMDFN(app, 0, "sav").c_str());
}

void WsSystem::closeSystem()
{
	mdfnGameInfo.CloseGame();
	mdfnGameInfo.rotated = MDFN_ROTATE0;
	saveFileIO = {};
}

void WsSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	static constexpr size_t maxRomSize = 0x4000000;
	EmuEx::loadContent(*this, mdfnGameInfo, io, maxRomSize);
	setupInput(EmuApp::get(appContext()));
	WSwan_SetPixelFormat(toMDFNSurface(mSurfacePix).format);
}

bool WsSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	mSurfacePix = {{vidBufferPx, fmt}, pixBuff};
	if(!hasContent())
		return false;
	WSwan_SetPixelFormat(toMDFNSurface(mSurfacePix).format);
	return false;
}

static uint8_t lcdVTotal() { return WSwan_GfxRead(0x16) + 1; }

FrameTime WsSystem::frameTime() const { return round<FrameTime>(FloatSeconds{lcdVTotal() * 256 / 3072000.}); }

void WsSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	uint32 mixRate = std::round(audioMixRate(outputRate, outputFrameTime));
	configuredLCDVTotal = lcdVTotal();
	if(GetSoundRate() == mixRate)
		return;
	logMsg("set sound mix rate:%d", (int)mixRate);
	WSwan_SetSoundRate(mixRate);
}

void WsSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	static constexpr size_t maxAudioFrames = 48000 / minFrameRate;
	EmuEx::runFrame(*this, mdfnGameInfo, taskCtx, video, mSurfacePix, audio, maxAudioFrames);
	if(configuredLCDVTotal != lcdVTotal()) [[unlikely]]
	{
		onFrameTimeChanged();
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
		{ .0, Gfx::PackedColor::format.build((0./255.) * .4, (158./255.) * .4, (211./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((0./255.) * .4, (158./255.) * .4, (211./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((0./255.) * .4, (53./255.) * .4, (70./255.) * .4, 1.) },
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
