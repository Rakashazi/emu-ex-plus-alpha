#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include "EmuConfig.hh"

extern "C"
{
	#include <yabause/yabause.h>
	#include <yabause/m68kcore.h>
	#include <yabause/peripheral.h>
	#include <yabause/sh2core.h>
	#include <yabause/sh2int.h>
	#include <yabause/vidsoft.h>
	#include <yabause/scsp.h>
	#include <yabause/cdbase.h>
	#include <yabause/cs0.h>
	#include <yabause/cs2.h>
}

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2012 the\nYabause Team\nyabause.org";
static PerPad_struct *pad[2];
// from sh2_dynarec.c
#define SH2CORE_DYNAREC 2

static bool isCDExtension(const char *name)
{
	return string_hasDotExtension(name, "cue") ||
			string_hasDotExtension(name, "iso") ||
			string_hasDotExtension(name, "bin");
}

static int ssFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isCDExtension(name);
}

static int ssBiosFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || string_hasDotExtension(name, "bin");
}

CLINK void DisplayMessage(const char* str) {}
CLINK int OSDInit(int coreid) { return 0; }
CLINK void OSDPushMessage(int msgtype, int ttl, const char * message, ...) {}
CLINK void OSDDisplayMessages(void) {}

// Sound

// EmuFramework is in charge of audio setup & parameters
static int SNDImagineInit() { logMsg("called sound core init"); return 0; }
static void SNDImagineDeInit() {}
static int SNDImagineReset() { return 0; }
static void SNDImagineMuteAudio() {}
static void SNDImagineUnMuteAudio() {}
static void SNDImagineSetVolume(int volume) {}

static int SNDImagineChangeVideoFormat(int vertfreq)
{
	return 0;
}

static void mergeSamplesToStereo(s32 srcL, s32 srcR, s16 *dst)
{
	// Left Channel
	if (srcL > 0x7FFF) *dst = 0x7FFF;
	else if (srcL < -0x8000) *dst = -0x8000;
	else *dst = srcL;
	dst++;
	// Right Channel
	if (srcR > 0x7FFF) *dst = 0x7FFF;
	else if (srcR < -0x8000) *dst = -0x8000;
	else *dst = srcR;
}

static void SNDImagineUpdateAudioNull(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 frames) { }

static void SNDImagineUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 frames)
{
	//logMsg("got %d audio frames to write", frames);
	s16 sample[frames*2];
	iterateTimes(frames, i)
	{
		mergeSamplesToStereo(leftchanbuffer[i], rightchanbuffer[i], &sample[i*2]);
	}
	EmuSystem::writeSound(sample, frames);
}

static u32 SNDImagineGetAudioSpace()
{
	return 1024; // always render all samples available
}

#define SNDCORE_IMAGINE 1
static SoundInterface_struct SNDImagine =
{
	SNDCORE_IMAGINE,
	"Imagine Engine Sound Interface",
	SNDImagineInit,
	SNDImagineDeInit,
	SNDImagineReset,
	SNDImagineChangeVideoFormat,
	SNDImagineUpdateAudio,
	SNDImagineGetAudioSpace,
	SNDImagineMuteAudio,
	SNDImagineUnMuteAudio,
	SNDImagineSetVolume
};

static FsSys::PathString bupPath{};
static char mpegPath[] = "";
static char cartPath[] = "";

SH2Interface_struct *SH2CoreList[] =
{
	#ifdef SH2_DYNAREC
	&SH2Dynarec,
	#endif
	&SH2Interpreter,
	//&SH2DebugInterpreter,
	nullptr
};

static const int defaultSH2CoreID =
#ifdef SH2_DYNAREC
SH2CORE_DYNAREC;
#else
SH2CORE_INTERPRETER;
#endif

PerInterface_struct *PERCoreList[] =
{
	&PERDummy,
	nullptr
};

CDInterface *CDCoreList[] =
{
	&DummyCD,
	&ISOCD,
	nullptr
};

SoundInterface_struct *SNDCoreList[] =
{
	//&SNDDummy,
	&SNDImagine,
	nullptr
};

VideoInterface_struct *VIDCoreList[] =
{
	//&VIDDummy,
	&VIDSoft,
	nullptr
};

#if !defined HAVE_Q68 && !defined HAVE_C68K
#warning No 68K cores compiled in
#endif

M68K_struct *M68KCoreList[] =
{
	//&M68KDummy,
	#ifdef HAVE_C68K
	&M68KC68K,
	#endif
	#ifdef HAVE_Q68
	&M68KQ68,
	#endif
	nullptr
};

// controls

enum
{
	ssKeyIdxUp = EmuControls::systemKeyMapStart,
	ssKeyIdxRight,
	ssKeyIdxDown,
	ssKeyIdxLeft,
	ssKeyIdxLeftUp,
	ssKeyIdxRightUp,
	ssKeyIdxRightDown,
	ssKeyIdxLeftDown,
	ssKeyIdxStart,
	ssKeyIdxA,
	ssKeyIdxB,
	ssKeyIdxC,
	ssKeyIdxX,
	ssKeyIdxY,
	ssKeyIdxZ,
	ssKeyIdxL,
	ssKeyIdxR,
	ssKeyIdxATurbo,
	ssKeyIdxBTurbo,
	ssKeyIdxCTurbo,
	ssKeyIdxXTurbo,
	ssKeyIdxYTurbo,
	ssKeyIdxZTurbo,
	ssKeyIdxLastGamepad = ssKeyIdxZTurbo
};

enum {
	CFGKEY_BIOS_PATH = 279, CFGKEY_SH2_CORE = 280
};

static bool OptionSH2CoreIsValid(uint8 val)
{
	for(const auto &coreI : SH2CoreList)
	{
		if(coreI->id == val)
		{
			logMsg("SH2 core option valid");
			return true;
		}
	}
	logMsg("SH2 core option not valid");
	return false;
}

FsSys::PathString biosPath{};
static PathOption optionBiosPath(CFGKEY_BIOS_PATH, biosPath, "");
static Byte1Option optionSH2Core(CFGKEY_SH2_CORE, defaultSH2CoreID, false, OptionSH2CoreIsValid);

static yabauseinit_struct yinit =
{
	PERCORE_DUMMY,
	defaultSH2CoreID,
	VIDCORE_SOFT,
	SNDCORE_IMAGINE,
	#if defined(HAVE_Q68)
	M68KCORE_Q68,
	#else
	M68KCORE_C68K,
	#endif
	CDCORE_ISO,
	CART_NONE,
	REGION_AUTODETECT,
	biosPath.data(),
	EmuSystem::fullGamePath(),
	bupPath.data(),
	mpegPath,
	cartPath,
	nullptr,
	VIDEOFORMATTYPE_NTSC
};

const char *EmuSystem::inputFaceBtnName = "A-Z";
const char *EmuSystem::inputCenterBtnName = "Start";
const uint EmuSystem::inputFaceBtns = 8;
const uint EmuSystem::inputCenterBtns = 1;
const bool EmuSystem::inputHasTriggerBtns = true;
const bool EmuSystem::inputHasRevBtnLayout = false;
const char *EmuSystem::configFilename = "SaturnEmu.config";
const uint EmuSystem::maxPlayers = 2;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
#include <emuframework/CommonGui.hh>

const char *EmuSystem::shortSystemName()
{
	return "SS";
}

const char *EmuSystem::systemName()
{
	return "Sega Saturn";
}


void EmuSystem::initOptions()
{
	optionNotificationIcon.initDefault(0);
	optionNotificationIcon.isConst = 1;
	optionAutoSaveState.initDefault(0);
	if(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS)
		optionSound.initDefault(0);
	optionSoundRate.initDefault(44100);
	optionSoundRate.isConst = 1;

	#ifndef CONFIG_BASE_ANDROID
	optionFrameSkip.initDefault(optionFrameSkipAuto); // auto-frameskip default due to highly variable CPU usage
	#endif
	optionTouchCtrlBtnSpace.initDefault(100);
	optionTouchCtrlBtnStagger.initDefault(3);
}

void EmuSystem::onOptionsLoaded()
{
	yinit.sh2coretype = optionSH2Core;
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_BIOS_PATH: optionBiosPath.readFromIO(io, readSize);
		bcase CFGKEY_SH2_CORE: optionSH2Core.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionBiosPath.writeToIO(io);
	optionSH2Core.writeWithKeyIfNotDefault(io);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = ssFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = ssFsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGBA8888;

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerOffset = player ? EmuControls::gamepadKeys : 0;
	map[SysVController::F_ELEM] = ssKeyIdxA + playerOffset;
	map[SysVController::F_ELEM+1] = ssKeyIdxB + playerOffset;
	map[SysVController::F_ELEM+2] = ssKeyIdxC + playerOffset;
	map[SysVController::F_ELEM+3] = ssKeyIdxX + playerOffset;
	map[SysVController::F_ELEM+4] = ssKeyIdxY + playerOffset;
	map[SysVController::F_ELEM+5] = ssKeyIdxZ + playerOffset;
	map[SysVController::F_ELEM+6] = ssKeyIdxL + playerOffset;
	map[SysVController::F_ELEM+7] = ssKeyIdxR + playerOffset;

	map[SysVController::C_ELEM] = ssKeyIdxStart + playerOffset;

	map[SysVController::D_ELEM] = ssKeyIdxLeftUp + playerOffset;
	map[SysVController::D_ELEM+1] = ssKeyIdxUp + playerOffset;
	map[SysVController::D_ELEM+2] = ssKeyIdxRightUp + playerOffset;
	map[SysVController::D_ELEM+3] = ssKeyIdxLeft + playerOffset;
	map[SysVController::D_ELEM+5] = ssKeyIdxRight + playerOffset;
	map[SysVController::D_ELEM+6] = ssKeyIdxLeftDown + playerOffset;
	map[SysVController::D_ELEM+7] = ssKeyIdxDown + playerOffset;
	map[SysVController::D_ELEM+8] = ssKeyIdxRightDown + playerOffset;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case ssKeyIdxXTurbo:
		case ssKeyIdxYTurbo:
		case ssKeyIdxZTurbo:
		case ssKeyIdxATurbo:
		case ssKeyIdxBTurbo:
		case ssKeyIdxCTurbo:
		case ssKeyIdxXTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxYTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxZTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxATurbo + EmuControls::gamepadKeys:
		case ssKeyIdxBTurbo + EmuControls::gamepadKeys:
		case ssKeyIdxCTurbo + EmuControls::gamepadKeys:
			turbo = 1;
		default: return input;
	}
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = 0;
	if(emuKey > ssKeyIdxLastGamepad)
	{
		player = 1;
		emuKey -= EmuControls::gamepadKeys;
	}
	PerPad_struct *p = (player == 1) ? pad[1] : pad[0];
	bool pushed = state == Input::PUSHED;
	switch(emuKey)
	{
		bcase ssKeyIdxUp: if(pushed) PerPadUpPressed(p); else PerPadUpReleased(p);
		bcase ssKeyIdxRight: if(pushed) PerPadRightPressed(p); else PerPadRightReleased(p);
		bcase ssKeyIdxDown: if(pushed) PerPadDownPressed(p); else PerPadDownReleased(p);
		bcase ssKeyIdxLeft: if(pushed) PerPadLeftPressed(p); else PerPadLeftReleased(p);
		bcase ssKeyIdxLeftUp: if(pushed) { PerPadLeftPressed(p); PerPadUpPressed(p); }
			else { PerPadLeftReleased(p); PerPadUpReleased(p); }
		bcase ssKeyIdxRightUp: if(pushed) { PerPadRightPressed(p); PerPadUpPressed(p); }
			else { PerPadRightReleased(p); PerPadUpReleased(p); }
		bcase ssKeyIdxRightDown: if(pushed) { PerPadRightPressed(p); PerPadDownPressed(p); }
			else { PerPadRightReleased(p); PerPadDownReleased(p); }
		bcase ssKeyIdxLeftDown: if(pushed) { PerPadLeftPressed(p); PerPadDownPressed(p); }
			else { PerPadLeftReleased(p); PerPadDownReleased(p); }
		bcase ssKeyIdxStart: if(pushed) PerPadStartPressed(p); else PerPadStartReleased(p);
		bcase ssKeyIdxXTurbo:
		case ssKeyIdxX: if(pushed) PerPadXPressed(p); else PerPadXReleased(p);
		bcase ssKeyIdxYTurbo:
		case ssKeyIdxY: if(pushed) PerPadYPressed(p); else PerPadYReleased(p);
		bcase ssKeyIdxZTurbo:
		case ssKeyIdxZ: if(pushed) PerPadZPressed(p); else PerPadZReleased(p);
		bcase ssKeyIdxATurbo:
		case ssKeyIdxA: if(pushed) PerPadAPressed(p); else PerPadAReleased(p);
		bcase ssKeyIdxBTurbo:
		case ssKeyIdxB: if(pushed) PerPadBPressed(p); else PerPadBReleased(p);
		bcase ssKeyIdxCTurbo:
		case ssKeyIdxC: if(pushed) PerPadCPressed(p); else PerPadCReleased(p);
		bcase ssKeyIdxL: if(pushed) PerPadLTriggerPressed(p); else PerPadLTriggerReleased(p);
		bcase ssKeyIdxR: if(pushed) PerPadRTriggerPressed(p); else PerPadRTriggerReleased(p);
		bdefault: bug_branch("%d", emuKey);
	}
}

static int ssResX = 320, ssResY = 224;

static bool renderToScreen = 0;

CLINK void YuiSwapBuffers()
{
	//logMsg("YuiSwapBuffers");
	if(likely(renderToScreen))
	{
		int height, width;
		VIDCore->GetGlSize(&width, &height);
		if(unlikely(ssResX != width || ssResY != height))
		{
			logMsg("resolution changed to %d,%d", width, height);
			ssResX = width;
			ssResY = height;
			emuVideo.resizeImage(ssResX, ssResY);
		}

		updateAndDrawEmuVideo();
		renderToScreen = 0;
	}
	else
	{
		//logMsg("skipping render");
	}
}

CLINK void YuiSetVideoAttribute(int type, int val) { }
CLINK int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) { return 0; }

CLINK void YuiErrorMsg(const char *string)
{
	logMsg("%s", string);
}

CLINK int OSDUseBuffer()
{
	return 0;
}

CLINK int OSDChangeCore(int coreid)
{
	return 0;
}

void EmuSystem::resetGame()
{
	logMsg("resetting system");
	assert(gameIsRunning());
	//Cs2ChangeCDCore(CDCORE_DUMMY, yinit.cdpath);
	YabauseReset();
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FsSys::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return makeFSPathStringPrintf("%s/%s.0%c.yss", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(YabSaveState(saveStr.data()) == 0)
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FsSys::fileExists(saveStr.data()))
	{
		logMsg("loading state %s", saveStr.data());
		if(YabLoadState(saveStr.data()) == 0)
			return STATE_RESULT_OK;
		else
			return STATE_RESULT_IO_ERROR;
	}
	return STATE_RESULT_NO_FILE;
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		T123Save(BupRam, 0x10000, 1, bupPath.data());
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		if(YabSaveState(saveStr.data()) != 0)
			logMsg("error saving state %s", saveStr.data());
	}
}

static bool yabauseIsInit = 0;

void EmuSystem::closeSystem()
{
	if(yabauseIsInit)
	{
		YabauseDeInit();
		yabauseIsInit = 0;
	}
}

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);

	string_printf(bupPath, "%s/bkram.bin", savePath());
	if(YabauseInit(&yinit) != 0)
	{
		logErr("YabauseInit failed");
		popup.postError("Error loading game");
		return 0;
	}
	logMsg("YabauseInit done");
	yabauseIsInit = 1;
	emuVideo.initPixmap((char*)dispbuffer, pixFmt, ssResX, ssResY);
	emuVideo.initImage(0, ssResX, ssResY);

	PerPortReset();
	pad[0] = PerPadAdd(&PORTDATA1);
	pad[1] = PerPadAdd(&PORTDATA2);
	ScspSetFrameAccurate(1);

	logMsg("finished loading game");
	return 1;
}

int EmuSystem::loadGameFromIO(IO &io, const char *origFilename)
{
	return 0; // TODO
}

void EmuSystem::clearInputBuffers()
{
	PerPortReset();
	pad[0] = PerPadAdd(&PORTDATA1);
	pad[1] = PerPadAdd(&PORTDATA2);
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(renderGfx)
		renderToScreen = 1;
	SNDImagine.UpdateAudio = renderAudio ? SNDImagineUpdateAudio : SNDImagineUpdateAudioNull;
	YabauseEmulate();
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

namespace Base
{

CallResult onInit(int argc, char** argv)
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.2 * .4, 0., 0., 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitCommon(argc, argv, navViewGrad);
	return OK;
}

}
