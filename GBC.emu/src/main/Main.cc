#define thisModuleName "main"
#include <logger/interface.h>
#include <util/area2.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>
#include <gambatte.h>
#include <resample/resamplerinfo.h>
#include <main/Cheats.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2011\nthe Gambatte Team\ngambatte.sourceforge.net";
gambatte::GB gbEmu;
static float audioFramesPerUpdateScaler;
static Resampler *resampler = nullptr;
static uint8 activeResampler = 1;

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
	CFGKEY_FULL_GBC_SATURATION = 272, CFGKEY_AUDIO_RESAMPLER = 273
};

struct GBPalette
{
	uint bg[4], sp1[4], sp2[4];
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

static void applyGBPalette(uint idx)
{
	assert(idx < sizeofArray(gbPal));
	GBPalette &pal = gbPal[idx];
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(0, i, pal.bg[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(1, i, pal.sp1[i]);
	iterateTimes(4, i)
		gbEmu.setDmgPaletteColor(2, i, pal.sp2[i]);
}

static Byte1Option optionGBPal
		(CFGKEY_GB_PAL_IDX, 0, 0, optionIsValidWithMax<sizeofArray(gbPal)-1>);
static Byte1Option optionReportAsGba(CFGKEY_REPORT_AS_GBA, 0);
static Byte1Option optionAudioResampler(CFGKEY_AUDIO_RESAMPLER, 1);

namespace gambatte
{
extern bool useFullColorSaturation;
}

static Option<OptionMethodRef<bool, gambatte::useFullColorSaturation>, uint8> optionFullGbcSaturation(CFGKEY_FULL_GBC_SATURATION, 0);

const uint EmuSystem::maxPlayers = 1;
uint EmuSystem::aspectRatioX = 10, EmuSystem::aspectRatioY = 9;
#include "CommonGui.hh"

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_GB_PAL_IDX: optionGBPal.readFromIO(io, readSize);
		bcase CFGKEY_REPORT_AS_GBA: optionReportAsGba.readFromIO(io, readSize);
		bcase CFGKEY_FULL_GBC_SATURATION: optionFullGbcSaturation.readFromIO(io, readSize);
		bcase CFGKEY_AUDIO_RESAMPLER: optionAudioResampler.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionGBPal.writeWithKeyIfNotDefault(io);
	optionReportAsGba.writeWithKeyIfNotDefault(io);
	optionFullGbcSaturation.writeWithKeyIfNotDefault(io);
	optionAudioResampler.writeWithKeyIfNotDefault(io);
}

void EmuSystem::initOptions()
{

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

static class GbcInput : public gambatte::InputGetter
{
public:
#ifndef __clang_major__
	constexpr GbcInput() {}
#endif
	unsigned bits = 0;
	unsigned operator()() { return bits; }
} gbcInput;

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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.gqs", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(Config::envIsIOSJB)
		fixFilePermissions(saveStr);
	logMsg("saving state %s", saveStr);
	if(!gbEmu.saveState(/*screenBuff*/0, 160, saveStr))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(FsSys::fileExists(saveStr))
	{
		logMsg("loading state %s", saveStr);
		if(!gbEmu.loadState(saveStr))
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
	gbEmu.setSaveDir(savePath());
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		logMsg("saving auto-state");
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		if(Config::envIsIOSJB)
			fixFilePermissions(saveStr);
		gbEmu.saveState(/*screenBuff*/0, 160, saveStr);
	}
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
	cheatList.removeAll();
	cheatsModified = 0;
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path)
{
	emuView.initImage(0, gbResX, gbResY);
	closeGame();
	setupGamePaths(path);
	auto result = gbEmu.load(fullGamePath, optionReportAsGba ? gbEmu.GBA_CGB : 0);
	if(result != gambatte::LOADRES_OK)
	{
		popup.printf(3, 1, "%s", gambatte::to_string(result).c_str());
		return 0;
	}

	readCheatFile();
	applyCheats();

	logMsg("started emu");
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	gbcInput.bits = 0;
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	#ifdef CONFIG_BASE_IOS
	long outputRate = (float)optionSoundRate*.99555;
	#elif defined(CONFIG_BASE_ANDROID)
	long outputRate = (uint)optionFrameSkip == optionFrameSkipAuto ? (float)optionSoundRate*.99555 : (float)optionSoundRate*.9954;
	#elif defined(CONFIG_ENV_WEBOS)
	long outputRate = (uint)optionFrameSkip == optionFrameSkipAuto ? (float)optionSoundRate*.99555 : (float)optionSoundRate*.963;
	#else
	long outputRate = float(optionSoundRate)*.99555;
	#endif
	audioFramesPerUpdateScaler = outputRate/2097152.;
	if(optionAudioResampler >= ResamplerInfo::num())
		optionAudioResampler = std::min((int)ResamplerInfo::num(), 1);
	if(!resampler || optionAudioResampler != activeResampler || resampler->outRate() != outputRate)
	{
		logMsg("setting up resampler %d for output rate %ldHz", (int)optionAudioResampler, outputRate);
		delete resampler;
		resampler = ResamplerInfo::get(optionAudioResampler).create(2097152, outputRate, 35112 + 2064);
		activeResampler = optionAudioResampler;
	}
}

static void writeAudio(const int16 *srcBuff, unsigned srcFrames)
{
	#ifdef USE_NEW_AUDIO
	Audio::BufferContext *aBuff = Audio::getPlayBuffer(Audio::maxRate/58);
	if(!aBuff)
		return;
	short *destBuff = (short*)aBuff->data;
	assert(Audio::maxRate/58 >= aBuff->frames);
	#else
	short destBuff[(Audio::maxRate/58)*2];
	#endif
	uint destFrames = resampler->resample(destBuff, (const short*)srcBuff, srcFrames);
	//logMsg("%d audio frames from %d, %d", destFrames, srcFrames, (int)destBuff[0]);
	#ifdef USE_NEW_AUDIO
	Audio::commitPlayBuffer(aBuff, destFrames);
	#else
	assert(Audio::maxFormat.framesToBytes(destFrames) <= sizeof(destBuff));
	//mem_zero(destBuff);
	Audio::writePcm((uchar*)destBuff, destFrames);
	#endif
}

static void commitVideoFrame()
{
	emuView.updateAndDrawContent();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	uint8 snd[(35112+2064)*4] ATTRS(aligned(4));
	unsigned samples;

	samples = 35112;
	int frameSample = gbEmu.runFor(processGfx ? screenBuff : nullptr, 160, (uint_least32_t*)snd, samples,
		renderGfx ? commitVideoFrame : nullptr);
	if(renderAudio)
	{
		if(frameSample == -1)
		{
			logMsg("no emulated frame with %d samples", samples);
		}
		//else logMsg("emulated frame at %d with %d samples", frameSample, samples);
		if(unlikely(samples < 34000))
		{
			uint repeatPos = std::max((int)samples-1, 0);
			uint32 *sndFrame = (uint32*)snd;
			logMsg("only %d, repeat %d", samples, (int)sndFrame[repeatPos]);
			for(uint i = samples; i < 35112; i++)
			{
				sndFrame[i] = sndFrame[repeatPos];
			}
			samples = 35112;
		}
		// video rendered in runFor()
		writeAudio((const int16*)snd, samples);
	}
}

namespace Input
{
void onInputEvent(const Input::Event &e)
{
	handleInputEvent(e);
}
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit(int argc, char** argv)
{
	mainInitCommon();
	emuView.initPixmap((uchar*)screenBuff, pixFmt, gbResX, gbResY);
	applyGBPalette(optionGBPal);
	gbEmu.setInputGetter(&gbcInput);
	gbEmu.setSaveDir(EmuSystem::savePath());
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((8./255.) * .4, (232./255.) * .4, (222./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((0./255.) * .4, (77./255.) * .4, (74./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
