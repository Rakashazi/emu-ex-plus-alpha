#pragma once

/*  This file is part of Lynx.emu.

	Lynx.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Lynx.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Lynx.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>
#include <compare>

extern const Mednafen::MDFNGI EmulatedLynx;

namespace EmuEx
{

enum
{
	CFGKEY_BIOS = 256, CFGKEY_LYNX_ROTATION = 257,
	CFGKEY_LOWPASS_FILTER = 258,
};

WISE_ENUM_CLASS((LynxRotation, uint8_t),
	Auto,
	Horizontal,
	VerticalLeft,
	VerticalRight);

class LynxSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedLynx};
	uint16_t inputBuff{};
	IG::MutablePixmapView mSurfacePix{};
	static constexpr IP vidBufferPx{160, 102};
	alignas(8) uint32_t pixBuff[vidBufferPx.x * vidBufferPx.y]{};
	uint8_t configuredHCount{};
	LynxRotation rotation{};
	bool lowpassFilter{};
	FS::PathString biosPath;

	LynxSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
	}

	void setRotation(LynxRotation);
	void setLowpassFilter(bool on);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".mca"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	InputAction translateInputAction(InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FloatSeconds frameTime() const;
	void configAudioRate(FloatSeconds outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	IG::Rotation contentRotation() const;
	bool resetSessionOptions(EmuApp &app);

private:
	unsigned rotateDPadKeycode(unsigned code) const;
};

using MainSystem = LynxSystem;

}