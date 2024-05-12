#pragma once

/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/io/FileIO.hh>

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

constexpr bool systemEnumIsValid(auto const &v)
{
	return v < SYS_MAX;
}

constexpr bool countryEnumIsValid(auto const &v)
{
	return v < CTY_MAX;
}

class NeoSystem final: public EmuSystem
{
public:
	static constexpr auto pixFmt = IG::PixelFmtRGB565;
	static constexpr int FBResX = 352;
	size_t saveStateSize{};
	FileIO nvramFileIO;
	FileIO memcardFileIO;
	GN_Surface sdlSurf{};
	uint16_t screenBuff[FBResX*256] __attribute__ ((aligned (8))){};
	FS::PathString datafilePath{};
	EmuSystem::OnLoadProgressDelegate onLoadProgress{};
	Property<bool, CFGKEY_LIST_ALL_GAMES> optionListAllGames;
	Property<uint8_t, CFGKEY_BIOS_TYPE,
		PropertyDesc<uint8_t>{.defaultValue = SYS_UNIBIOS, .isValid = systemEnumIsValid}> optionBIOSType;
	Property<uint8_t, CFGKEY_MVS_COUNTRY,
		PropertyDesc<uint8_t>{.defaultValue = CTY_USA, .isValid = countryEnumIsValid}> optionMVSCountry;
	Property<uint8_t, CFGKEY_TIMER_INT,
		PropertyDesc<uint8_t>{.defaultValue = 2}> optionTimerInt;
	Property<bool, CFGKEY_CREATE_USE_CACHE> optionCreateAndUseCache;
	Property<bool, CFGKEY_STRICT_ROM_CHECKING> optionStrictROMChecking;
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
	size_t stateSize();
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key);
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

bool openState(MapIO &io, int mode);
void makeState(MapIO &io, int mode);

}

constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags SRAM_DIRTY_BIT = IG::bit(0);
constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags MEMCARD_DIRTY_BIT = IG::bit(1);

CLINK CONFIG conf;
