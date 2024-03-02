#pragma once

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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/io/FileIO.hh>
#include <mednafen/mednafen.h>
#include <compare>

extern const Mednafen::MDFNGI EmulatedWSwan;

namespace EmuEx
{

enum
{
	CFGKEY_USER_NAME = 256, CFGKEY_USER_PROFILE = 257,
	CFGKEY_SHOW_VGAMEPAD_Y_HORIZ = 258, CFGKEY_SHOW_VGAMEPAD_AB_VERT = 259,
	CFGKEY_WS_ROTATION = 260, CFGKEY_NO_MD5_FILENAMES = 261,
};

struct WsUserProfile
{
	unsigned birthYear:14;
	unsigned birthMonth:4;
	unsigned birthDay:5;
	unsigned sex:2;
	unsigned bloodType:3;
	unsigned languageIsEnglish:1;

	constexpr static uint32_t pack(WsUserProfile p)
	{
		return p.birthYear |
			p.birthMonth << 14 |
			p.birthDay << 18 |
			p.sex << 23 |
			p.bloodType << 25 |
			p.languageIsEnglish << 26;
	}

	constexpr static WsUserProfile unpack(uint32_t v)
	{
		return
		{
			.birthYear = v & IG::bits(14),
			.birthMonth = (v >> 14) & IG::bits(4),
			.birthDay = (v >> 18) & IG::bits(5),
			.sex = (v >> 23) & IG::bits(2),
			.bloodType = (v >> 25) & IG::bits(3),
			.languageIsEnglish = (v >> 26) & IG::bits(1)
		};
	}

	constexpr bool operator==(const WsUserProfile &o) const { return pack(*this) == pack(o); }
};

constexpr WsUserProfile defaultUserProfile
{
	.birthYear = 1999,
	.birthMonth = 3,
	.birthDay = 4,
	.sex = 3,
	.bloodType = 5,
	.languageIsEnglish = 0
};

WISE_ENUM_CLASS((WsRotation, uint8_t),
	Auto,
	Horizontal,
	Vertical);

class WsSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedWSwan};
	FileIO saveFileIO;
	IG::MutablePixmapView mSurfacePix{};
	static constexpr WSize vidBufferPx{224, 144};
	alignas(8) uint32_t pixBuff[vidBufferPx.x * vidBufferPx.y]{};
	IG::StaticString<16> userName{};
	WsUserProfile userProfile{defaultUserProfile};
	uint8_t configuredLCDVTotal{};
	bool showVGamepadYWhenHorizonal = true;
	bool showVGamepadABWhenVertical{};
	bool noMD5InFilenames{};
	WsRotation rotation{};

	WsSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
	}

	void setShowVGamepadYWhenHorizonal(bool);
	void setShowVGamepadABWhenVertical(bool);
	void setRotation(WsRotation);

	bool isRotated() const
	{
		switch(rotation)
		{
			case WsRotation::Auto: return mdfnGameInfo.rotated == Mednafen::MDFN_ROTATE90;
			case WsRotation::Horizontal: return false;
			case WsRotation::Vertical: return true;
		}
		bug_unreachable("invalid WsRotation");
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
	InputAction translateInputAction(InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const;
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &app) const;
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	IG::Rotation contentRotation() const;
	bool resetSessionOptions(EmuApp &app);

private:
	void setupInput(EmuApp &app);
};

using MainSystem = WsSystem;

}
