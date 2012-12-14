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
	ssKeyIdxZTurbo
};

enum {
	CFGKEY_SS_UP = 256, CFGKEY_SS_RIGHT = 257,
	CFGKEY_SS_DOWN = 258, CFGKEY_SS_LEFT = 259,
	CFGKEY_SS_LEFT_UP = 260, CFGKEY_SS_RIGHT_UP = 261,
	CFGKEY_SS_RIGHT_DOWN = 262, CFGKEY_SS_LEFT_DOWN = 263,
	CFGKEY_SS_START = 264,
	CFGKEY_SS_A = 265, CFGKEY_SS_B = 266,
	CFGKEY_SS_C = 267, CFGKEY_SS_X = 268,
	CFGKEY_SS_Y = 269, CFGKEY_SS_Z = 270,
	CFGKEY_SS_L = 271, CFGKEY_SS_R = 272,
	CFGKEY_SS_A_TURBO = 273, CFGKEY_SS_B_TURBO = 274,
	CFGKEY_SS_C_TURBO = 275, CFGKEY_SS_X_TURBO = 276,
	CFGKEY_SS_Y_TURBO = 277, CFGKEY_SS_Z_TURBO = 278,

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

namespace EmuControls
{
KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};
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

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_SS_UP: readKeyConfig2(io, ssKeyIdxUp, readSize);
		bcase CFGKEY_SS_RIGHT: readKeyConfig2(io, ssKeyIdxRight, readSize);
		bcase CFGKEY_SS_DOWN: readKeyConfig2(io, ssKeyIdxDown, readSize);
		bcase CFGKEY_SS_LEFT: readKeyConfig2(io, ssKeyIdxLeft, readSize);
		bcase CFGKEY_SS_LEFT_UP: readKeyConfig2(io, ssKeyIdxLeftUp, readSize);
		bcase CFGKEY_SS_RIGHT_UP: readKeyConfig2(io, ssKeyIdxRightUp, readSize);
		bcase CFGKEY_SS_RIGHT_DOWN: readKeyConfig2(io, ssKeyIdxRightDown, readSize);
		bcase CFGKEY_SS_LEFT_DOWN: readKeyConfig2(io, ssKeyIdxLeftDown, readSize);
		bcase CFGKEY_SS_START: readKeyConfig2(io, ssKeyIdxStart, readSize);
		bcase CFGKEY_SS_A: readKeyConfig2(io, ssKeyIdxA, readSize);
		bcase CFGKEY_SS_B: readKeyConfig2(io, ssKeyIdxB, readSize);
		bcase CFGKEY_SS_C: readKeyConfig2(io, ssKeyIdxC, readSize);
		bcase CFGKEY_SS_X: readKeyConfig2(io, ssKeyIdxX, readSize);
		bcase CFGKEY_SS_Y: readKeyConfig2(io, ssKeyIdxY, readSize);
		bcase CFGKEY_SS_Z: readKeyConfig2(io, ssKeyIdxZ, readSize);
		bcase CFGKEY_SS_L: readKeyConfig2(io, ssKeyIdxL, readSize);
		bcase CFGKEY_SS_R: readKeyConfig2(io, ssKeyIdxR, readSize);
		bcase CFGKEY_SS_A_TURBO: readKeyConfig2(io, ssKeyIdxATurbo, readSize);
		bcase CFGKEY_SS_B_TURBO: readKeyConfig2(io, ssKeyIdxBTurbo, readSize);
		bcase CFGKEY_SS_C_TURBO: readKeyConfig2(io, ssKeyIdxCTurbo, readSize);
		bcase CFGKEY_SS_X_TURBO: readKeyConfig2(io, ssKeyIdxXTurbo, readSize);
		bcase CFGKEY_SS_Y_TURBO: readKeyConfig2(io, ssKeyIdxYTurbo, readSize);
		bcase CFGKEY_SS_Z_TURBO: readKeyConfig2(io, ssKeyIdxZTurbo, readSize);
		bcase CFGKEY_BIOS_PATH: optionBiosPath.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionBiosPath.writeToIO(io);

	writeKeyConfig2(io, ssKeyIdxUp, CFGKEY_SS_UP);
	writeKeyConfig2(io, ssKeyIdxRight, CFGKEY_SS_RIGHT);
	writeKeyConfig2(io, ssKeyIdxDown, CFGKEY_SS_DOWN);
	writeKeyConfig2(io, ssKeyIdxLeft, CFGKEY_SS_LEFT);
	writeKeyConfig2(io, ssKeyIdxLeftUp, CFGKEY_SS_LEFT_UP);
	writeKeyConfig2(io, ssKeyIdxRightUp, CFGKEY_SS_RIGHT_UP);
	writeKeyConfig2(io, ssKeyIdxRightDown, CFGKEY_SS_RIGHT_DOWN);
	writeKeyConfig2(io, ssKeyIdxLeftDown, CFGKEY_SS_LEFT_DOWN);
	writeKeyConfig2(io, ssKeyIdxStart, CFGKEY_SS_START);
	writeKeyConfig2(io, ssKeyIdxA, CFGKEY_SS_A);
	writeKeyConfig2(io, ssKeyIdxB, CFGKEY_SS_B);
	writeKeyConfig2(io, ssKeyIdxC, CFGKEY_SS_C);
	writeKeyConfig2(io, ssKeyIdxX, CFGKEY_SS_X);
	writeKeyConfig2(io, ssKeyIdxY, CFGKEY_SS_Y);
	writeKeyConfig2(io, ssKeyIdxZ, CFGKEY_SS_Z);
	writeKeyConfig2(io, ssKeyIdxL, CFGKEY_SS_L);
	writeKeyConfig2(io, ssKeyIdxR, CFGKEY_SS_R);
	writeKeyConfig2(io, ssKeyIdxATurbo, CFGKEY_SS_A_TURBO);
	writeKeyConfig2(io, ssKeyIdxBTurbo, CFGKEY_SS_B_TURBO);
	writeKeyConfig2(io, ssKeyIdxCTurbo, CFGKEY_SS_C_TURBO);
	writeKeyConfig2(io, ssKeyIdxXTurbo, CFGKEY_SS_X_TURBO);
	writeKeyConfig2(io, ssKeyIdxYTurbo, CFGKEY_SS_Y_TURBO);
	writeKeyConfig2(io, ssKeyIdxZTurbo, CFGKEY_SS_Z_TURBO);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = ssFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = ssFsFilter;

static const PixelFormatDesc *pixFmt = &PixelFormatRGBA8888;

static uint ptrInputToSysButton(int input)
{
	switch(input)
	{
		case SysVController::F_ELEM: return ssKeyIdxA;
		case SysVController::F_ELEM+1: return ssKeyIdxB;
		case SysVController::F_ELEM+2: return ssKeyIdxC;
		case SysVController::F_ELEM+3: return ssKeyIdxX;
		case SysVController::F_ELEM+4: return ssKeyIdxY;
		case SysVController::F_ELEM+5: return ssKeyIdxZ;
		case SysVController::F_ELEM+6: return ssKeyIdxL;
		case SysVController::F_ELEM+7: return ssKeyIdxR;

		case SysVController::C_ELEM: return ssKeyIdxStart;

		case SysVController::D_ELEM: return ssKeyIdxLeftUp;
		case SysVController::D_ELEM+1: return ssKeyIdxUp;
		case SysVController::D_ELEM+2: return ssKeyIdxRightUp;
		case SysVController::D_ELEM+3: return ssKeyIdxLeft;
		case SysVController::D_ELEM+5: return ssKeyIdxRight;
		case SysVController::D_ELEM+6: return ssKeyIdxLeftDown;
		case SysVController::D_ELEM+7: return ssKeyIdxDown;
		case SysVController::D_ELEM+8: return ssKeyIdxRightDown;
		default: bug_branch("%d", input); return 0;
	}
}

void EmuSystem::handleOnScreenInputAction(uint state, uint vCtrlKey)
{
	handleInputAction(pointerInputPlayer, state, ptrInputToSysButton(vCtrlKey));
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
			turbo = 1;
		default: return input;
	}
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	PerPad_struct *p = (player == 1) ? pad[1] : pad[0];
	bool pushed = state == INPUT_PUSHED;
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
void onInputEvent(const InputEvent &e)
{
	handleInputEvent(e);
}
}

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit()
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
