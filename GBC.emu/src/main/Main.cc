/*  This file is part of GBA.emu.

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

#define LOGTAG "main"
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>
#include <gambatte.h>
#include <resample/resampler.h>
#include <resample/resamplerinfo.h>
#include <main/Cheats.hh>
#include <main/Palette.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011\nthe Gambatte Team\ngambatte.sourceforge.net";
gambatte::GB gbEmu;
static Resampler *resampler = nullptr;
static uint8 activeResampler = 1;
static const GBPalette *gameBuiltinPalette = nullptr;
#ifdef __clang__
PathOption optionFirmwarePath(0, nullptr, 0, nullptr); // unused, make linker happy
#endif

class ImagineFile : public gambatte::File
{
public:
	ImagineFile(Io &io): io(io) {}

	~ImagineFile() override {}

	void rewind() override
	{
		io.seekA(0);
	}

	std::size_t size() const override
	{
		return io.size();
	}

	void read(char *buffer, std::size_t amount) override
	{
		if(io.read(buffer, amount) != OK)
			failed = true;
	}

	bool fail() const override { return failed; }

private:
	Io &io;
	bool failed = false;
};

// controls

enum
{
	gbcKeyIdxUp = EmuControls::systemKeyMapStart,
	gbcKeyIdxRight,
	gbcKeyIdxDown,
	gbcKeyIdxLeft,
	gbcKeyIdxLeftUp,
	gbcKeyIdxRightUp,
	gbcKeyIdxRightDown,
	gbcKeyIdxLeftDown,
	gbcKeyIdxSelect,
	gbcKeyIdxStart,
	gbcKeyIdxA,
	gbcKeyIdxB,
	gbcKeyIdxATurbo,
	gbcKeyIdxBTurbo
};

enum {
	CFGKEY_GB_PAL_IDX = 270, CFGKEY_REPORT_AS_GBA = 271,
	CFGKEY_FULL_GBC_SATURATION = 272, CFGKEY_AUDIO_RESAMPLER = 273,
	CFGKEY_USE_BUILTIN_GB_PAL = 274
};

static GBPalette gbPal[] =
{
	{ { 0xd7e894, 0xaec440, 0x527f39, 0x204631 }, { 0xd7e894, 0xaec440, 0x527f39, 0x204631 }, { 0xd7e894, 0xaec440, 0x527f39, 0x204631 } }, // Original Green-scale
	{ { 0xfcfafc, 0xec9a54, 0x844204, 0x040204 }, { 0xfcfafc, 0xec9a54, 0x844204, 0x040204 }, { 0xfcfafc, 0xec9a54, 0x844204, 0x040204 } }, // Brown
	{ { 0xfcfafc, 0xec8a8c, 0xac2624, 0x040204 }, { 0xfcfafc, 0x04fa04, 0x4c8a04, 0x040204 }, { 0xfcfafc, 0x7caafc, 0x0432fc, 0x040204 } }, // Red
	{ { 0xfceae4, 0xc4ae94, 0x947a4c, 0x4c2a04 }, { 0xfceae4, 0xec9a54, 0x844204, 0x000000 }, { 0xfceae4, 0xec9a54, 0x844204, 0x000000 } }, // Dark Brown
	{ { 0xfcfaac, 0xec8a8c, 0x9c92f4, 0x040204 }, { 0xfcfaac, 0xec8a8c, 0x9c92f4, 0x040204 }, { 0xfcfaac, 0xec8a8c, 0x9c92f4, 0x040204 } }, // Pastel mix
	{ { 0xfcfafc, 0xf4fe04, 0xfc3204, 0x040204 }, { 0xfcfafc, 0xf4fe04, 0xfc3204, 0x040204 }, { 0xfcfafc, 0xf4fe04, 0xfc3204, 0x040204 } }, // Orange
	{ { 0xfcfafc, 0xf4fe04, 0x844204, 0x040204 }, { 0xfcfafc, 0x7caafc, 0x0432fc, 0x040204 }, { 0xfcfafc, 0x04fa04, 0x4c8a04, 0x040204 } }, // Yellow
	{ { 0xfcfafc, 0x7caafc, 0x0432fc, 0x040204 }, { 0xfcfafc, 0xec8a8c, 0xac2624, 0x040204 }, { 0xfcfafc, 0x04fa04, 0x4c8a04, 0x040204 } }, // Blue
	{ { 0xfcfafc, 0x9c92f4, 0x4432a4, 0x040204 }, { 0xfcfafc, 0xec8a8c, 0xac2624, 0x040204 }, { 0xfcfafc, 0xec9a54, 0x844204, 0x040204 } }, // Dark blue
	{ { 0xfcfafc, 0xbcbabc, 0x747274, 0x040204 }, { 0xfcfafc, 0xbcbabc, 0x747274, 0x040204 }, { 0xfcfafc, 0xbcbabc, 0x747274, 0x040204 } }, // Gray
	{ { 0xfcfafc, 0x04fa04, 0xfc3204, 0x040204 }, { 0xfcfafc, 0x04fa04, 0xfc3204, 0x040204 }, { 0xfcfafc, 0x04fa04, 0xfc3204, 0x040204 } }, // Green
	{ { 0xfcfafc, 0x04fa04, 0x0432fc, 0x040204 }, { 0xfcfafc, 0xec8a8c, 0xac2624, 0x040204 }, { 0xfcfafc, 0xec8a8c, 0xac2624, 0x040204 } }, // Dark green
	{ { 0x040204, 0x04a2a4, 0xf4fe04, 0xfcfafc }, { 0x040204, 0x04a2a4, 0xf4fe04, 0xfcfafc }, { 0x040204, 0x04a2a4, 0xf4fe04, 0xfcfafc } }, // Reverse
};

static Byte1Option optionGBPal
		(CFGKEY_GB_PAL_IDX, 0, 0, optionIsValidWithMax<sizeofArray(gbPal)-1>);
static Byte1Option optionUseBuiltinGBPalette(CFGKEY_USE_BUILTIN_GB_PAL, 1);
static Byte1Option optionReportAsGba(CFGKEY_REPORT_AS_GBA, 0);
static Byte1Option optionAudioResampler(CFGKEY_AUDIO_RESAMPLER, 1);

static void applyGBPalette()
{
	uint idx = optionGBPal;
	assert(idx < sizeofArray(gbPal));
	bool useBuiltin = optionUseBuiltinGBPalette && gameBuiltinPalette;
	if(useBuiltin)
		logMsg("using built-in game palette");
	else
		logMsg("using palette index %d", idx);
	const GBPalette &pal = useBuiltin ? *gameBuiltinPalette : gbPal[idx];
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(0, i, pal.bg[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(1, i, pal.sp1[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(2, i, pal.sp2[i]);
}

static class GbcInput : public gambatte::InputGetter
{
public:
#ifndef __clang__
	constexpr GbcInput() {}
#endif
	unsigned bits = 0;
	unsigned operator()() override { return bits; }
} gbcInput;

namespace gambatte
{
extern bool useFullColorSaturation;
}

static Option<OptionMethodRef<bool, gambatte::useFullColorSaturation>, uint8> optionFullGbcSaturation(CFGKEY_FULL_GBC_SATURATION, 0);

const uint EmuSystem::maxPlayers = 1;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"10:9 (Original)", 10, 9},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
#include "CommonGui.hh"

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game.gb"	}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "GB";
}

const char *EmuSystem::systemName()
{
	return "Game Boy";
}

bool EmuSystem::readConfig(Io &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_GB_PAL_IDX: optionGBPal.readFromIO(io, readSize);
		bcase CFGKEY_REPORT_AS_GBA: optionReportAsGba.readFromIO(io, readSize);
		bcase CFGKEY_FULL_GBC_SATURATION: optionFullGbcSaturation.readFromIO(io, readSize);
		bcase CFGKEY_AUDIO_RESAMPLER: optionAudioResampler.readFromIO(io, readSize);
		bcase CFGKEY_USE_BUILTIN_GB_PAL: optionUseBuiltinGBPalette.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionGBPal.writeWithKeyIfNotDefault(io);
	optionReportAsGba.writeWithKeyIfNotDefault(io);
	optionFullGbcSaturation.writeWithKeyIfNotDefault(io);
	optionAudioResampler.writeWithKeyIfNotDefault(io);
	optionUseBuiltinGBPalette.writeWithKeyIfNotDefault(io);
}

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded()
{
	gbEmu.setInputGetter(&gbcInput);
}

static bool isROMExtension(const char *name)
{
	return string_hasDotExtension(name, "gb") ||
			string_hasDotExtension(name, "gbc");
}

static bool isGBCExtension(const char *name)
{
	return isROMExtension(name) || string_hasDotExtension(name, "zip");
}

static int gbcFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isGBCExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = gbcFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = gbcFsFilter;

static const int gbResX = 160, gbResY = 144;

#ifdef GAMBATTE_COLOR_RGB565
	static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
#else
	static const PixelFormatDesc *pixFmt = &PixelFormatBGRA8888;
#endif

static const uint PADDING_HACK_SIZE =
#ifdef CONFIG_BASE_ANDROID
	gbResX*9; // Adreno 205 crashing due to driver bug reading beyond the array bounds, add some padding
#else
	0;
#endif

static gambatte::PixelType screenBuff[(gbResX*gbResY)+PADDING_HACK_SIZE] __attribute__ ((aligned (8))) {0};

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	using namespace gambatte;
	map[SysVController::F_ELEM] = InputGetter::A;
	map[SysVController::F_ELEM+1] = InputGetter::B;

	map[SysVController::C_ELEM] = InputGetter::SELECT;
	map[SysVController::C_ELEM+1] = InputGetter::START;

	map[SysVController::D_ELEM] = InputGetter::UP | InputGetter::LEFT;
	map[SysVController::D_ELEM+1] = InputGetter::UP;
	map[SysVController::D_ELEM+2] = InputGetter::UP | InputGetter::RIGHT;
	map[SysVController::D_ELEM+3] = InputGetter::LEFT;
	map[SysVController::D_ELEM+5] = InputGetter::RIGHT;
	map[SysVController::D_ELEM+6] = InputGetter::DOWN | InputGetter::LEFT;
	map[SysVController::D_ELEM+7] = InputGetter::DOWN;
	map[SysVController::D_ELEM+8] = InputGetter::DOWN | InputGetter::RIGHT;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	using namespace gambatte;
	turbo = 0;
	switch(input)
	{
		case gbcKeyIdxUp: return InputGetter::UP;
		case gbcKeyIdxRight: return InputGetter::RIGHT;
		case gbcKeyIdxDown: return InputGetter::DOWN;
		case gbcKeyIdxLeft: return InputGetter::LEFT;
		case gbcKeyIdxLeftUp: return InputGetter::LEFT | InputGetter::UP;
		case gbcKeyIdxRightUp: return InputGetter::RIGHT | InputGetter::UP;
		case gbcKeyIdxRightDown: return InputGetter::RIGHT | InputGetter::DOWN;
		case gbcKeyIdxLeftDown: return InputGetter::LEFT | InputGetter::DOWN;
		case gbcKeyIdxSelect: return InputGetter::SELECT;
		case gbcKeyIdxStart: return InputGetter::START;
		case gbcKeyIdxATurbo: turbo = 1;
		case gbcKeyIdxA: return InputGetter::A;
		case gbcKeyIdxBTurbo: turbo = 1;
		case gbcKeyIdxB: return InputGetter::B;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	if(state == Input::PUSHED)
		setBits(gbcInput.bits, emuKey);
	else
		unsetBits(gbcInput.bits, emuKey);
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	gbEmu.reset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FsSys::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return makeFSPathStringPrintf("%s/%s.0%c.gqs", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	logMsg("saving state %s", saveStr.data());
	if(!gbEmu.saveState(/*screenBuff*/0, 160, saveStr.data()))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FsSys::fileExists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(!gbEmu.loadState(saveStr.data()))
			return STATE_RESULT_IO_ERROR;
		else
			return STATE_RESULT_OK;
	}
	return STATE_RESULT_NO_FILE;
}

void EmuSystem::saveBackupMem()
{
	logMsg("saving battery");
	gbEmu.saveSavedata();

	writeCheatFile();
}

void EmuSystem::savePathChanged()
{
	if(gameIsRunning())
		gbEmu.setSaveDir(savePath());
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		logMsg("saving auto-state");
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		gbEmu.saveState(/*screenBuff*/0, 160, saveStr.data());
	}
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
	cheatList.clear();
	cheatsModified = 0;
	gameBuiltinPalette = nullptr;
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

static int loadGameCommon(gambatte::LoadRes result)
{
	if(result != gambatte::LOADRES_OK)
	{
		popup.printf(3, 1, "%s", gambatte::to_string(result).c_str());
		return 0;
	}
	emuVideo.initImage(0, gbResX, gbResY);
	if(!gbEmu.isCgb())
	{
		gameBuiltinPalette = findGbcTitlePal(gbEmu.romTitle().c_str());
		if(gameBuiltinPalette)
			logMsg("game %s has built-in palette", gbEmu.romTitle().c_str());
		applyGBPalette();
	}

	readCheatFile();
	applyCheats();

	logMsg("started emu");
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);
	gbEmu.setSaveDir(EmuSystem::savePath());
	auto result = gbEmu.load(fullGamePath(), optionReportAsGba ? gbEmu.GBA_CGB : 0);
	return loadGameCommon(result);
}

int EmuSystem::loadGameFromIO(Io &io, const char *origFilename)
{
	closeGame();
	setupGameName(origFilename);
	gbEmu.setSaveDir(EmuSystem::savePath());
	ImagineFile file(io);
	auto result = gbEmu.load(file, origFilename, optionReportAsGba ? gbEmu.GBA_CGB : 0);
	return loadGameCommon(result);
}

void EmuSystem::clearInputBuffers()
{
	gbcInput.bits = 0;
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	long outputRate = optionSoundRate;
	long inputRate = std::round(2097152. * 1.004605);
	if(optionAudioResampler >= ResamplerInfo::num())
		optionAudioResampler = std::min((int)ResamplerInfo::num(), 1);
	if(!resampler || optionAudioResampler != activeResampler || resampler->outRate() != outputRate)
	{
		logMsg("setting up resampler %d for input rate %ldHz", (int)optionAudioResampler, inputRate);
		delete resampler;
		resampler = ResamplerInfo::get(optionAudioResampler).create(inputRate, outputRate, 35112 + 2064);
		activeResampler = optionAudioResampler;
	}
}

static void commitVideoFrame()
{
	updateAndDrawEmuVideo();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	uint8 snd[(35112+2064)*4] ATTRS(aligned(4));
	size_t samples = 35112;
	int frameSample = gbEmu.runFor(processGfx ? screenBuff : nullptr, 160, (uint_least32_t*)snd, samples,
		renderGfx ? commitVideoFrame : nullptr);
	if(renderAudio)
	{
		if(frameSample == -1)
		{
			logMsg("no emulated frame with %d samples", (int)samples);
		}
		//else logMsg("emulated frame at %d with %d samples", frameSample, samples);
		if(unlikely(samples < 34000))
		{
			uint repeatPos = std::max((int)samples-1, 0);
			uint32 *sndFrame = (uint32*)snd;
			logMsg("only %d, repeat %d", (int)samples, (int)sndFrame[repeatPos]);
			for(uint i = samples; i < 35112; i++)
			{
				sndFrame[i] = sndFrame[repeatPos];
			}
			samples = 35112;
		}
		// video rendered in runFor()
		short destBuff[(Audio::maxRate()/59)*2];
		uint destFrames = resampler->resample(destBuff, (const short*)snd, samples);
		assert(Audio::pcmFormat.framesToBytes(destFrames) <= sizeof(destBuff));
		EmuSystem::writeSound(destBuff, destFrames);
	}
}

namespace Base
{

CallResult onInit(int argc, char** argv)
{
	emuVideo.initPixmap((char*)screenBuff, pixFmt, gbResX, gbResY);

	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((0./255.) * .4, (77./255.) * .4, (74./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitCommon(argc, argv, navViewGrad);
	return OK;
}

}
