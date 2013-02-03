/*  This file is part of PCE.emu.

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

#define thisModuleName "main"
#include "MDFNWrapper.hh"
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

#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

namespace PCE_Fast
{
	void HuCDumpSave(void);
	void applyVideoFormat(EmulateSpecStruct *espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern bool AVPad6Enabled[5];
	extern vce_t vce;
}

static bool isHuCardExtension(const char *name)
{
	return string_hasDotExtension(name, "pce") || string_hasDotExtension(name, "sgx") || string_hasDotExtension(name, "zip");
}

static bool isCDExtension(const char *name)
{
	return string_hasDotExtension(name, "toc") || string_hasDotExtension(name, "cue");
}

static int pceHuCDFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isHuCardExtension(name) || isCDExtension(name);
}

static int pceHuFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isHuCardExtension(name);
}

Byte1Option optionArcadeCard(CFGKEY_ARCADE_CARD, 1);
FsSys::cPath sysCardPath = "";
static PathOption optionSysCardPath(CFGKEY_SYSCARD_PATH, sysCardPath, sizeof(sysCardPath), "");

const uint EmuSystem::maxPlayers = 5;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include "CommonGui.hh"

void EmuSystem::initOptions() { }

extern char MDFN_cdErrorStr[256];

enum
{
	pceKeyIdxUp = EmuControls::systemKeyMapStart,
	pceKeyIdxRight,
	pceKeyIdxDown,
	pceKeyIdxLeft,
	pceKeyIdxLeftUp,
	pceKeyIdxRightUp,
	pceKeyIdxRightDown,
	pceKeyIdxLeftDown,
	pceKeyIdxSelect,
	pceKeyIdxRun,
	pceKeyIdxI,
	pceKeyIdxII,
	pceKeyIdxITurbo,
	pceKeyIdxIITurbo,
	pceKeyIdxIII,
	pceKeyIdxIV,
	pceKeyIdxV,
	pceKeyIdxVI
};

#if defined(CONFIG_BASE_PS3)
const char *ps3_productCode = "PCEE00000";
#endif

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_ARCADE_CARD: optionArcadeCard.readFromIO(io, readSize);
		bcase CFGKEY_SYSCARD_PATH: optionSysCardPath.readFromIO(io, readSize);
		logMsg("syscard path %s", sysCardPath);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	optionArcadeCard.writeWithKeyIfNotDefault(io);
	optionSysCardPath.writeToIO(io);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = pceHuCDFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = pceHuFsFilter;

static EmulateSpecStruct espec;
static std::vector<CDIF *> CDInterfaces;

#ifndef CONFIG_BASE_PS3
	#define USE_PIX_RGB565
#endif
static const uint vidBufferX = 512, vidBufferY = 242;
static
#ifdef USE_PIX_RGB565
	uint16
#else
	uint32
#endif
	pixBuff[vidBufferX*vidBufferY] __attribute__ ((aligned (8))) {0};
static MDFN_Rect lineWidth[vidBufferY];
static MDFN_Rect currRect;
static const MDFN_PixelFormat mPixFmtRGB565 = { 16, MDFN_COLORSPACE_RGB, { 11 }, { 5 }, { 0 }, 16, 5, 6, 5, 8 };
static const MDFN_PixelFormat mPixFmtRGBA8888 = { 32, MDFN_COLORSPACE_RGB, { 0 }, { 8 }, { 16 }, 24, 8, 8, 8, 8 };
static const MDFN_PixelFormat mPixFmtBGRA8888 = { 32, MDFN_COLORSPACE_RGB, { 16 }, { 8 }, { 0 }, 24, 8, 8, 8, 8 };
#ifdef USE_PIX_RGB565
	//static const MDFN_PixelFormat mPixFmt(16, MDFN_COLORSPACE_RGB, 11, 5, 0, 16);
	static const MDFN_PixelFormat &mPixFmt = mPixFmtRGB565;
	static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
#else
	#ifdef CONFIG_BASE_PS3
		static const MDFN_PixelFormat &mPixFmt = mPixFmtBGRA8888;
		static const PixelFormatDesc *pixFmt = &PixelFormatARGB8888;
	#else
		// Default to RGBA Ordering
		static const MDFN_PixelFormat &mPixFmt = mPixFmtRGBA8888;
		static const PixelFormatDesc *pixFmt = &PixelFormatRGBA8888;
	#endif
#endif
static MDFN_Surface mSurface;

static void initVideoFormat()
{
	mSurface.Init(pixBuff, vidBufferX, vidBufferY, vidBufferX, mPixFmt);
	#if !defined(USE_PIX_RGB565) && !defined(CONFIG_BASE_PS3)
	if(gfx_preferBGRA)
	{
		pixFmt = &PixelFormatBGRA8888;
		mSurface.format = mPixFmtBGRA8888;
	}
	#endif
}

static uint16 inputBuff[5] = { 0 }; // 5 gamepad buffers

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, "ncq");
		logMsg("saving autosave-state %s", statePath.c_str());
		#ifdef CONFIG_BASE_IOS_SETUID
			fixFilePermissions(statePath.c_str());
		#endif
		MDFNI_SaveState(statePath.c_str(), 0, 0, 0, 0);
	}
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		// TODO: fix iOS permissions if needed
		PCE_Fast::HuCDumpSave();
	}
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'q';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *statePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.%s.nc%c", statePath, gameName, md5_context::asciistr(MDFNGameInfo->MD5, 0).c_str(), saveSlotChar(slot));
}

void EmuSystem::closeSystem()
{
	emuSys->CloseGame();
	if(CDInterfaces.size())
	{
		assert(CDInterfaces.size() == 1);
		delete CDInterfaces[0];
		CDInterfaces.clear();
	}
}

static void writeCDMD5()
{
	CD_TOC toc;
	md5_context layout_md5;

	CDInterfaces[0]->ReadTOC(&toc);

	layout_md5.starts();

	layout_md5.update_u32_as_lsb(toc.first_track);
	layout_md5.update_u32_as_lsb(toc.last_track);
	layout_md5.update_u32_as_lsb(toc.tracks[100].lba);

	for(uint32 track = toc.first_track; track <= toc.last_track; track++)
	{
		layout_md5.update_u32_as_lsb(toc.tracks[track].lba);
		layout_md5.update_u32_as_lsb(toc.tracks[track].control & 0x4);
	}

	uint8 LayoutMD5[16];
	layout_md5.finish(LayoutMD5);

	memcpy(emuSys->MD5, LayoutMD5, 16);
}

bool EmuSystem::vidSysIsPAL() { return 0; }
bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);

	if(isHuCardExtension(path))
	{
		MDFNFILE fp;
		static const FileExtensionSpecStruct ext[] =
		{
			{ ".pce", 0 },
			{ ".sgx", 0 },
			{ 0, 0 }
		};
		if(!fp.Open(fullGamePath, ext, 0))
		{
			popup.printf(3, 1, "Error opening file");
			goto FAIL;
		}
		if(emuSys->Load(0, &fp) != 1)
		{
			popup.printf(3, 1, "Error loading ROM");
			goto FAIL;
		}
	}
	else if(isCDExtension(path))
	{
		if(!strlen(sysCardPath) || !FsSys::fileExists(sysCardPath))
		{
			popup.printf(3, 1, "No System Card Set");
			goto FAIL;
		}
		try
		{
			CDInterfaces.push_back(new CDIF(fullGamePath));
		}
		catch(std::exception &e)
		{
			popup.printf(4, 1, "%s", e.what());
			goto FAIL;
		}
		writeCDMD5(); // calc MD5s for CDs or mednafen will leave the previous hucard's MD5 in memory
		if(!emuSys->LoadCD(&CDInterfaces))
		{
			popup.printf(3, 1, "Error loading CD");
			goto FAIL;
		}
	}
	else
		goto FAIL;

	//logMsg("%d input ports", MDFNGameInfo->InputInfo->InputPorts);
	iterateTimes(5, i)
	{
		emuSys->SetInput(i, "gamepad", &inputBuff[i]);
	}

	if(unlikely(!espec.surface))
	{
		logMsg("doing initial audio/video setup for emulator");
		espec.LineWidths = lineWidth;
		espec.NeedRewind = 0;

		espec.surface = &mSurface;

		espec.SoundBuf = 0;
		espec.SoundBufMaxSize = 0;
		espec.skip = 0;

		PCE_Fast::applyVideoFormat(&espec);

		currRect = (MDFN_Rect){0, 4, 256, 232};//espec.DisplayRect;
		emuView.initImage(0, currRect.x, currRect.y, currRect.w, currRect.h, currRect.w, vidBufferY);
	}

	{
		// run 1 frame so vce line count is computed
		//logMsg("no previous state, running 1st frame");
		espec.skip = 1;
		espec.SoundBuf = 0;
		emuSys->Emulate(&espec);
	}

	configAudioRate();
	return 1;

	FAIL:
	if(CDInterfaces.size())
	{
		assert(CDInterfaces.size() == 1);
		delete CDInterfaces[0];
		CDInterfaces.clear();
	}
	return 0;
}

void EmuSystem::clearInputBuffers()
{
	mem_zero(inputBuff);
}

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	espec.SoundRate = (float)optionSoundRate * (vce.lc263 ? 0.99702 : 1.001);
	#ifdef CONFIG_ENV_WEBOS
		if(optionFrameSkip != optionFrameSkipAuto)
			espec.SoundRate *= 42660./44100.; // better sync with Pre's refresh rate
	#elif defined(CONFIG_BASE_PS3)
		espec.SoundRate *= 1.0011;
	#endif
	logMsg("emu sound rate %d, 262 lines %d, fskip %d", (int)espec.SoundRate, !vce.lc263, (int)optionFrameSkip);
	PCE_Fast::applySoundFormat(&espec);
}

static const uint audioMaxFramesPerUpdate = (Audio::maxRate/59)*2;

static bool renderToScreen = 0;
void MDFND_commitVideoFrame()
{
	if(likely(renderToScreen))
	{
		if(unlikely(espec.DisplayRect.x != currRect.x || espec.DisplayRect.y != currRect.y ||
						espec.DisplayRect.w != currRect.w || espec.DisplayRect.h != currRect.h))
		{
			currRect = espec.DisplayRect;
			emuView.resizeImage(currRect.x, currRect.y, currRect.w, currRect.h, currRect.w, vidBufferY);
		}

		emuView.updateAndDrawContent();
		renderToScreen = 0;
	}
	else
	{
		//logMsg("skipping render");
	}
}

#ifdef USE_NEW_AUDIO
static Audio::BufferContext *aBuff = 0;
#else
static int16 audioBuff[(Audio::maxRate/59)*2];
#endif


static void setupEmuAudio(bool render)
{
	#ifdef USE_NEW_AUDIO
	if(render && (aBuff = Audio::getPlayBuffer(audioMaxFramesPerUpdate)))
	{
		espec.SoundBuf = (int16*)aBuff->data;
		espec.SoundBufMaxSize = aBuff->frames-1;
	}
	else
	{
		espec.SoundBuf = 0;
	}
	#else
	espec.SoundBuf = render ? audioBuff : 0;
	espec.SoundBufMaxSize = EmuSystem::pcmFormat.bytesToFrames(sizeof(audioBuff));
	#endif
}

static void commitEmuAudio(bool render)
{
	//logMsg("writing %d frames", espec.SoundBufSize);
	if(render)
	{
	#ifdef USE_NEW_AUDIO
		if(aBuff)
		{
			assert((uint)espec.SoundBufSize <= aBuff->frames);
			Audio::commitPlayBuffer(aBuff, espec.SoundBufSize);
		}
	#else
		assert((uint)espec.SoundBufSize <= EmuSystem::pcmFormat.bytesToFrames(sizeof(audioBuff)));
		Audio::writePcm((uchar*)audioBuff, espec.SoundBufSize);
	#endif
	}
}

#ifdef INPUT_SUPPORTS_POINTER

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerMask = player << 12;
	map[SysVController::F_ELEM] = BIT(0) | playerMask;
	map[SysVController::F_ELEM+1] = BIT(1) | playerMask;
	map[SysVController::F_ELEM+2] = BIT(8) | playerMask;
	map[SysVController::F_ELEM+3] = BIT(9) | playerMask;
	map[SysVController::F_ELEM+4] = BIT(10) | playerMask;
	map[SysVController::F_ELEM+5] = BIT(11) | playerMask;

	map[SysVController::C_ELEM] = BIT(2) | playerMask;
	map[SysVController::C_ELEM+1] = BIT(3) | playerMask;

	map[SysVController::D_ELEM] = BIT(4) | BIT(7) | playerMask;
	map[SysVController::D_ELEM+1] = BIT(4) | playerMask;
	map[SysVController::D_ELEM+2] = BIT(4) | BIT(5) | playerMask;
	map[SysVController::D_ELEM+3] = BIT(7) | playerMask;
	map[SysVController::D_ELEM+5] = BIT(5) | playerMask;
	map[SysVController::D_ELEM+6] = BIT(6) | BIT(7) | playerMask;
	map[SysVController::D_ELEM+7] = BIT(6) | playerMask;
	map[SysVController::D_ELEM+8] = BIT(6) | BIT(5) | playerMask;
}

#endif

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	assert(input >= pceKeyIdxUp);
	uint player = (input - pceKeyIdxUp) / EmuControls::gamepadKeys;
	uint playerMask = player << 12;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case pceKeyIdxUp: return BIT(4) | playerMask;
		case pceKeyIdxRight: return BIT(5) | playerMask;
		case pceKeyIdxDown: return BIT(6) | playerMask;
		case pceKeyIdxLeft: return BIT(7) | playerMask;
		case pceKeyIdxLeftUp: return BIT(7) | BIT(4) | playerMask;
		case pceKeyIdxRightUp: return BIT(5) | BIT(4) | playerMask;
		case pceKeyIdxRightDown: return BIT(5) | BIT(6) | playerMask;
		case pceKeyIdxLeftDown: return BIT(7) | BIT(6) | playerMask;
		case pceKeyIdxSelect: return BIT(2) | playerMask;
		case pceKeyIdxRun: return BIT(3) | playerMask;
		case pceKeyIdxITurbo: turbo = 1;
		case pceKeyIdxI: return BIT(0) | playerMask;
		case pceKeyIdxIITurbo: turbo = 1;
		case pceKeyIdxII: return BIT(1) | playerMask;
		case pceKeyIdxIII: return BIT(8) | playerMask;
		case pceKeyIdxIV: return BIT(9) | playerMask;
		case pceKeyIdxV: return BIT(10) | playerMask;
		case pceKeyIdxVI: return BIT(11) | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 12;
	assert(player < maxPlayers);
	if(state == Input::PUSHED)
		setBits(inputBuff[player], emuKey);
	else
		unsetBits(inputBuff[player], emuKey);
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	setupEmuAudio(renderAudio);
	if(renderGfx)
		renderToScreen = 1;
	espec.skip = processGfx ? 0 : 1;
	//logMsg("render audio %d, %p", renderAudio, espec.SoundBuf );
	emuSys->Emulate(&espec);
	static bool first = 1;
	if(first)
	{
		logMsg("got %d audio size", espec.SoundBufSize);
		first = 0;
	}
	commitEmuAudio(renderAudio);
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	PCE_Fast::PCE_Power();
}

int EmuSystem::saveState()
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(saveStateSlot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	logMsg("saving state %s", statePath.c_str());
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(statePath.c_str());
	#endif
	if(!MDFNI_SaveState(statePath.c_str(), 0, 0, 0, 0))
		return STATE_RESULT_IO_ERROR;
	else
		return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(saveStateSlot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	if(FsSys::fileExists(statePath.c_str()))
	{
		logMsg("loading state %s", statePath.c_str());
		if(!MDFNI_LoadState(statePath.c_str(), 0))
			return STATE_RESULT_IO_ERROR;
		else
			return STATE_RESULT_OK;
	}
	return STATE_RESULT_NO_FILE;
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
	mem_zero(espec);
	// espec.SoundRate is set in mainInitCommon()
	mainInitCommon();
	#ifndef CONFIG_BASE_PS3
	vController.gp.activeFaceBtns = 2;
	#endif
	initVideoFormat();
	emuView.initPixmap((uchar*)pixBuff, pixFmt, vidBufferX, vidBufferY);

	emuSys->soundchan = 0;
	emuSys->soundrate = 0;
	emuSys->name = (uint8*)EmuSystem::gameName;
	emuSys->rotated = 0;
	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((85./255.) * .4, (35./255.) * .4, (10./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
