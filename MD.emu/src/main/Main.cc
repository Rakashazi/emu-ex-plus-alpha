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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"
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
#endif
#include <fileio/fileio.h>
#include "Cheats.hh"

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2021\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
t_config config{};
bool config_ym2413_enabled = true;
int8 mdInputPortDev[2]{-1, -1};
t_bitmap bitmap{};
static unsigned autoDetectedVidSysPAL = 0;

bool hasMDExtension(const char *name)
{
	return hasROMExtension(name);
}

static bool hasMDCDExtension(const char *name)
{
	return string_hasDotExtension(name, "cue") || string_hasDotExtension(name, "iso");
}

static bool hasMDWithCDExtension(const char *name)
{
	return hasMDExtension(name)
	#ifndef NO_SCD
		|| hasMDCDExtension(name)
	#endif
	;
}

const char *EmuSystem::shortSystemName()
{
	return "MD-Genesis";
}

const char *EmuSystem::systemName()
{
	return "Mega Drive (Sega Genesis)";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMDWithCDExtension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasMDExtension;

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio)
{
	//logMsg("frame start");
	RAMCheatUpdate();
	system_frame(task, video);

	int16 audioBuff[snd.buffer_size * 2];
	int frames = audio_update(audioBuff);
	if(audio)
	{
		//logMsg("%d frames", frames);
		audio->writeFrames(audioBuff, frames);
	}
	//logMsg("frame end");
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithAltFormat({}, framebufferRenderFormatPixmap());
}

bool EmuSystem::vidSysIsPAL() { return vdp_pal; }

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	#ifndef NO_SCD
	if(sCD.isActive)
		system_reset();
	else
	#endif
		gen_reset(0);
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.gp", statePath, gameName, saveSlotChar(slot));
}

static FS::PathString sprintSaveFilename()
{
	return FS::makePathStringPrintf("%s/%s.srm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static FS::PathString sprintBRAMSaveFilename()
{
	return FS::makePathStringPrintf("%s/%s.brm", EmuSystem::savePath(), EmuSystem::gameName().data());
}

static const unsigned maxSaveStateSize = STATE_SIZE+4;

static EmuSystem::Error saveMDState(const char *path)
{
	auto stateData = std::make_unique<uint8_t[]>(maxSaveStateSize);
	if(!stateData)
		return EmuSystem::makeError("Out of memory");
	logMsg("saving state data");
	int size = state_save(stateData.get());
	logMsg("writing to file");
	std::error_code ec;
	if(FileUtils::writeToPath(path, stateData.get(), size, &ec) == -1)
	{
		return EmuSystem::makeError(std::error_code{ec});
	}
	logMsg("wrote %d byte state", size);
	return {};
}

static EmuSystem::Error loadMDState(const char *path)
{
	FileIO f;
	if(auto ec = f.open(path, IO::AccessHint::ALL);
		ec)
	{
		EmuSystem::makeError(std::error_code{ec});
	}
	auto stateData = (const uint8_t *)f.mmapConst();
	if(!stateData)
	{
		return EmuSystem::makeFileReadError();
	}
	if(auto err = state_load(stateData);
		err)
	{
		return err;
	}
	//sound_restore();
	return {};
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	return saveMDState(path);
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	return loadMDState(path);
}

void EmuSystem::saveBackupMem() // for manually saving when not closing game
{
	if(!gameIsRunning())
		return;
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		logMsg("saving BRAM");
		auto saveStr = sprintBRAMSaveFilename();
		FileIO bramFile;
		bramFile.create(saveStr.data());
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
		auto saveStr = sprintSaveFilename();

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
		if(FileUtils::writeToPath(saveStr.data(), sramPtr, 0x10000, nullptr) == -1)
			logMsg("error creating sram file");
	}
	writeCheatFile();
}

void EmuSystem::closeSystem()
{
	saveBackupMem();
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

void setupMDInput(EmuApp &app)
{
	if(!EmuSystem::gameIsRunning())
	{
		app.setActiveFaceButtons(option6BtnPad ? 6 : 3);
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
		app.setActiveFaceButtons(3);
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
	app.setActiveFaceButtons(option6BtnPad ? 6 : 3);
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

FS::PathString EmuSystem::willLoadGameFromPath(FS::PathString path)
{
	#ifndef NO_SCD
	// check if loading a .bin with matching .cue
	if(string_hasDotExtension(path.data(), "bin"))
	{
		auto len = strlen(path.data());
		auto possibleCuePath = path;
		possibleCuePath[len-3] = 0; // delete extension
		string_cat(possibleCuePath, "cue");
		if(FS::exists(possibleCuePath))
		{
			logMsg("loading %s instead of .bin file", possibleCuePath.data());
			return possibleCuePath;
		}
	}
	#endif
	return path;
}

EmuSystem::Error EmuSystem::loadGame(Base::ApplicationContext ctx, IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	#ifndef NO_SCD
	using namespace Mednafen;
	CDAccess *cd{};
	if(hasMDCDExtension(gameFileName().data()) ||
		(string_hasDotExtension(gameFileName().data(), "bin") && FS::file_size(fullGamePath()) > 1024*1024*10)) // CD
	{
		FS::current_path(gamePath());
		try
		{
			cd = CDAccess_Open(&NVFS, fullGamePath(), false);
		}
		catch(std::exception &e)
		{
			return makeError("%s", e.what());
		}

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

		const char *biosPath = optionCDBiosJpnPath;
		const char *biosName = "Japan";
		switch(region)
		{
			bcase REGION_USA: biosPath = optionCDBiosUsaPath; biosName = "USA";
			bcase REGION_EUROPE: biosPath = optionCDBiosEurPath; biosName = "Europe";
		}
		if(!strlen(biosPath))
		{
			delete cd;
			return makeError("Set a %s BIOS in the Options", biosName);
		}
		if(FileIO io;
			!load_rom(io, biosPath, nullptr))
		{
			delete cd;
			return makeError("Error loading BIOS: %s", biosPath);
		}
		if(!sCD.isActive)
		{
			delete cd;
			return makeError("Invalid BIOS: %s", biosPath);
		}
	}
	else
	#endif
	// ROM
	{
		logMsg("loading ROM %s", fullGamePath());
		if(!load_rom(io, fullGamePath(), originalGameFileName().data()))
		{
			return makeFileReadError();
		}
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
	if(vidSysIsPAL())
		logMsg("using PAL timing");

	system_init();
	iterateTimes(2, i)
	{
		if(old_system[i] != -1)
			old_system[i] = input.system[i]; // store input ports set by game
	}
	setupMDInput(EmuApp::get(ctx));

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		auto saveStr = sprintBRAMSaveFilename();
		FileIO bramFile;
		bramFile.open(saveStr.data(), IO::AccessHint::ALL);

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
		auto saveStr = sprintSaveFilename();

		if(FileUtils::readFromPath(saveStr.data(), sram.sram, 0x10000) <= 0)
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
			delete cd;
			return makeError("Error loading CD");
		}
	}
	#endif

	readCheatFile();
	applyCheats();

	return {};
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	audio_init(rate, 1. / frameTime.count());
	if(gameIsRunning())
		sound_restore();
	logMsg("md sound buffer size %d", snd.buffer_size);
}

void EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	setFramebufferRenderFormat(fmt);
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(0., 0., 1. * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build(0., 0., .6 * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}
