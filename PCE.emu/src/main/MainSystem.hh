#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <mednafen/mednafen.h>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

namespace EmuEx::Controls
{
extern const unsigned gamepadKeys;
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
	CFGKEY_ADPCM_FILTER = 285,

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

class PceSystem final: public EmuSystem
{
public:
	std::array<uint16, 5> inputBuff{}; // 5 gamepad buffers
	static constexpr int vidBufferX = 512, vidBufferY = 242;
	alignas(8) uint32_t pixBuff[vidBufferX*vidBufferY]{};
	IG::MutablePixmapView mSurfacePix{};
	bool prevUsing263Lines{};
	std::vector<CDInterface *> CDInterfaces;
	FS::PathString sysCardPath{};
	Byte1Option optionArcadeCard{CFGKEY_ARCADE_CARD, 1};
	Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
	VisibleLines defaultVisibleLines{};
	VisibleLines visibleLines{};
	uint8_t cdSpeed{2};
	uint8_t cddaVolume{100};
	uint8_t adpcmVolume{100};
	bool noSpriteLimit{};
	bool correctLineAspect{};
	bool adpcmFilter{};

	PceSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setVisibleLines(VisibleLines);
	void setNoSpriteLimit(bool);
	void setCdSpeed(uint8_t);
	void setVolume(VolumeType, uint8_t volume);
	uint8_t volume(VolumeType type) { return  volumeVar(type); }
	void setAdpcmFilter(bool);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	unsigned translateInputAction(unsigned input, bool &turbo);
	VController::Map vControllerMap(int player);
	void configAudioRate(FloatSeconds frameTime, int rate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void onFlushBackupMemory(BackupMemoryDirtyFlags);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	WP multiresVideoBaseSize() const;
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
	double videoAspectRatioScale() const;

private:
	void updateCdSettings();

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

namespace MDFN_IEN_PCE_FAST
{
void applySoundFormat(double rate);
extern vce_t vce;
}

extern Mednafen::MDFNGI EmulatedPCE_Fast;
static Mednafen::MDFNGI *emuSys = &EmulatedPCE_Fast;
