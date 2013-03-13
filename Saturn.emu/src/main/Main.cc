#define thisModuleName "main"
#include <resource2/image/png/ResourceImagePng.h>
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

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2013\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2012 the\nYabause Team\nyabause.org";
static PerPad_struct *pad[2];

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

CLINK void DisplayMessage(const char* str) { }
CLINK int OSDInit(int coreid) { return 0; }
CLINK void OSDPushMessage(int msgtype, int ttl, const char * message, ...) { }
CLINK void OSDDisplayMessages(void) { }

// Sound

// EmuFramework is in charge of audio setup & parameters
static int SNDImagineInit() { logMsg("called sound core init"); return 0; }
static void SNDImagineDeInit() { }
static int SNDImagineReset() { return 0; }
static void SNDImagineMuteAudio() { }
static void SNDImagineUnMuteAudio() { }
static void SNDImagineSetVolume(int volume) { }

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
	frames = IG::min(800U, frames);
	s16 sample[800*2];
	iterateTimes(frames, i)
	{
		mergeSamplesToStereo(leftchanbuffer[i], rightchanbuffer[i], &sample[i*2]);
	}
	Audio::writePcm((uchar*)sample, frames);
}

static u32 SNDImagineGetAudioSpace()
{
	return 1000; // always render all samples available
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

static FsSys::cPath bupPath = "";
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
	CFGKEY_BIOS_PATH = 279
};

FsSys::cPath biosPath = "";
static PathOption optionBiosPath(CFGKEY_BIOS_PATH, biosPath, sizeof(biosPath), "");

static const int SH2CORE_DYNAREC = 2;
static yabauseinit_struct yinit =
{
	PERCORE_DUMMY,
	#ifdef SH2_DYNAREC
	SH2CORE_DYNAREC,
	#else
	SH2CORE_INTERPRETER,
	#endif
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
	biosPath,
	EmuSystem::fullGamePath,
	bupPath,
	mpegPath,
	cartPath,
	nullptr,
	VIDEOFORMATTYPE_NTSC
};


const uint EmuSystem::maxPlayers = 2;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include <CommonGui.hh>

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

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_BIOS_PATH: optionBiosPath.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionBiosPath.writeToIO(io);
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
			emuView.resizeImage(ssResX, ssResY);
		}

		emuView.updateAndDrawContent();
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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.yss", statePath, gameName, saveSlotChar(slot));
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(saveStr);
	#endif
	if(YabSaveState(saveStr) == 0)
		return STATE_RESULT_OK;
	else
		return STATE_RESULT_IO_ERROR;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	if(FsSys::fileExists(saveStr))
	{
		logMsg("loading state %s", saveStr);
		if(YabLoadState(saveStr) == 0)
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
		T123Save(BupRam, 0x10000, 1, bupPath);
	}
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(saveStr);
		#endif
		if(YabSaveState(saveStr) != 0)
			logMsg("error saving state %s", saveStr);
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
bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);

	snprintf(bupPath, sizeof(bupPath), "%s/bkram.bin", savePath());
	if(YabauseInit(&yinit) != 0)
	{
		logErr("YabauseInit failed");
		popup.postError("Error loading game");
		return 0;
	}
	logMsg("YabauseInit done");
	yabauseIsInit = 1;
	emuView.initPixmap((uchar*)dispbuffer, pixFmt, ssResX, ssResY);
	emuView.initImage(0, ssResX, ssResY);

	PerPortReset();
	pad[0] = PerPadAdd(&PORTDATA1);
	pad[1] = PerPadAdd(&PORTDATA2);

	//EmuSystem::configAudioRate();
	logMsg("finished loading game");
	return 1;
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
	ScspSetFrameAccurate(1);
	mainInitCommon();
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .3, VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .97, VertexColorPixelFormat.build(.2 * .4, 0., 0., 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
