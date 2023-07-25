#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

extern "C"
{
	#include <gngeo/emu.h>
}

namespace EmuEx::Controls
{
static const unsigned joystickKeys = 19;
}

namespace EmuEx
{

enum
{
	CFGKEY_LIST_ALL_GAMES = 275, CFGKEY_BIOS_TYPE = 276,
	CFGKEY_MVS_COUNTRY = 277, CFGKEY_TIMER_INT = 278,
	CFGKEY_CREATE_USE_CACHE = 279,
	CFGKEY_NEOGEOKEY_TEST_SWITCH = 280, CFGKEY_STRICT_ROM_CHECKING = 281
};

inline bool systemEnumIsValid(uint8_t val)
{
	return val < SYS_MAX;
}

inline bool countryEnumIsValid(uint8_t val)
{
	return val < CTY_MAX;
}

class NeoSystem final: public EmuSystem
{
public:
	static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
	static constexpr int FBResX = 352;
	FileIO nvramFileIO;
	FileIO memcardFileIO;
	GN_Surface sdlSurf{};
	uint16_t screenBuff[FBResX*256] __attribute__ ((aligned (8))){};
	FS::PathString datafilePath{};
	EmuSystem::OnLoadProgressDelegate onLoadProgress{};
	Byte1Option optionListAllGames{CFGKEY_LIST_ALL_GAMES, 0};
	Byte1Option optionBIOSType{CFGKEY_BIOS_TYPE, SYS_UNIBIOS, 0, systemEnumIsValid};
	Byte1Option optionMVSCountry{CFGKEY_MVS_COUNTRY, CTY_USA, 0, countryEnumIsValid};
	Byte1Option optionTimerInt{CFGKEY_TIMER_INT, 2};
	Byte1Option optionCreateAndUseCache{CFGKEY_CREATE_USE_CACHE, 0};
	Byte1Option optionStrictROMChecking{CFGKEY_STRICT_ROM_CHECKING, 0};
	static constexpr auto neogeoFrameTime{fromSeconds<FrameTime>(264. / 15625.)}; // ~59.18Hz

	NeoSystem(ApplicationContext ctx);
	void setTimerIntOption();
	PixmapView videoPixmap()
	{
		// start image on y 16, x 24, size 304x224, 48 pixel padding on the right
		return {{{304, 224}, pixFmt}, screenBuff + (16*FBResX) + (24), {FBResX, IG::PixmapView::Units::PIXEL}};
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return neogeoFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	bool resetSessionOptions(EmuApp &);
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	FS::FileString contentDisplayNameForPath(IG::CStringView path) const;
};

using MainSystem = NeoSystem;

}

constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags SRAM_DIRTY_BIT = IG::bit(0);
constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags MEMCARD_DIRTY_BIT = IG::bit(1);

CLINK CONFIG conf;
