/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "input.h"
#include "io_ctrl.h"
#include "sram.h"
#include "state.h"
#include "sound.h"
#include "vdp_ctrl.h"
#include "vdp_render.h"
#include "genesis.h"
#include "genplus-config.h"
#ifndef NO_SCD
#include <scd/scd.h>
#include <mednafen/mednafen.h>
#include <mednafen/cdrom/CDAccess.h>
#endif
#include "Cheats.hh"
#include <imagine/fs/FS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ScopeGuard.hh>

t_config config{};
t_bitmap bitmap{};
bool config_ym2413_enabled = true;

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::canRenderRGB565 = RENDER_BPP == 16;
bool EmuSystem::canRenderRGBA8888 = RENDER_BPP == 32;
bool EmuApp::needsGlobalInstance = true;

static bool hasBinExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".bin", ".BIN");
}

bool hasMDExtension(std::string_view name)
{
	return hasBinExtension(name) || IG::stringEndsWithAny(name, ".smd", ".md", ".gen", ".SMD", ".MD", ".GEN"
		#ifndef NO_SYSTEM_PBC
		, ".sms", ".SMS"
		#endif
		);
}

static bool hasMDCDExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".cue", ".iso", ".CUE", ".ISO");
}

static bool hasMDWithCDExtension(std::string_view name)
{
	return hasMDExtension(name)
	#ifndef NO_SCD
		|| hasMDCDExtension(name)
	#endif
	;
}

const char *EmuSystem::shortSystemName() const
{
	return "MD-Genesis";
}

const char *EmuSystem::systemName() const
{
	return "Mega Drive (Sega Genesis)";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMDWithCDExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasMDExtension;

void MdSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	//logMsg("frame start");
	RAMCheatUpdate();
	system_frame(taskCtx, video);

	int16 audioBuff[snd.buffer_size * 2];
	int frames = audio_update(audioBuff);
	if(audio)
	{
		//logMsg("%d frames", frames);
		audio->writeFrames(audioBuff, frames);
	}
	//logMsg("frame end");
}

void MdSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithAltFormat({}, framebufferRenderFormatPixmap());
}

VideoSystem MdSystem::videoSystem() const { return vdp_pal ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC; }

void MdSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	#ifndef NO_SCD
	if(sCD.isActive)
		system_reset();
	else
	#endif
		gen_reset(0);
}

FS::FileString MdSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}.gp", name, saveSlotChar(slot));
}

static FS::PathString saveFilename(EmuSystem &sys)
{
	return sys.contentSaveFilePath(".srm");
}

static FS::PathString bramSaveFilename(EmuSystem &sys)
{
	return sys.contentSaveFilePath(".brm");
}

static const unsigned maxSaveStateSize = STATE_SIZE+4;

void MdSystem::saveState(IG::CStringView path)
{
	auto stateData = std::make_unique<uint8_t[]>(maxSaveStateSize);
	logMsg("saving state data");
	size_t size = state_save(stateData.get());
	logMsg("writing to file");
	if(FileUtils::writeToUri(appContext(), path, {stateData.get(), size}) == -1)
		throwFileWriteError();
	logMsg("wrote %zu byte state", size);
}

void MdSystem::loadState(EmuApp &app, IG::CStringView path)
{
	state_load(FileUtils::bufferFromUri(app.appContext(), path).data());
}

static bool sramHasContent(std::span<uint8> sram)
{
	for(auto v : sram)
	{
		if(v != 0xFF)
			return true;
	}
	return false;
}

void MdSystem::onFlushBackupMemory(BackupMemoryDirtyFlags)
{
	if(!hasContent())
		return;
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		logMsg("saving BRAM");
		auto saveStr = bramSaveFilename(*this);
		auto bramFile = appContext().openFileUri(saveStr, IO::OPEN_NEW | IO::TEST_BIT);
		if(!bramFile)
			logMsg("error creating bram file");
		else
		{
			bramFile.write(bram, sizeof(bram));
			char sramTemp[0x10000];
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(unsigned i = 0; i < 0x10000; i += 2)
			{
				std::swap(sramTemp[i], sramTemp[i+1]);
			}
			bramFile.write(sramTemp, 0x10000);
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = saveFilename(*this);
		if(sramHasContent({sram.sram, 0x10000}))
		{
			logMsg("saving SRAM%s", optionBigEndianSram ? ", byte-swapped" : "");
			uint8_t sramTemp[0x10000];
			uint8_t *sramPtr = sram.sram;
			if(optionBigEndianSram)
			{
				memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
				for(unsigned i = 0; i < 0x10000; i += 2)
				{
					std::swap(sramTemp[i], sramTemp[i+1]);
				}
				sramPtr = sramTemp;
			}
			try
			{
				FileUtils::writeToUri(appContext(), saveStr, {sramPtr, 0x10000});
			}
			catch(...)
			{
				logMsg("error creating sram file");
			}
		}
		else
		{
			logMsg("SRAM wasn't written to");
			appContext().removeFileUri(saveStr);
		}
	}
}

void MdSystem::closeSystem()
{
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		scd_deinit();
	}
	#endif
	old_system[0] = old_system[1] = -1;
	clearCheatList();
}

const char *mdInputSystemToStr(uint8 system)
{
	switch(system)
	{
		case NO_SYSTEM: return "unconnected";
		case SYSTEM_MD_GAMEPAD: return "gamepad";
		case SYSTEM_MS_GAMEPAD: return "sms gamepad";
		case SYSTEM_MOUSE: return "mouse";
		case SYSTEM_MENACER: return "menacer";
		case SYSTEM_JUSTIFIER: return "justifier";
		case SYSTEM_TEAMPLAYER: return "team-player";
		default : return "unknown";
	}
}

static bool inputPortWasAutoSetByGame(unsigned port)
{
	return old_system[port] != -1;
}

static void setupSMSInput()
{
	input.system[0] = input.system[1] =  SYSTEM_MS_GAMEPAD;
}

void MdSystem::setupMDInput(EmuApp &app)
{
	static constexpr std::pair<int, bool> setMd6BGamepad[]
	{
		{0, true}, {1, true}, {2, true}, {3, true}, {4, true}, {5, true}
	};
	static constexpr std::pair<int, bool> setMdGamepad[]
	{
		{0, true}, {1, true}, {2, true}, {3, false}, {4, false}, {5, false}
	};
	static constexpr std::pair<int, bool> setM3Gamepad[]
	{
		{0, false}, {1, true}, {2, true}, {3, false}, {4, false}, {5, false}
	};
	static constexpr std::pair<int, bool> enableModeBtn[]{{0, true}};
	static constexpr std::pair<int, bool> disableModeBtn[]{{0, false}};
	if(!hasContent())
	{
		app.applyEnabledFaceButtons(option6BtnPad ? setMd6BGamepad : setMdGamepad);
		app.applyEnabledCenterButtons(option6BtnPad ? enableModeBtn : disableModeBtn);
		return;
	}

	IG::fill(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

	unsigned mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	iterateTimes(4, i)
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSMSInput();
		io_init();
		app.applyEnabledFaceButtons(setM3Gamepad);
		app.applyEnabledCenterButtons(disableModeBtn);
		return;
	}

	if(cart.special & HW_J_CART)
	{
		input.system[0] = input.system[1] = SYSTEM_MD_GAMEPAD;
		playerIdxMap[2] = 5;
		playerIdxMap[3] = 6;
	}
	else if(optionMultiTap)
	{
		input.system[0] = SYSTEM_TEAMPLAYER;
		input.system[1] = 0;

		playerIdxMap[1] = 1;
		playerIdxMap[2] = 2;
		playerIdxMap[3] = 3;
	}
	else
	{
		iterateTimes(2, i)
		{
			if(mdInputPortDev[i] == -1) // user didn't specify device, go with auto settings
			{
				if(!inputPortWasAutoSetByGame(i))
					input.system[i] = SYSTEM_MD_GAMEPAD;
				else
				{
					logMsg("input port %d set by game detection", i);
					input.system[i] = old_system[i];
				}
			}
			else
				input.system[i] = mdInputPortDev[i];
			logMsg("attached %s to port %d%s", mdInputSystemToStr(input.system[i]), i, mdInputPortDev[i] == -1 ? " (auto)" : "");
		}
	}

	io_init();
	app.applyEnabledFaceButtons(option6BtnPad ? setMd6BGamepad : setMdGamepad);
	app.applyEnabledCenterButtons(option6BtnPad ? enableModeBtn : disableModeBtn);
}

static unsigned detectISORegion(uint8 bootSector[0x800])
{
	auto bootByte = bootSector[0x20b];

	if(bootByte == 0x7a)
		return REGION_USA;
	else if(bootByte == 0x64)
		return REGION_EUROPE;
	else
		return REGION_JAPAN_NTSC;
}

FS::PathString EmuSystem::willLoadContentFromPath(std::string_view path, std::string_view displayName)
{
	#ifndef NO_SCD
	// check if loading a .bin with matching .cue
	if(hasBinExtension(path))
	{
		FS::PathString possibleCuePath{path};
		possibleCuePath.replace(possibleCuePath.end() - 3, possibleCuePath.end(), "cue");
		if(appContext().fileUriExists(possibleCuePath))
		{
			logMsg("loading %s instead of .bin file", possibleCuePath.data());
			return possibleCuePath;
		}
	}
	#endif
	return FS::PathString{path};
}

void MdSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	#ifndef NO_SCD
	using namespace Mednafen;
	CDAccess *cd{};
	auto deleteCDAccess = IG::scopeGuard([&](){ delete cd; });
	if(hasMDCDExtension(contentFileName()) ||
		(hasBinExtension(contentFileName()) && io.size() > 1024*1024*10)) // CD
	{
		if(contentDirectory().empty())
		{
			throwMissingContentDirError();
		}
		cd = CDAccess_Open(&NVFS, std::string{contentLocation()}, false);

		unsigned region = REGION_USA;
		if (config.region_detect == 1) region = REGION_USA;
	  else if (config.region_detect == 2) region = REGION_EUROPE;
	  else if (config.region_detect == 3) region = REGION_JAPAN_NTSC;
	  else if (config.region_detect == 4) region = REGION_JAPAN_PAL;
	  else
	  {
	  	uint8 bootSector[2048];
	  	cd->Read_Sector(bootSector, 0, 2048);
			region = detectISORegion(bootSector);
	  }

		std::string_view biosPath = cdBiosJpnPath;
		std::string_view biosName = "Japan";
		switch(region)
		{
			bcase REGION_USA: biosPath = cdBiosUSAPath; biosName = "USA";
			bcase REGION_EUROPE: biosPath = cdBiosEurPath; biosName = "Europe";
		}
		if(biosPath.empty())
		{
			throw std::runtime_error(fmt::format("Set a {} BIOS in the Options", biosName));
		}
		auto [biosSize, biosFilename] = FileUtils::readFromUriWithArchiveScan(appContext(), biosPath, {cart.rom, MAXROMSIZE}, hasMDExtension);
		if(biosSize <= 0)
			throw std::runtime_error(fmt::format("Error loading BIOS: {}", biosPath));
		init_rom(biosSize, "");
		if(!sCD.isActive)
		{
			throw std::runtime_error(fmt::format("Invalid BIOS: {}", biosPath));
		}
	}
	else
	#endif
	// ROM
	{
		logMsg("loading ROM %s", contentLocation().data());
		auto size = io.read(cart.rom, MAXROMSIZE);
		if(size <= 0)
			throwFileReadError();
		init_rom(size, contentFileName());
	}
	autoDetectedVidSysPAL = vdp_pal;
	if((int)optionVideoSystem == 1)
	{
		vdp_pal = 0;
	}
	else if((int)optionVideoSystem == 2)
	{
		vdp_pal = 1;
	}
	if(videoSystem() == VideoSystem::PAL)
		logMsg("using PAL timing");

	system_init();
	iterateTimes(2, i)
	{
		if(old_system[i] != -1)
			old_system[i] = input.system[i]; // store input ports set by game
	}
	setupMDInput(EmuApp::get(appContext()));

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		auto saveStr = bramSaveFilename(*this);
		auto bramFile = appContext().openFileUri(saveStr, IO::AccessHint::ALL, IO::TEST_BIT);
		if(!bramFile)
		{
			logMsg("no BRAM on disk, formatting");
			IG::fill(bram);
			memcpy(bram + sizeof(bram) - sizeof(fmtBram), fmtBram, sizeof(fmtBram));
			auto sramFormatStart = sram.sram + 0x10000 - sizeof(fmt64kSram);
			memcpy(sramFormatStart, fmt64kSram, sizeof(fmt64kSram));
			for(unsigned i = 0; i < 0x40; i += 2) // byte-swap sram cart format region
			{
				std::swap(sramFormatStart[i], sramFormatStart[i+1]);
			}
		}
		else
		{
			bramFile.read(bram, sizeof(bram));
			bramFile.read(sram.sram, 0x10000);
			for(unsigned i = 0; i < 0x10000; i += 2) // byte-swap
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
			}
			logMsg("loaded BRAM from disk");
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = saveFilename(*this);

		if(FileUtils::readFromUri(appContext(), saveStr, {sram.sram, 0x10000}) <= 0)
			logMsg("no SRAM on disk");
		else
			logMsg("loaded SRAM from disk%s", optionBigEndianSram ? ", will byte-swap" : "");

		if(optionBigEndianSram)
		{
			for(unsigned i = 0; i < 0x10000; i += 2)
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
			}
		}
	}

	system_reset();

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		if(Insert_CD(cd) != 0)
		{
			throw std::runtime_error("Error loading CD");
		}
		deleteCDAccess.cancel();
	}
	#endif

	readCheatFile(*this);
	applyCheats();
}

void MdSystem::configAudioRate(IG::FloatSeconds frameTime, int rate)
{
	audio_init(rate, 1. / frameTime.count());
	if(hasContent())
		sound_restore();
	logMsg("md sound buffer size %d", snd.buffer_size);
}

bool MdSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	setFramebufferRenderFormat(fmt);
	return false;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(0., 0., .6 * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}
