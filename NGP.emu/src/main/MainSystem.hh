#pragma once

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

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/EmuOptions.hh>
#include <mednafen/mednafen.h>

extern const Mednafen::MDFNGI EmulatedNGP;

namespace EmuEx
{

enum
{
	CFGKEY_NGPKEY_LANGUAGE = 269, CFGKEY_NO_MD5_FILENAMES = 270,
};

class NgpSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedNGP};
	Property<bool, CFGKEY_NGPKEY_LANGUAGE, PropertyDesc<bool>{.defaultValue = true}> optionNGPLanguage;
	uint8_t inputBuff{};
	MutablePixmapView mSurfacePix{};
	static constexpr WSize vidBufferPx{160, 152};
	alignas(8) uint32_t pixBuff[vidBufferPx.x * vidBufferPx.y]{};
	bool noMD5InFilenames{};
	// TODO: Mednafen/Neopop timing is based on 199 lines/frame, verify if this is correct
	static constexpr auto ngpFrameTime{fromSeconds<FrameTime>(199. * 515. / 6144000.)}; //~59.95Hz

	NgpSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
		mdfnGameInfo.SetInput(0, "gamepad", (uint8*)&inputBuff);
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".mca"; }
	size_t stateSize();
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags);
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return ngpFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
};

using MainSystem = NgpSystem;

}
