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
#include "genesis.h"
#include "genplus-config.h"
#ifndef NO_SCD
#include <scd/scd.h>
#endif
#include <fileio/fileio.h>
#include "Cheats.hh"

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\ncgfm2.emuviews.com";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
t_config config{};
uint config_ym2413_enabled = 1;
int8 mdInputPortDev[2]{-1, -1};
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
t_bitmap bitmap{};
bool usingMultiTap = false;
static uint autoDetectedVidSysPAL = 0;

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

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	//logMsg("frame start");
	RAMCheatUpdate();
	system_frame(!processGfx, renderGfx, emuVideo);

	int16 audioBuff[snd.buffer_size * 2];
	int frames = audio_update(audioBuff);
	if(renderAudio)
	{
		//logMsg("%d frames", frames);
		writeSound(audioBuff, frames);
	}
	//logMsg("frame end");
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

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'A';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
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

static const uint maxSaveStateSize = STATE_SIZE+4;

static std::error_code saveMDState(const char *path)
{
	auto stateData = std::make_unique<uchar[]>(maxSaveStateSize);
	if(!stateData)
		return {ENOMEM, std::system_category()};
	logMsg("saving state data");
	int size = state_save(stateData.get());
	logMsg("writing to file");
	auto ec = writeToNewFile(path, stateData.get(), size);
	if(ec)
	{
		logMsg("error writing state file");
		return ec;
	}
	logMsg("wrote %d byte state", size);
	return {};
}

static std::system_error loadMDState(const char *path)
{
	FileIO f;
	auto ec = f.open(path);
	if(ec)
	{
		return {ec};
	}
	auto stateData = (const uchar *)f.mmapConst();
	if(!stateData)
	{
		return {{EIO, std::system_category()}};
	}
	auto err = state_load(stateData);
	if(err.code())
	{
		return err;
	}
	//sound_restore();
	return {{}};
}

std::error_code EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(saveStr);
	logMsg("saving state %s", saveStr.data());
	return saveMDState(saveStr.data());
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	logMsg("loading state %s", saveStr.data());
	return loadMDState(saveStr.data());
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
			for(uint i = 0; i < 0x10000; i += 2)
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
		fixFilePermissions(saveStr);

		logMsg("saving SRAM%s", optionBigEndianSram ? ", byte-swapped" : "");

		uchar sramTemp[0x10000];
		uchar *sramPtr = sram.sram;
		if(optionBigEndianSram)
		{
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(uint i = 0; i < 0x10000; i += 2)
			{
				std::swap(sramTemp[i], sramTemp[i+1]);
			}
			sramPtr = sramTemp;
		}
		auto ec = writeToNewFile(saveStr.data(), sramPtr, 0x10000);
		if(ec)
			logMsg("error creating sram file");
	}
	writeCheatFile();
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		fixFilePermissions(saveStr);
		saveMDState(saveStr.data());
	}
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

static bool inputPortWasAutoSetByGame(uint port)
{
	return old_system[port] != -1;
}

static void setupSMSInput()
{
	input.system[0] = input.system[1] =  SYSTEM_MS_GAMEPAD;
}

void setupMDInput()
{
	if(!EmuSystem::gameIsRunning())
	{
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
		#endif
		return;
	}

	IG::fillData(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

	uint mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	iterateTimes(4, i)
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSMSInput();
		io_init();
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		vController.gp.activeFaceBtns = 3;
		#endif
		return;
	}

	if(cart.special & HW_J_CART)
	{
		input.system[0] = input.system[1] = SYSTEM_MD_GAMEPAD;
		playerIdxMap[2] = 5;
		playerIdxMap[3] = 6;
	}
	else if(usingMultiTap)
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
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	#endif
}

static uint detectISORegion(uint8 bootSector[0x800])
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

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	closeGame();
	setupGamePaths(path);
	#ifndef NO_SCD
	CDAccess *cd{};
	if(hasMDCDExtension(fullGamePath()) ||
		(string_hasDotExtension(path, "bin") && FS::file_size(fullGamePath()) > 1024*1024*10)) // CD
	{
		FS::current_path(gamePath());
		try
		{
			cd = cdaccess_open_image(fullGamePath(), false);
		}
		catch(std::exception &e)
		{
			popup.printf(4, 1, "%s", e.what());
			return 0;
		}

		uint region = REGION_USA;
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
			popup.printf(4, 1, "Set a %s BIOS in the Options", biosName);
			delete cd;
			return 0;
		}
		FileIO io;
		if(!load_rom(io, biosPath, nullptr))
		{
			popup.printf(4, 1, "Error loading BIOS: %s", biosPath);
			delete cd;
			return 0;
		}
		if(!sCD.isActive)
		{
			popup.printf(4, 1, "Invalid BIOS: %s", biosPath);
			delete cd;
			return 0;
		}
	}
	else
	#endif
	if(hasMDExtension(origFilename)) // ROM
	{
		logMsg("loading ROM %s", path);
		if(!load_rom(io, path, origFilename))
		{
			popup.post("Error loading game", 1);
			return 0;
		}
	}
	else
	{
		popup.post("Invalid game", 1);
		return 0;
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

	configAudioPlayback();
	system_init();
	iterateTimes(2, i)
	{
		if(old_system[i] != -1)
			old_system[i] = input.system[i]; // store input ports set by game
	}
	setupMDInput();

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		auto saveStr = sprintBRAMSaveFilename();
		FileIO bramFile;
		bramFile.open(saveStr.data());

		if(!bramFile)
		{
			logMsg("no BRAM on disk, formatting");
			IG::fillData(bram);
			memcpy(bram + sizeof(bram) - sizeof(fmtBram), fmtBram, sizeof(fmtBram));
			auto sramFormatStart = sram.sram + 0x10000 - sizeof(fmt64kSram);
			memcpy(sramFormatStart, fmt64kSram, sizeof(fmt64kSram));
			for(uint i = 0; i < 0x40; i += 2) // byte-swap sram cart format region
			{
				std::swap(sramFormatStart[i], sramFormatStart[i+1]);
			}
		}
		else
		{
			bramFile.read(bram, sizeof(bram));
			bramFile.read(sram.sram, 0x10000);
			for(uint i = 0; i < 0x10000; i += 2) // byte-swap
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

		if(readFromFile(saveStr.data(), sram.sram, 0x10000) <= 0)
			logMsg("no SRAM on disk");
		else
			logMsg("loaded SRAM from disk%s", optionBigEndianSram ? ", will byte-swap" : "");

		if(optionBigEndianSram)
		{
			for(uint i = 0; i < 0x10000; i += 2)
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
			popup.post("Error loading CD", 1);
			delete cd;
			closeGame();
			return 0;
		}
	}
	#endif

	readCheatFile();
	applyCheats();

	logMsg("started emu");
	return 1;
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	audio_init(optionSoundRate, 1. / frameTime);
	if(gameIsRunning())
		sound_restore();
	logMsg("md sound buffer size %d", snd.buffer_size);
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
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

void EmuSystem::onMainWindowCreated(Base::Window &win)
{
	win.setOnInputEvent(
		[](Base::Window &win, Input::Event e)
		{
			if(EmuSystem::isActive())
			{
				int gunDevIdx = 4;
				if(unlikely(e.isPointer() && input.dev[gunDevIdx] == DEVICE_LIGHTGUN))
				{
					if(emuVideoLayer.gameRect().overlaps({e.x, e.y}))
					{
						int xRel = e.x - emuVideoLayer.gameRect().x, yRel = e.y - emuVideoLayer.gameRect().y;
						input.analog[gunDevIdx][0] = IG::scalePointRange((float)xRel, (float)emuVideoLayer.gameRect().xSize(), (float)bitmap.viewport.w);
						input.analog[gunDevIdx][1] = IG::scalePointRange((float)yRel, (float)emuVideoLayer.gameRect().ySize(), (float)bitmap.viewport.h);
					}
					if(e.state == Input::PUSHED)
					{
						input.pad[gunDevIdx] |= INPUT_A;
						logMsg("gun pushed @ %d,%d, on MD %d,%d", e.x, e.y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
					}
					else if(e.state == Input::RELEASED)
					{
						input.pad[gunDevIdx] = IG::clearBits(input.pad[gunDevIdx], (uint16)INPUT_A);
					}
				}
			}
			handleInputEvent(win, e);
		});
}

CallResult EmuSystem::onInit()
{
	emuVideo.initFormat(pixFmt);
	return OK;
}
