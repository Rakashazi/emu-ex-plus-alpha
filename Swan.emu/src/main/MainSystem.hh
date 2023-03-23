#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>
#include <compare>

extern const Mednafen::MDFNGI EmulatedWSwan;

namespace EmuEx
{

enum
{
	CFGKEY_USER_NAME = 256, CFGKEY_USER_PROFILE = 257,
	CFGKEY_SHOW_VGAMEPAD_Y_HORIZ = 258, CFGKEY_SHOW_VGAMEPAD_AB_VERT = 259,
	CFGKEY_WS_ROTATION = 260,
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
	uint16_t inputBuff{};
	IG::MutablePixmapView mSurfacePix{};
	static constexpr int vidBufferX = 224, vidBufferY = 144;
	alignas(8) uint32_t pixBuff[vidBufferX*vidBufferY]{};
	IG::StaticString<16> userName{};
	WsUserProfile userProfile{defaultUserProfile};
	uint8_t prevLCDVTotal{};
	bool showVGamepadYWhenHorizonal = true;
	bool showVGamepadABWhenVertical{};
	WsRotation rotation{};

	WsSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
		mdfnGameInfo.SetInput(0, "gamepad", (uint8*)&inputBuff);
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
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	IG::Time backupMemoryLastWriteTime(const EmuApp &app) const;
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	IG::Rotation contentRotation() const;
	bool resetSessionOptions(EmuApp &app);

private:
	void setupInput(EmuApp &app);
};

using MainSystem = WsSystem;

}
