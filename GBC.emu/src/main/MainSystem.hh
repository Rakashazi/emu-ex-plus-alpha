#pragma once

/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <main/Palette.hh>
#include <gambatte.h>
#include <libgambatte/src/video/lcddef.h>
#include <resample/resampler.h>
#include <imagine/fs/FS.hh>
#include <imagine/util/bit.hh>
#include <memory>
#include <vector>

namespace EmuEx
{

enum
{
	CFGKEY_GB_PAL_IDX = 270, CFGKEY_REPORT_AS_GBA = 271,
	CFGKEY_FULL_GBC_SATURATION = 272, CFGKEY_AUDIO_RESAMPLER = 273,
	CFGKEY_USE_BUILTIN_GB_PAL = 274, CFGKEY_RENDER_PIXEL_FORMAT_UNUSED = 275,
	CFGKEY_CHEATS_PATH = 276,
};

constexpr unsigned COLOR_CONVERSION_SATURATED_BIT = bit(0);
constexpr unsigned COLOR_CONVERSION_BGR_BIT = bit(1);

class CheatCode: public StaticString<12> {};

class Cheat
{
public:
	StaticString<64> name;
	std::vector<CheatCode> codes{};
	uint8_t on:1{};

	bool operator==(Cheat const& rhs) const { return codes == rhs.codes; }
};

class GbcInput final : public gambatte::InputGetter
{
public:
	unsigned bits{};

	constexpr GbcInput() = default;
	constexpr unsigned operator()() final { return bits; }
};

class GbcSystem final: public EmuSystem
{
public:
	gambatte::GB gbEmu;
	GbcInput gbcInput;
	std::unique_ptr<Resampler> resampler;
	const GBPalette *gameBuiltinPalette{};
	FileIO saveFileIO;
	FileIO rtcFileIO;
	std::string cheatsDir;
	std::vector<Cheat> cheatList;
	size_t saveStateSize{};
	uint64_t totalSamples{};
	uint32_t totalFrames{};
	uint8_t activeResampler = 1;
	bool useBgrOrder{};
	alignas(8) uint_least32_t frameBuffer[gambatte::lcd_hres * gambatte::lcd_vres];

	Property<uint8_t, CFGKEY_GB_PAL_IDX,
		PropertyDesc<uint8_t>{.isValid = isValidWithMax<gbNumPalettes-1>}> optionGBPal;
	Property<bool, CFGKEY_USE_BUILTIN_GB_PAL,
		PropertyDesc<bool>{.defaultValue = true}> optionUseBuiltinGBPalette;
	Property<bool, CFGKEY_REPORT_AS_GBA> optionReportAsGba;
	Property<uint8_t, CFGKEY_AUDIO_RESAMPLER,
		PropertyDesc<uint8_t>{.defaultValue = 1}> optionAudioResampler;
	Property<bool, CFGKEY_FULL_GBC_SATURATION> optionFullGbcSaturation;
	static constexpr FloatSeconds gbFrameTimeSecs{70224. / 4194304.}; // ~59.7275Hz
	static constexpr auto gbFrameTime{round<FrameTime>(gbFrameTimeSecs)};

	GbcSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		gbEmu.setInputGetter(&gbcInput);
	}
	void applyGBPalette();
	void applyCheats();
	void readCheatFile();
	void writeCheatFile();
	void refreshPalettes();

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	size_t stateSize() { return saveStateSize; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return gbFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	void closeSystem();
	void onOptionsLoaded();
	bool resetSessionOptions(EmuApp &);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	void renderFramebuffer(EmuVideo &);
	Cheat* newCheat(EmuApp&, const char* name, CheatCodeDesc);
	bool setCheatName(Cheat&, const char* name);
	std::string_view cheatName(const Cheat&) const;
	void setCheatEnabled(Cheat&, bool on);
	bool isCheatEnabled(const Cheat&) const;
	bool addCheatCode(EmuApp&, Cheat*&, CheatCodeDesc);
	bool modifyCheatCode(EmuApp&, Cheat&, CheatCode&, CheatCodeDesc);
	Cheat* removeCheatCode(Cheat&, CheatCode&);
	bool removeCheat(Cheat&);
	void forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)>);
	void forEachCheatCode(Cheat&, DelegateFunc<bool(CheatCode&, std::string_view)>);

protected:
	uint_least32_t makeOutputColor(uint_least32_t rgb888) const;
	size_t runUntilVideoFrame(gambatte::uint_least32_t *videoBuf, std::ptrdiff_t pitch,
		EmuAudio *audio, gambatte::VideoFrameDelegate videoFrameCallback);
	void renderVideo(const EmuSystemTaskContext &taskCtx, EmuVideo &video);
	void updateColorConversionFlags();
};

using MainSystem = GbcSystem;

}
