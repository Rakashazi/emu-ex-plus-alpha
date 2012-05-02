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

#ifdef CONFIG_BASE_USES_SHARED_DOCUMENTS_DIR
	#define CONFIG_FILE_NAME "PceEmu.config"
#else
	#define CONFIG_FILE_NAME "config"
#endif

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

static const GfxLGradientStopDesc navViewGrad[] =
{
	{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	{ .03, VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
	{ .3, VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
	{ .97, VertexColorPixelFormat.build((85./255.) * .4, (35./255.) * .4, (10./255.) * .4, 1.) },
	{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
};

static const char *touchConfigFaceBtnName = "I/II", *touchConfigCenterBtnName = "Select/Run";
static const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.sourceforge.net";
const uint EmuSystem::maxPlayers = 5;
static const uint systemFaceBtns = 6, systemCenterBtns = 2;;
static const bool systemHasTriggerBtns = 0, systemHasRevBtnLayout = 0;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#define systemAspectRatioString "4:3"
#include "CommonGui.hh"

namespace EmuControls
{

KeyCategory category[categories] =
{
		EMU_CONTROLS_IN_GAME_ACTIONS_CATEGORY_INIT,
		KeyCategory("Gamepad Controls", gamepadName, gameActionKeys),
};

}

BasicByteOption optionArcadeCard(CFGKEY_ARCADE_CARD, 1);
FsSys::cPath sysCardPath = "";
static PathOption<CFGKEY_SYSCARD_PATH> optionSysCardPath;

void EmuSystem::initOptions()
{
	optionSysCardPath.init(sysCardPath, sizeof(sysCardPath), "");
}

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

#include "PceFS.hh"
#include "PceOptionView.hh"
static PceOptionView oCategoryMenu;

#include "PceMenuView.hh"
static PceMenuView mMenu;

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_ARCADE_CARD: optionArcadeCard.readFromIO(io, readSize);
		bcase CFGKEY_SYSCARD_PATH: optionSysCardPath.readFromIO(io, readSize);
		logMsg("syscard path %s", sysCardPath);
		bcase CFGKEY_PCEKEY_UP: readKeyConfig2(io, pceKeyIdxUp, readSize);
		bcase CFGKEY_PCEKEY_RIGHT: readKeyConfig2(io, pceKeyIdxRight, readSize);
		bcase CFGKEY_PCEKEY_DOWN: readKeyConfig2(io, pceKeyIdxDown, readSize);
		bcase CFGKEY_PCEKEY_LEFT: readKeyConfig2(io, pceKeyIdxLeft, readSize);
		bcase CFGKEY_PCEKEY_LEFT_UP: readKeyConfig2(io, pceKeyIdxLeftUp, readSize);
		bcase CFGKEY_PCEKEY_RIGHT_UP: readKeyConfig2(io, pceKeyIdxRightUp, readSize);
		bcase CFGKEY_PCEKEY_RIGHT_DOWN: readKeyConfig2(io, pceKeyIdxRightDown, readSize);
		bcase CFGKEY_PCEKEY_LEFT_DOWN: readKeyConfig2(io, pceKeyIdxLeftDown, readSize);
		bcase CFGKEY_PCEKEY_SELECT: readKeyConfig2(io, pceKeyIdxSelect, readSize);
		bcase CFGKEY_PCEKEY_RUN: readKeyConfig2(io, pceKeyIdxRun, readSize);
		bcase CFGKEY_PCEKEY_I: readKeyConfig2(io, pceKeyIdxI, readSize);
		bcase CFGKEY_PCEKEY_II: readKeyConfig2(io, pceKeyIdxII, readSize);
		bcase CFGKEY_PCEKEY_I_TURBO: readKeyConfig2(io, pceKeyIdxITurbo, readSize);
		bcase CFGKEY_PCEKEY_II_TURBO: readKeyConfig2(io, pceKeyIdxIITurbo, readSize);
		bcase CFGKEY_PCEKEY_III: readKeyConfig2(io, pceKeyIdxIII, readSize);
		bcase CFGKEY_PCEKEY_IV: readKeyConfig2(io, pceKeyIdxIV, readSize);
		bcase CFGKEY_PCEKEY_V: readKeyConfig2(io, pceKeyIdxV, readSize);
		bcase CFGKEY_PCEKEY_VI: readKeyConfig2(io, pceKeyIdxVI, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{
	if(!optionArcadeCard.isDefault())
	{
		io->writeVar((uint16)optionArcadeCard.ioSize());
		optionArcadeCard.writeToIO(io);
	}
	optionSysCardPath.writeToIO(io);

	writeKeyConfig2(io, pceKeyIdxUp, CFGKEY_PCEKEY_UP);
	writeKeyConfig2(io, pceKeyIdxRight, CFGKEY_PCEKEY_RIGHT);
	writeKeyConfig2(io, pceKeyIdxDown, CFGKEY_PCEKEY_DOWN);
	writeKeyConfig2(io, pceKeyIdxLeft, CFGKEY_PCEKEY_LEFT);
	writeKeyConfig2(io, pceKeyIdxLeftUp, CFGKEY_PCEKEY_LEFT_UP);
	writeKeyConfig2(io, pceKeyIdxRightUp, CFGKEY_PCEKEY_RIGHT_UP);
	writeKeyConfig2(io, pceKeyIdxRightDown, CFGKEY_PCEKEY_RIGHT_DOWN);
	writeKeyConfig2(io, pceKeyIdxLeftDown, CFGKEY_PCEKEY_LEFT_DOWN);
	writeKeyConfig2(io, pceKeyIdxSelect, CFGKEY_PCEKEY_SELECT);
	writeKeyConfig2(io, pceKeyIdxRun, CFGKEY_PCEKEY_RUN);
	writeKeyConfig2(io, pceKeyIdxI, CFGKEY_PCEKEY_I);
	writeKeyConfig2(io, pceKeyIdxII, CFGKEY_PCEKEY_II);
	writeKeyConfig2(io, pceKeyIdxITurbo, CFGKEY_PCEKEY_I_TURBO);
	writeKeyConfig2(io, pceKeyIdxIITurbo, CFGKEY_PCEKEY_II_TURBO);
	writeKeyConfig2(io, pceKeyIdxIII, CFGKEY_PCEKEY_III);
	writeKeyConfig2(io, pceKeyIdxIV, CFGKEY_PCEKEY_IV);
	writeKeyConfig2(io, pceKeyIdxV, CFGKEY_PCEKEY_V);
	writeKeyConfig2(io, pceKeyIdxVI, CFGKEY_PCEKEY_VI);
}

static EmulateSpecStruct espec;

static int cdLoaded = 0;

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
	pixBuff[vidBufferX*vidBufferY] __attribute__ ((aligned (8)));
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
static Pixmap vidPixFull;

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

static void setupDrawing(bool force)
{
	MDFN_Rect &rect = currRect;
	logMsg("set rect %d,%d %d,%d", rect.x, rect.y, rect.w, rect.h);
	vidPixFull.init((uchar*)pixBuff, pixFmt, rect.w, vidBufferY);
	emuView.vidPix.initSubPixmap(vidPixFull, rect.x, rect.y, rect.w, rect.h);
	emuView.vidImg.init(emuView.vidPix, 0, optionImgFilter);
	emuView.disp.setImg(&emuView.vidImg);
}

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

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *gamePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.%s.nc%c", gamePath, gameName, md5_context::asciistr(MDFNGameInfo->MD5, 0).c_str(), saveSlotChar(slot));
}

/*bool EmuSystem::stateExists(int slot)
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(slot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	return Fs::fileExists(statePath.c_str());
}*/

void EmuSystem::closeSystem()
{
	emuSys->CloseGame();
	if(cdLoaded)
	{
		CDIF_Close();
		cdLoaded = 0;
	}
}

static void writeCDMD5()
{
	CD_TOC toc;
	md5_context layout_md5;

	CDIF_ReadTOC(&toc);

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
static bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path, bool allowAutosaveState)
{
	closeGame();

	string_copy(gamePath, FsSys::workDir(), sizeof(gamePath));
	#ifdef CONFIG_BASE_IOS_SETUID
		fixFilePermissions(gamePath);
	#endif
	snprintf(fullGamePath, sizeof(fullGamePath), "%s/%s", FsSys::workDir(), path);
	logMsg("full game path: %s", fullGamePath);
	//GetFileBase(fullPath);
	string_copyUpToLastCharInstance(gameName, path, '.');
	logMsg("set game name: %s", gameName);

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
		if(!strlen(sysCardPath) || !Fs::fileExists(sysCardPath))
		{
			popup.printf(3, 1, "No System Card Set");
			goto FAIL;
		}
		MDFN_cdErrorStr[0] = 0;
		if(!CDIF_Open(fullGamePath))
		{
			if(MDFN_cdErrorStr[0])
				popup.printf(4, 1, "%s", MDFN_cdErrorStr);
			else
				popup.printf(3, 1, "Error loading CUE/TOC");
			goto FAIL;
		}
		cdLoaded = 1;
		writeCDMD5(); // calc MD5s for CDs or mednafen will leave the previous hucard's MD5 in memory
		if(!emuSys->LoadCD())
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
		setupDrawing();
	}

	{
		bool previousState = 0;
		if(allowAutosaveState && optionAutoSaveState)
		{
			std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, "ncq");
			logMsg("loading autosave-state %s", statePath.c_str());
			previousState = MDFNI_LoadState(statePath.c_str(), 0);
		}

		if(!previousState)
		{
			// run 1 frame so vce line count is computed
			logMsg("no previous state, running 1st frame");
			espec.skip = 1;
			espec.SoundBuf = 0;
			emuSys->Emulate(&espec);
		}
	}

	configAudioRate();
	return 1;

	FAIL:
	if(cdLoaded)
	{
		CDIF_Close();
		cdLoaded = 0;
	}
	strcpy(gameName, "");
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
			setupDrawing();
			if(optionImageZoom == optionImageZoomIntegerOnly)
				emuView.placeEmu();
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
static uint ptrInputToSysButton(int input)
{
	switch(input)
	{
		bcase SysVController::F_ELEM: return BIT(0);
		bcase SysVController::F_ELEM+1: return BIT(1);
		bcase SysVController::F_ELEM+2: return BIT(0) << 8;
		bcase SysVController::F_ELEM+3: return BIT(1) << 8;
		bcase SysVController::F_ELEM+4: return BIT(2) << 8;
		bcase SysVController::F_ELEM+5: return BIT(3) << 8;

		bcase SysVController::C_ELEM: return BIT(2);
		bcase SysVController::C_ELEM+1: return BIT(3);

		bcase SysVController::D_ELEM: return BIT(4) | BIT(7);
		bcase SysVController::D_ELEM+1: return BIT(4); // up
		bcase SysVController::D_ELEM+2: return BIT(4) | BIT(5);
		bcase SysVController::D_ELEM+3: return BIT(7); // left
		bcase SysVController::D_ELEM+5: return BIT(5); // right
		bcase SysVController::D_ELEM+6: return BIT(6) | BIT(7);
		bcase SysVController::D_ELEM+7: return BIT(6); // down
		bcase SysVController::D_ELEM+8: return BIT(6) | BIT(5);
		bdefault: bug_branch("%d", input); return 0;
	}
}

void EmuSystem::handleOnScreenInputAction(uint state, uint vCtrlKey)
{
	handleInputAction(pointerInputPlayer, state, ptrInputToSysButton(vCtrlKey));
}

#endif

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case pceKeyIdxUp: return BIT(4);
		case pceKeyIdxRight: return BIT(5);
		case pceKeyIdxDown: return BIT(6);
		case pceKeyIdxLeft: return BIT(7);
		case pceKeyIdxLeftUp: return BIT(7) | BIT(4);
		case pceKeyIdxRightUp: return BIT(5) | BIT(4);
		case pceKeyIdxRightDown: return BIT(5) | BIT(6);
		case pceKeyIdxLeftDown: return BIT(7) | BIT(6);
		case pceKeyIdxSelect: return BIT(2);
		case pceKeyIdxRun: return BIT(3);
		case pceKeyIdxITurbo: turbo = 1;
		case pceKeyIdxI: return BIT(0);
		case pceKeyIdxIITurbo: turbo = 1;
		case pceKeyIdxII: return BIT(1);
		case pceKeyIdxIII: return BIT(0) << 8;
		case pceKeyIdxIV: return BIT(1) << 8;
		case pceKeyIdxV: return BIT(2) << 8;
		case pceKeyIdxVI: return BIT(3) << 8;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint player, uint state, uint emuKey)
{
	if(state == INPUT_PUSHED)
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

int EmuSystem::loadState()
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(saveStateSlot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	if(Fs::fileExists(statePath.c_str()))
	{
		logMsg("loading state %s", statePath.c_str());
		if(!MDFNI_LoadState(statePath.c_str(), 0))
			return STATE_RESULT_IO_ERROR;
		else
			return STATE_RESULT_OK;
	}
	return STATE_RESULT_NO_FILE;
}

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
	mem_zero(espec);
	// espec.SoundRate is set in mainInitCommon()
	mainInitCommon();
	#ifndef CONFIG_BASE_PS3
	vController.gp.activeFaceBtns = 2;
	#endif
	initVideoFormat();

	emuSys->soundchan = 0;
	emuSys->soundrate = 0;
	emuSys->name = (uint8*)EmuSystem::gameName;
	emuSys->rotated = 0;

	mMenu.init(Config::envIsPS3);
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	Base::displayNeedsUpdate();
	return OK;
}

}

#undef thisModuleName
