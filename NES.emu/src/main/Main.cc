/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "internal.hh"
#include "EmuFileIO.hh"
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <fceu/driver.h>
#include <fceu/state.h>
#include <fceu/fceu.h>
#include <fceu/ppu.h>
#include <fceu/fds.h>
#include <fceu/input.h>
#include <fceu/cheat.h>
#include <fceu/video.h>
#include <fceu/sound.h>
#include <fceu/palette.h>
#include <fceu/x6502.h>

void ApplyDeemphasisComplete(pal* pal512);
void FCEU_setDefaultPalettePtr(pal *ptr);

static uint8 XBufData[256 * 256 + 16]{};
// Separate front & back buffers not needed for our video implementation
uint8 *XBuf = XBufData;
uint8 *XBackBuf = XBufData;
uint8 *XDBuf{};
uint8 *XDBackBuf{};
int dendy = 0;
bool paldeemphswap = false;
bool swapDuty = false;

namespace EmuEx
{

using PalArray = std::array<pal, 512>;

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nFCEUX Team\nfceux.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
IG::ApplicationContext appCtx{};
unsigned fceuCheats = 0;
ESI nesInputPortDev[2]{SI_UNSET, SI_UNSET};
unsigned autoDetectedRegion = 0;
static IG::PixelFormat pixFmt{};
static PalArray defaultPal{};
union
{
	uint16_t col16[256];
	uint32_t col32[256];
} nativeCol;
static const unsigned nesPixX = 256, nesPixY = 240, nesVisiblePixY = 224;

bool hasFDSBIOSExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".rom", ".bin", ".ROM", ".BIN");
}

static bool hasFDSExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".fds", ".FDS");
}

static bool hasROMExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".nes", ".unf", ".NES", ".UNF");
}

static bool hasNESExtension(std::string_view name)
{
	return hasROMExtension(name) || hasFDSExtension(name);
}

const BundledGameInfo &EmuSystem::bundledGameInfo(unsigned idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game.7z" }
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "FC-NES";
}

const char *EmuSystem::systemName()
{
	return "Famicom (Nintendo Entertainment System)";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasNESExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasNESExtension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
		FCEUI_PowerNES();
	else
		FCEUI_ResetNES();
}

static char saveSlotCharNES(int slot)
{
	switch(slot)
	{
		case -1: return 's';
		case 0 ... 9: return '0' + slot;
		default: bug_unreachable("slot == %d", slot); return 0;
	}
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.fc{}", name, saveSlotCharNES(slot));
}

void EmuSystem::saveState(IG::CStringView path)
{
	if(!FCEUI_SaveState(path))
		EmuSystem::throwFileWriteError();
}

void EmuSystem::loadState(IG::CStringView path)
{
	if(!FCEUI_LoadState(path))
		EmuSystem::throwFileReadError();
}

void EmuSystem::saveBackupMem(IG::ApplicationContext ctx)
{
	if(gameIsRunning())
	{
		logMsg("saving backup memory if needed");
		if(isFDS)
			FCEU_FDSWriteModifiedDisk();
		else
			GameInterface(GI_WRITESAVE);
		FCEU_FlushGameCheats(nullptr, 0, false);
	}
}

void EmuSystem::closeSystem(IG::ApplicationContext ctx)
{
	FCEUI_CloseGame();
	fceuCheats = 0;
}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	bug_unreachable("called FCEUD_GetPalette()");
}

static void setDefaultPalette(IO &io)
{
	auto bytesRead = io.read(defaultPal.data(), 512);
	if(bytesRead < 192)
	{
		logErr("skipped palette with only %d bytes", (int)bytesRead);
		return;
	}
	if(bytesRead != 512)
	{
		ApplyDeemphasisComplete(defaultPal.data());
	}
	FCEU_setDefaultPalettePtr(defaultPal.data());
}

void setDefaultPalette(IG::ApplicationContext ctx, IG::CStringView palPath)
{
	if(palPath.empty())
	{
		FCEU_setDefaultPalettePtr(nullptr);
		return;
	}
	logMsg("setting default palette with path:%s", palPath.data());
	if(palPath[0] != '/' && !IG::isUri(palPath))
	{
		// load as asset
		auto io = ctx.openAsset(FS::pathString("palette", palPath), IO::AccessHint::ALL);
		if(!io)
			return;
		setDefaultPalette(io);
	}
	else
	{
		auto io = ctx.openFileUri(palPath, IO::AccessHint::ALL, IO::OPEN_TEST);
		if(!io)
			return;
		setDefaultPalette(io);
	}
}

static void cacheUsingZapper()
{
	assert(GameInfo);
	iterateTimes(2, i)
	{
		if(joyports[i].type == SI_ZAPPER)
		{
			usingZapper = true;
			return;
		}
	}
	usingZapper = false;
}

static const char* fceuInputToStr(int input)
{
	switch(input)
	{
		case SI_UNSET: return "Unset";
		case SI_GAMEPAD: return "Gamepad";
		case SI_ZAPPER: return "Zapper";
		case SI_NONE: return "None";
		default: bug_unreachable("input == %d", input); return 0;
	}
}

void setupNESFourScore()
{
	if(!GameInfo)
		return;
	if(!usingZapper)
	{
		if(optionFourScore)
			logMsg("attaching four score");
		FCEUI_SetInputFourscore(optionFourScore);
	}
	else
		FCEUI_SetInputFourscore(0);
}

bool EmuSystem::vidSysIsPAL()
{
	return PAL || dendy;
}

void setupNESInputPorts()
{
	if(!GameInfo)
		return;
	iterateTimes(2, i)
	{
		if(nesInputPortDev[i] == SI_UNSET) // user didn't specify device, go with auto settings
			connectNESInput(i, GameInfo->input[i] == SI_UNSET ? SI_GAMEPAD : GameInfo->input[i]);
		else
			connectNESInput(i, nesInputPortDev[i]);
		logMsg("attached %s to port %d%s", fceuInputToStr(joyports[i].type), i, nesInputPortDev[i] == SI_UNSET ? " (auto)" : "");
	}
	cacheUsingZapper();
	setupNESFourScore();
}

static int cheatCallback(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	logMsg("cheat: %s, %d", name, s);
	fceuCheats++;
	return 1;
}

const char *regionToStr(int region)
{
	switch(region)
	{
		case 0: return "NTSC";
		case 1: return "PAL";
		case 2: return "Dendy";
	}
	return "Unknown";
}

static int regionFromName(std::string_view name)
{
	if(IG::stringContainsAny(name, "(E)", "(e)", "(EU)", "(Europe)", "(PAL)",
		"(F)", "(f)", "(G)", "(g)", "(I)", "(i)"))
	{
		return 1; // PAL
	}
	else if(IG::stringContainsAny(name, "(RU)", "(ru)"))
	{
		return 2; // Dendy
	}
	return 0; // NTSC
}

void setRegion(int region, int defaultRegion, int detectedRegion)
{
	if(region)
	{
		logMsg("Forced region:%s", regionToStr(region - 1));
		FCEUI_SetRegion(region - 1, false);
	}
	else if(defaultRegion)
	{
		logMsg("Forced region (Default):%s", regionToStr(defaultRegion - 1));
		FCEUI_SetRegion(defaultRegion - 1, false);
	}
	else
	{
		logMsg("Detected region:%s", regionToStr(detectedRegion));
		FCEUI_SetRegion(detectedRegion, false);
	}
}

void EmuSystem::loadGame(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	auto ioStream = new EmuFileIO(io);
	auto file = new FCEUFILE();
	file->filename = contentFileName();
	file->archiveIndex = -1;
	file->stream = ioStream;
	file->size = ioStream->size();
	if(!FCEUI_LoadGameWithFile(file, contentFileName().data(), 0))
	{
		throw std::runtime_error("Error loading game");
	}
	autoDetectedRegion = regionFromName(contentFileName());
	setRegion(optionVideoSystem.val, optionDefaultVideoSystem.val, autoDetectedRegion);
	FCEUI_ListCheats(cheatCallback, 0);
	if(fceuCheats)
		logMsg("%d total cheats", fceuCheats);

	setupNESInputPorts();
}

void EmuSystem::onPrepareAudio(EmuAudio &audio)
{
	audio.setStereo(false);
}

bool EmuSystem::onVideoRenderFormatChange(EmuVideo &video, IG::PixelFormat fmt)
{
	video.setFormat({{nesPixX, nesVisiblePixY}, fmt});
	pixFmt = fmt;
	FCEU_ResetPalette();
	return true;
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	constexpr double ntscFrameRate = 21477272.0 / 357366.0;
	constexpr double palFrameRate = 21281370.0 / 425568.0;
	double systemFrameRate = vidSysIsPAL() ? palFrameRate : ntscFrameRate;
	double mixRate = std::round(rate * (systemFrameRate * frameTime.count()));
	FCEUI_Sound(mixRate);
	logMsg("set NES audio rate %d", FSettings.SndRate);
}

void emulateSound(EmuAudio *audio)
{
	static constexpr size_t maxAudioFrames = 1024;
	int32 sound[maxAudioFrames];
	auto frames = FlushEmulateSound(sound);
	soundtimestamp = 0;
	//logMsg("%d frames", frames);
	assert(frames <= maxAudioFrames);
	if(audio)
	{
		int16 sound16[maxAudioFrames];
		copy_n(sound, maxAudioFrames, sound16);
		audio->writeFrames(sound16, frames);
	}
}

static void renderVideo(EmuSystemTaskContext taskCtx, EmuVideo &video, uint8 *buf)
{
	auto img = video.startFrame(taskCtx);
	auto pix = img.pixmap();
	IG::Pixmap ppuPix{{{256, 256}, IG::PIXEL_FMT_I8}, buf};
	auto ppuPixRegion = ppuPix.subView({0, 8}, {256, 224});
	assumeExpr(pix.size() == ppuPixRegion.size());
	if(pix.format() == IG::PIXEL_RGB565)
	{
		pix.writeTransformed([](uint8 p){ return nativeCol.col16[p]; }, ppuPixRegion);
	}
	else
	{
		assumeExpr(pix.format().bytesPerPixel() == 4);
		pix.writeTransformed([](uint8 p){ return nativeCol.col32[p]; }, ppuPixRegion);
	}
	img.endFrame();
}

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	bool skip = !video && !optionCompatibleFrameskip;
	FCEUI_Emulate(taskCtx, video, skip, audio);
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	renderVideo({}, video, XBuf);
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(1. * .4, 0., 0., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(.5 * .4, 0., 0., 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onInit(IG::ApplicationContext ctx)
{
	appCtx = ctx;
	backupSavestates = 0;
	if(!FCEUI_Initialize())
	{
		throw std::runtime_error{"Error in FCEUI_Initialize"};
	}
}

}

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	using namespace EmuEx;
	if(pixFmt == IG::PIXEL_RGB565)
	{
		nativeCol.col16[index] = pixFmt.desc().build(r >> 3, g >> 2, b >> 3, 0);
	}
	else // RGBA8888
	{
		auto desc = pixFmt == IG::PIXEL_BGRA8888 ? IG::PIXEL_DESC_BGRA8888.nativeOrder() : IG::PIXEL_DESC_RGBA8888_NATIVE;
		nativeCol.col32[index] = desc.build(r, g, b, (uint8)0);
	}
	//logMsg("set palette %d %X", index, nativeCol[index]);
}

void FCEUPPU_FrameReady(EmuEx::EmuSystemTaskContext taskCtx, EmuEx::EmuVideo *video, uint8 *buf)
{
	if(!video)
	{
		return;
	}
	if(!buf) [[unlikely]]
	{
		video->startUnchangedFrame(taskCtx);
		return;
	}
	renderVideo(taskCtx, *video, buf);
}
