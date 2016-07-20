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

#define LOGTAG "main"
#include "MDFN.hh"
#include <emuframework/EmuApp.hh>
#include "../../../EmuFramework/include/emuframework/EmuAppInlines.hh"
#include "EmuConfig.hh"
#include "internal.hh"
#include <imagine/util/ScopeGuard.hh>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/huc.h>
#include <mednafen/pce_fast/vdc.h>

using namespace IG;

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.sourceforge.net";

bool hasHuCardExtension(const char *name)
{
	return string_hasDotExtension(name, "pce") || string_hasDotExtension(name, "sgx");
}

static bool hasCDExtension(const char *name)
{
	return string_hasDotExtension(name, "toc") || string_hasDotExtension(name, "cue") || string_hasDotExtension(name, "ccd");
}

static bool hasPCEWithCDExtension(const char *name)
{
	return hasHuCardExtension(name) || hasCDExtension(name);
}

Byte1Option optionArcadeCard{CFGKEY_ARCADE_CARD, 1};
FS::PathString sysCardPath{};
static PathOption optionSysCardPath{CFGKEY_SYSCARD_PATH, sysCardPath, ""};

const char *EmuSystem::inputFaceBtnName = "I/II";
const char *EmuSystem::inputCenterBtnName = "Select/Run";
const uint EmuSystem::inputFaceBtns = 6;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const char *EmuSystem::configFilename = "PceEmu.config";
const uint EmuSystem::maxPlayers = 5;
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);

const char *EmuSystem::shortSystemName()
{
	return "PCE-TG16";
}

const char *EmuSystem::systemName()
{
	return "PC Engine (TurboGrafx-16)";
}

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = 2;
	#endif
}

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

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_ARCADE_CARD: optionArcadeCard.readFromIO(io, readSize);
		bcase CFGKEY_SYSCARD_PATH: optionSysCardPath.readFromIO(io, readSize);
		logMsg("syscard path %s", sysCardPath.data());
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionArcadeCard.writeWithKeyIfNotDefault(io);
	optionSysCardPath.writeToIO(io);
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasPCEWithCDExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasHuCardExtension;

static std::vector<CDIF *> CDInterfaces;

static const MDFN_PixelFormat mPixFmtRGB565 { MDFN_COLORSPACE_RGB, 11, 5, 0, 16 };
static const MDFN_PixelFormat mPixFmtRGBA8888 { MDFN_COLORSPACE_RGB, 0, 8, 16, 24 };
static const MDFN_PixelFormat mPixFmtBGRA8888 { MDFN_COLORSPACE_RGB, 16, 8, 0, 24 };
#ifdef MDFN_PIXELFORMAT_SINGLE_BPP
	#if MDFN_PIXELFORMAT_SINGLE_BPP == 16
	using Pixel = uint16;
	static const MDFN_PixelFormat &mPixFmt = mPixFmtRGB565;
	static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
	#else
	using Pixel = uint32;
	static const MDFN_PixelFormat &mPixFmt = mPixFmtRGBA8888;
	static constexpr auto pixFmt = IG::PIXEL_FMT_RGBA8888;
	#endif
#else
using Pixel = uint32;
static const MDFN_PixelFormat &mPixFmt = mPixFmtRGBA8888;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGBA8888;
#endif

static const uint vidBufferX = 512, vidBufferY = 242;
static const uint vidBufferXMax = 1024; // width when scaling multi-res content
static Pixel pixBuff[vidBufferX*vidBufferY] __attribute__ ((aligned (8))) {0};
static Pixel pixBuffScaled[vidBufferXMax*vidBufferY] __attribute__ ((aligned (8))) {0};
static MDFN_Surface mSurface{pixBuff, vidBufferX, vidBufferY, vidBufferX, mPixFmt};
static uint16 inputBuff[5] {0}; // 5 gamepad buffers
static bool usingMultires = false;

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, "ncq");
		logMsg("saving autosave-state %s", statePath.c_str());
		fixFilePermissions(statePath.c_str());
		MDFNI_SaveState(statePath.c_str(), 0, 0, 0, 0);
	}
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory");
		// TODO: fix iOS permissions if needed
		PCE_Fast::HuC_DumpSave();
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.%s.nc%c", statePath, gameName, md5_context::asciistr(MDFNGameInfo->MD5, 0).c_str(), saveSlotChar(slot));
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
uint EmuSystem::multiresVideoBaseX() { return 512; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	auto unloadCD = IG::scopeGuard(
		[]()
		{
			if(CDInterfaces.size())
			{
				assert(CDInterfaces.size() == 1);
				delete CDInterfaces[0];
				CDInterfaces.clear();
			}
		});
	if(hasCDExtension(path))
	{
		if(!strlen(sysCardPath.data()) || !FS::exists(sysCardPath))
		{
			popup.printf(3, 1, "No System Card Set");
			return 0;
		}
		CDInterfaces.reserve(1);
		FS::current_path(gamePath());
		try
		{
			CDInterfaces.push_back(CDIF_Open(fullGamePath(), false, false));
			writeCDMD5();
			emuSys->LoadCD(&CDInterfaces);
		}
		catch(std::exception &e)
		{
			popup.printf(4, 1, "%s", e.what());
			return 0;
		}
	}
	else
	{
		try
		{
			MDFNFILE fp(io, origFilename);
			emuSys->Load(&fp);
		}
		catch(std::exception &e)
		{
			popup.printf(3, 1, "%s", e.what());
			return 0;
		}
	}

	//logMsg("%d input ports", MDFNGameInfo->InputInfo->InputPorts);
	iterateTimes(5, i)
	{
		emuSys->SetInput(i, "gamepad", &inputBuff[i]);
	}

	if(unlikely(!emuVideo.vidImg))
	{
		logMsg("doing initial video setup for emulator");
		PCE_Fast::applyVideoFormat(mSurface);
		emuVideo.initImage(0, 0, 0, 256, 224, 256, 224);
	}

	{
		// run 1 frame so vce line count is computed
		//logMsg("no previous state, running 1st frame");
		EmulateSpecStruct espec;
		espec.skip = 1;
		espec.surface = &mSurface;
		MDFN_SubSurface dummyLineWidth[vidBufferY];
		espec.subSurface = dummyLineWidth;
		emuSys->Emulate(&espec);
	}

	configAudioPlayback();
	unloadCD.cancel();
	return 1;
}

void EmuSystem::clearInputBuffers()
{
	IG::fillData(inputBuff);
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	EmulateSpecStruct espec;
	double systemFrameRate = vce.lc263 ? 59.826 : 60.0545;
	espec.SoundRate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	logMsg("emu sound rate:%f, 263 lines:%d", (double)espec.SoundRate, vce.lc263);
	PCE_Fast::applySoundFormat(&espec);
}

static bool updateEmuPixmap(uint width, uint height, char *pixBuff)
{
	if(unlikely(width != emuVideo.vidPix.w() || height != emuVideo.vidPix.h()
		|| pixBuff != emuVideo.vidPix.pixel({})))
	{
		//logMsg("display rect %d:%d:%d:%d", displayRect.x, displayRect.y, displayRect.w, displayRect.h);
		emuVideo.initPixmap(pixBuff, pixFmt, width, height);
		emuVideo.resizeImage(0, 0, width, height, width, height);
		return true;
	}
	return false;
}

void MDFND_commitVideoFrame(const MDFN_FrameInfo &info)
{
	bool isMultires = info.subSurfaces > 1;
	int pixWidth = info.displayRect.w;

	if(isMultires)
	{
		// get the final width
		iterateTimes(info.subSurfaces, i)
		{
			auto &s = info.subSurface[i];
			if(s.w == 341)
				pixWidth = 1024;
			else
				pixWidth = std::max(pixWidth, s.w);
		}
		if(updateEmuPixmap(pixWidth, info.displayRect.h, (char*)pixBuffScaled))
		{
			iterateTimes(info.subSurfaces, i)
			{
				auto &s = info.subSurface[i];
				logMsg("sub-surface %d: %d:%d", i, s.w, s.h);
			}
		}
		// blit the pixels
		auto srcPixAddr = &pixBuff[0];
		auto destPixAddr = &pixBuffScaled[0];
		int prevY = 0;
		if(pixWidth == 1024)
		{
			// scale 256x4, 341x3 + 1x4, 512x2
			iterateTimes(info.subSurfaces, i)
			{
				auto &s = info.subSurface[i];
				switch(s.w)
				{
					bdefault:
						bug_branch("%d", s.w);
					bcase 256:
					{
						iterateTimes(s.h, h)
						{
							iterateTimes(256, w)
							{
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr++;
							}
						}
					}
					bcase 341:
					{
						iterateTimes(s.h, h)
						{
							iterateTimes(340, w)
							{
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr++;
							}
							*destPixAddr++ = *srcPixAddr;
							*destPixAddr++ = *srcPixAddr;
							*destPixAddr++ = *srcPixAddr;
							*destPixAddr++ = *srcPixAddr++;
						}
					}
					bcase 512:
					{
						iterateTimes(s.h, h)
						{
							iterateTimes(512, w)
							{
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr++;
							}
						}
					}
				}
			}
		}
		else
		{
			assert(pixWidth == 512);
			iterateTimes(info.subSurfaces, i)
			{
				auto &s = info.subSurface[i];
				switch(s.w)
				{
					bdefault:
						bug_branch("%d", s.w);
					bcase 256:
					{
						iterateTimes(s.h, h)
						{
							iterateTimes(256, w)
							{
								*destPixAddr++ = *srcPixAddr;
								*destPixAddr++ = *srcPixAddr++;
							}
						}
					}
					bcase 512:
					{
						uint pixelsToCopy = s.h * 512;
						memcpy(destPixAddr, srcPixAddr, pixelsToCopy * sizeof(Pixel));
						destPixAddr += pixelsToCopy;
						srcPixAddr += pixelsToCopy;
					}
				}
			}
		}
	}
	else
	{
		updateEmuPixmap(pixWidth, info.displayRect.h, (char*)pixBuff);
	}

	updateAndDrawEmuVideo();
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerMask = player << 12;
	map[SysVController::F_ELEM] = bit(0) | playerMask;
	map[SysVController::F_ELEM+1] = bit(1) | playerMask;
	map[SysVController::F_ELEM+2] = bit(8) | playerMask;
	map[SysVController::F_ELEM+3] = bit(9) | playerMask;
	map[SysVController::F_ELEM+4] = bit(10) | playerMask;
	map[SysVController::F_ELEM+5] = bit(11) | playerMask;

	map[SysVController::C_ELEM] = bit(2) | playerMask;
	map[SysVController::C_ELEM+1] = bit(3) | playerMask;

	map[SysVController::D_ELEM] = bit(4) | bit(7) | playerMask;
	map[SysVController::D_ELEM+1] = bit(4) | playerMask;
	map[SysVController::D_ELEM+2] = bit(4) | bit(5) | playerMask;
	map[SysVController::D_ELEM+3] = bit(7) | playerMask;
	map[SysVController::D_ELEM+5] = bit(5) | playerMask;
	map[SysVController::D_ELEM+6] = bit(6) | bit(7) | playerMask;
	map[SysVController::D_ELEM+7] = bit(6) | playerMask;
	map[SysVController::D_ELEM+8] = bit(6) | bit(5) | playerMask;
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	assert(input >= pceKeyIdxUp);
	uint player = (input - pceKeyIdxUp) / EmuControls::gamepadKeys;
	uint playerMask = player << 12;
	input -= EmuControls::gamepadKeys * player;
	switch(input)
	{
		case pceKeyIdxUp: return bit(4) | playerMask;
		case pceKeyIdxRight: return bit(5) | playerMask;
		case pceKeyIdxDown: return bit(6) | playerMask;
		case pceKeyIdxLeft: return bit(7) | playerMask;
		case pceKeyIdxLeftUp: return bit(7) | bit(4) | playerMask;
		case pceKeyIdxRightUp: return bit(5) | bit(4) | playerMask;
		case pceKeyIdxRightDown: return bit(5) | bit(6) | playerMask;
		case pceKeyIdxLeftDown: return bit(7) | bit(6) | playerMask;
		case pceKeyIdxSelect: return bit(2) | playerMask;
		case pceKeyIdxRun: return bit(3) | playerMask;
		case pceKeyIdxITurbo: turbo = 1; // fall through to pceKeyIdxI
		case pceKeyIdxI: return bit(0) | playerMask;
		case pceKeyIdxIITurbo: turbo = 1; // fall through to pceKeyIdxII
		case pceKeyIdxII: return bit(1) | playerMask;
		case pceKeyIdxIII: return bit(8) | playerMask;
		case pceKeyIdxIV: return bit(9) | playerMask;
		case pceKeyIdxV: return bit(10) | playerMask;
		case pceKeyIdxVI: return bit(11) | playerMask;
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	uint player = emuKey >> 12;
	assert(player < maxPlayers);
	inputBuff[player] = IG::setOrClearBits(inputBuff[player], (uint16)emuKey, state == Input::PUSHED);
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	uint maxFrames = Audio::maxRate()/54;
	int16 audioBuff[maxFrames*2];
	EmulateSpecStruct espec;
	if(renderAudio)
	{
		espec.SoundBuf = audioBuff;
		espec.SoundBufMaxSize = maxFrames;
	}
	espec.commitVideo = renderGfx;
	espec.skip = !processGfx;
	espec.surface = &mSurface;
	MDFN_SubSurface subSurface[vidBufferY];
	espec.subSurface = subSurface;
	emuSys->Emulate(&espec);
	if(renderAudio)
	{
		assert((uint)espec.SoundBufSize <= EmuSystem::pcmFormat.bytesToFrames(sizeof(audioBuff)));
		writeSound((uchar*)audioBuff, espec.SoundBufSize);
	}
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	PCE_Fast::PCE_Power();
}

std::error_code EmuSystem::saveState()
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(saveStateSlot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	logMsg("saving state %s", statePath.c_str());
	fixFilePermissions(statePath.c_str());
	if(!MDFNI_SaveState(statePath.c_str(), 0, 0, 0, 0))
		return {EIO, std::system_category()};
	else
		return {};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	char ext[] = { "nc0" };
	ext[2] = saveSlotChar(saveStateSlot);
	std::string statePath = MDFN_MakeFName(MDFNMKF_STATE, 0, ext);
	if(FS::exists(statePath.c_str()))
	{
		logMsg("loading state %s", statePath.c_str());
		if(!MDFNI_LoadState(statePath.c_str(), 0))
			return {{EIO, std::system_category()}};
		else
			return {{}};
	}
	return {{ENOENT, std::system_category()}};
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return true; }

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((255./255.) * .4, (104./255.) * .4, (31./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((85./255.) * .4, (35./255.) * .4, (10./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

CallResult EmuSystem::onInit()
{
	emuVideo.initPixmap((char*)pixBuff, pixFmt, vidBufferX, vidBufferY);
	emuSys->name = (uint8*)EmuSystem::gameName;
	return OK;
}
