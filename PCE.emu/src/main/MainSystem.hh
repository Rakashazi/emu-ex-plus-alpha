#pragma once

/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuSystem.hh>
#include <mednafen/mednafen.h>
#include <pce/vce.h>
#include <pce_fast/pce.h>
#include <pce_fast/vdc.h>

extern const Mednafen::MDFNGI EmulatedPCE_Fast;
extern const Mednafen::MDFNGI EmulatedPCE;

namespace Mednafen
{
class CDInterface;

void SCSICD_SetDisc(bool tray_open, CDInterface* cdif, bool no_emu_side_effects = false);
}

namespace MDFN_IEN_PCE_FAST
{
extern vce_t vce;

void SetSoundRate(double rate);
double GetSoundRate();
void PCECD_Drive_SetDisc(bool tray_open, CDInterface* cdif, bool no_emu_side_effects = false) MDFN_COLD;
}

namespace MDFN_IEN_PCE
{
class VCE;

extern VCE *vce;

bool SetSoundRate(double rate);
double GetSoundRate();
}

namespace EmuEx
{

class EmuApp;

enum
{
	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
	CFGKEY_6_BTN_PAD = 277, CFGKEY_VISIBLE_LINES = 278,
	CFGKEY_DEFAULT_VISIBLE_LINES = 279, CFGKEY_CORRECT_LINE_ASPECT = 280,
	CFGKEY_NO_SPRITE_LIMIT = 281, CFGKEY_CD_SPEED = 282,
	CFGKEY_CDDA_VOLUME = 283, CFGKEY_ADPCM_VOLUME = 284,
	CFGKEY_ADPCM_FILTER = 285, CFGKEY_EMU_CORE = 286,
	CFGKEY_NO_MD5_FILENAMES = 287,
};

void set6ButtonPadEnabled(EmuApp &, bool);
bool hasHuCardExtension(std::string_view name);

struct VisibleLines
{
	uint8_t first{11};
	uint8_t last{234};

	constexpr bool operator==(const VisibleLines&) const = default;
};

enum class VolumeType
{
	CDDA, ADPCM
};

WISE_ENUM_CLASS((EmuCore, uint8_t),
	Auto,
	Fast,
	Accurate);

inline std::string_view asModuleString(EmuCore c) { return c == EmuCore::Accurate ? "pce" : "pce_fast"; }

class PceSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedPCE_Fast};
	std::array<uint16, 5> inputBuff; // 5 gamepad buffers
	static constexpr int maxFrameBuffWidth = 1365, maxFrameBuffHeight = 270;
	alignas(8) uint32_t pixBuff[maxFrameBuffWidth * maxFrameBuffHeight];
	IG::MutablePixmapView mSurfacePix;
	bool configuredFor263Lines{};
	std::vector<CDInterface *> CDInterfaces;
	FS::PathString sysCardPath;
	Property<bool, CFGKEY_ARCADE_CARD, PropertyDesc<bool>{.defaultValue = true}> optionArcadeCard;
	Property<bool, CFGKEY_6_BTN_PAD> option6BtnPad;
	VisibleLines defaultVisibleLines{};
	VisibleLines visibleLines{};
	uint8_t cdSpeed{2};
	uint8_t cddaVolume{100};
	uint8_t adpcmVolume{100};
	bool noSpriteLimit{};
	bool correctLineAspect{};
	bool adpcmFilter{};
	bool noMD5InFilenames{};
	EmuCore defaultCore{};
	EmuCore core{};

	PceSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
	}
	void setVisibleLines(VisibleLines);
	void setNoSpriteLimit(bool);
	void setCdSpeed(uint8_t);
	void setVolume(VolumeType, uint8_t volume);
	uint8_t volume(VolumeType type) { return  volumeVar(type); }
	void setAdpcmFilter(bool);
	bool isUsingAccurateCore() const { return mdfnGameInfo.fb_width == 1365; }
	EmuCore resolvedCore(EmuCore c) const { return c == EmuCore::Auto ? defaultCore : c; }
	EmuCore resolvedCore() const { return resolvedCore(core); }
	EmuCore resolvedDefaultCore() const { return defaultCore == EmuCore::Auto ? EmuCore::Fast : defaultCore; }

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
	FrameTime frameTime() const;
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	WSize multiresVideoBaseSize() const;
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	double videoAspectRatioScale() const;

private:
	void updateCdSettings();
	void updatePixmap(IG::PixelFormat);

	bool isUsing263Lines() const
	{
		if(isUsingAccurateCore())
			return MDFN_IEN_PCE::vce ? MDFN_IEN_PCE::vce->CR & 0x04 : false;
		else
			return MDFN_IEN_PCE_FAST::vce.CR & 0x04;
	}

	uint8_t &volumeVar(VolumeType vol)
	{
		switch(vol)
		{
			case VolumeType::CDDA: return cddaVolume;
			case VolumeType::ADPCM: return adpcmVolume;
		}
		bug_unreachable("invalid VolumeType");
	}
};

using MainSystem = PceSystem;

}
