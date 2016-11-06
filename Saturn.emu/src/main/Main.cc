
#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"

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

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2012-2014\nRobert Broglia\nwww.explusalpha.com\n\n(c) 2012 the\nYabause Team\nyabause.org";
bool EmuSystem::handlesGenericIO = false;
PerPad_struct *pad[2];
// from sh2_dynarec.c
#define SH2CORE_DYNAREC 2

static bool hasCDExtension(const char *name)
{
	return string_hasDotExtension(name, "cue") ||
			string_hasDotExtension(name, "iso") ||
			string_hasDotExtension(name, "bin");
}

bool hasBIOSExtension(const char *name)
{
	return string_hasDotExtension(name, "bin");
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

static FS::PathString bupPath{};
static char mpegPath[] = "";
static char cartPath[] = "";

extern const int defaultSH2CoreID =
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

FS::PathString biosPath{};

yabauseinit_struct yinit
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

const char *EmuSystem::shortSystemName()
{
	return "SS";
}

const char *EmuSystem::systemName()
{
	return "Sega Saturn";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasCDExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasCDExtension;

static constexpr auto pixFmt = IG::PIXEL_FMT_RGBA8888;

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

void EmuSystem::reset(ResetMode mode)
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.yss", statePath, gameName, saveSlotChar(slot));
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	if(YabSaveState(saveStr.data()) == 0)
		return {};
	else
		return {EIO, std::system_category()};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	if(FS::exists(saveStr))
	{
		logMsg("loading state %s", saveStr.data());
		if(YabLoadState(saveStr.data()) == 0)
			return {{}};
		else
			return {{EIO, std::system_category()}};
	}
	return {{ENOENT, std::system_category()}};
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

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	return 0; // TODO
}

void EmuSystem::configAudioRate(double frameTime)
{
	// TODO: use frameTime
	pcmFormat.rate = optionSoundRate;
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	if(renderGfx)
		renderToScreen = 1;
	SNDImagine.UpdateAudio = renderAudio ? SNDImagineUpdateAudio : SNDImagineUpdateAudioNull;
	YabauseEmulate();
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(.8 * .4, 0., 0., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.2 * .4, 0., 0., 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

CallResult EmuSystem::onInit()
{
	return OK;
}

