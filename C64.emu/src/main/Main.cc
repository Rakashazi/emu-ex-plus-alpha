/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAppInlines.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/gui/AlertView.hh>
#include "internal.hh"
#include <sys/time.h>

extern "C"
{
	#include "machine.h"
	#include "maincpu.h"
	#include "drive.h"
	#include "lib.h"
	#include "util.h"
	#include "ioutil.h"
	#include "uiapi.h"
	#include "console.h"
	#include "monitor.h"
	#include "video.h"
	#include "cmdline.h"
	#include "gfxoutput.h"
	#include "videoarch.h"
	#include "init.h"
	#include "resources.h"
	#include "sysfile.h"
	#include "log.h"
	#include "archdep.h"
	#include "palette.h"
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
	#include "keyboard.h"
	#include "autostart.h"
	#include "kbdbuf.h"
	#include "attach.h"
	#include "raster.h"
	#include "sound.h"
	#include "cartridge.h"
	#include "tape.h"
	#include "interrupt.h"
	#include "sid/sid.h"
	#include "c64/cart/c64cartsystem.h"
	#include "vicii.h"
	#include "diskimage.h"
	#include "vdrive-internal.h"
	#include "autostart-prg.h"
}

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2013-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nwww.viceteam.org";
IG::Semaphore execSem{0}, execDoneSem{0};
bool runningFrame = false, doAudio = false;
static bool c64IsInit = false, c64FailedInit = false;
FS::PathString firmwareBasePath{};
FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3]{};
bool isPal = false;
VicePlugin plugin{};
ViceSystem currSystem = VICE_SYSTEM_C64;
static int defaultNTSCModel[]
{
	C64MODEL_C64_NTSC,
	C64MODEL_C64_NTSC,
	DTVMODEL_V3_NTSC,
	C128MODEL_C128_NTSC,
	C64MODEL_C64_NTSC,
	CBM2MODEL_610_NTSC,
	CBM2MODEL_510_NTSC,
	PETMODEL_8032,
	PLUS4MODEL_PLUS4_NTSC,
	VIC20MODEL_VIC20_NTSC
};
static int defaultPALModel[]
{
	C64MODEL_C64_PAL,
	C64MODEL_C64_PAL,
	DTVMODEL_V3_PAL,
	C128MODEL_C128_PAL,
	C64MODEL_C64_NTSC,
	CBM2MODEL_610_PAL,
	CBM2MODEL_510_PAL,
	PETMODEL_8032,
	PLUS4MODEL_PLUS4_PAL,
	VIC20MODEL_VIC20_PAL
};

bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::handlesGenericIO = false;

const char *EmuSystem::shortSystemName()
{
	return "C64";
}

const char *EmuSystem::systemName()
{
	return "Commodore 64";
}

int intResource(const char *name)
{
	int val = 0;
	auto failed = plugin.resources_get_int(name, &val);
	assert(!failed);
	return val;
}

void setAutostartWarp(bool on)
{
	plugin.resources_set_int("AutostartWarp", on);
}

static bool autostartWarp()
{
	return intResource("AutostartWarp");
}

void setAutostartTDE(bool on)
{
	plugin.resources_set_int("AutostartHandleTrueDriveEmulation", on);
}

static bool autostartTDE()
{
	return intResource("AutostartHandleTrueDriveEmulation");
}

static bool sysIsPal()
{
	switch(intResource("MachineVideoStandard"))
	{
		case MACHINE_SYNC_PAL:
		case MACHINE_SYNC_PALN:
			return true;
	}
	return false;
}

static void setModel(int model)
{
	logMsg("setting model id:%d", model);
	plugin.model_set(model);
}

int sysModel()
{
	return plugin.model_get();
}

void setSysModel(int model)
{
	setModel(model);
	isPal = sysIsPal();
	if(isPal)
	{
		logMsg("C64 model has PAL timings");
	}
}

void setDefaultNTSCModel()
{
	setSysModel(defaultNTSCModel[currSystem]);
}

void setDefaultPALModel()
{
	setSysModel(defaultPALModel[currSystem]);
}

void setBorderMode(int mode)
{
	if(!plugin.borderModeStr)
		return;
	plugin.resources_set_int(plugin.borderModeStr, mode);
}

static int borderMode()
{
	if(!plugin.borderModeStr)
		return -1;
	return intResource(plugin.borderModeStr);
}

void setSidEngine(int engine)
{
	plugin.resources_set_int("SidEngine", engine);
}

static int sidEngine()
{
	return intResource("SidEngine");
}

void setDriveTrueEmulation(bool on)
{
	plugin.resources_set_int("DriveTrueEmulation", on);
}

bool driveTrueEmulation()
{
	return intResource("DriveTrueEmulation");
}

void setVirtualDeviceTraps(bool on)
{
	plugin.resources_set_int("VirtualDevices", on);
}

bool virtualDeviceTraps()
{
	return intResource("VirtualDevices");
}

void setDefaultC64Model(int model)
{
	optionC64Model = model;
	if(!EmuSystem::gameIsRunning() &&
		(currSystem == VICE_SYSTEM_C64 || currSystem == VICE_SYSTEM_C64SC))
	{
		setSysModel(model);
	}
}

void setDefaultDTVModel(int model)
{
	optionDTVModel = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_C64DTV)
	{
		setSysModel(model);
	}
}

void setDefaultC128Model(int model)
{
	optionC128Model = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_C128)
	{
		setSysModel(model);
	}
}

void setDefaultSuperCPUModel(int model)
{
	optionSuperCPUModel = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_SUPER_CPU)
	{
		setSysModel(model);
	}
}

void setDefaultCBM2Model(int model)
{
	optionCBM2Model = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_CBM2)
	{
		setSysModel(model);
	}
}

void setDefaultCBM5x0Model(int model)
{
	optionCBM5x0Model = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_CBM5X0)
	{
		setSysModel(model);
	}
}

void setDefaultPETModel(int model)
{
	optionPETModel = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_PET)
	{
		setSysModel(model);
	}
}

void setDefaultPlus4Model(int model)
{
	optionPlus4Model = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_PLUS4)
	{
		setSysModel(model);
	}
}

void setDefaultVIC20Model(int model)
{
	optionVIC20Model = model;
	if(!EmuSystem::gameIsRunning() && currSystem == VICE_SYSTEM_VIC20)
	{
		setSysModel(model);
	}
}

static void setIntResourceToDefault(const char *name)
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return;
	}
	logMsg("setting resource %s to default:%d", name, val);
	plugin.resources_set_int(name, val);
}

void applyInitialOptionResources()
{
	setVirtualDeviceTraps(optionVirtualDeviceTraps);
	setDriveTrueEmulation(optionDriveTrueEmulation);
	setAutostartWarp(optionAutostartWarp);
	setAutostartTDE(optionAutostartTDE);
	setBorderMode(optionBorderMode);
	setSidEngine(optionSidEngine);
	// default drive setup
	setIntResourceToDefault("Drive8Type");
	plugin.resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
	plugin.resources_set_int("Drive10Type", DRIVE_TYPE_NONE);
	plugin.resources_set_int("Drive11Type", DRIVE_TYPE_NONE);
}

bool hasC64DiskExtension(const char *name)
{
	return string_hasDotExtension(name, "d64") ||
			string_hasDotExtension(name, "d67") ||
			string_hasDotExtension(name, "d71") ||
			string_hasDotExtension(name, "d80") ||
			string_hasDotExtension(name, "d81") ||
			string_hasDotExtension(name, "d82") ||
			string_hasDotExtension(name, "d1m") ||
			string_hasDotExtension(name, "d2m") ||
			string_hasDotExtension(name, "d4m") ||
			string_hasDotExtension(name, "g64") ||
			string_hasDotExtension(name, "p64") ||
			string_hasDotExtension(name, "g41") ||
			string_hasDotExtension(name, "x64") ||
			string_hasDotExtension(name, "dsk");
}

bool hasC64TapeExtension(const char *name)
{
	return string_hasDotExtension(name, "t64") ||
			string_hasDotExtension(name, "tap");
}

bool hasC64CartExtension(const char *name)
{
	return string_hasDotExtension(name, "bin") ||
			string_hasDotExtension(name, "crt");
}

static bool hasC64Extension(const char *name)
{
	return hasC64DiskExtension(name) ||
			hasC64TapeExtension(name) ||
			hasC64CartExtension(name) ||
			string_hasDotExtension(name, "prg") ||
			string_hasDotExtension(name, "p00");
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasC64Extension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasC64Extension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	plugin.machine_trigger_reset(mode == RESET_HARD ? MACHINE_RESET_MODE_HARD : MACHINE_RESET_MODE_SOFT);
}

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *statePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.%c.vsf", statePath, gameName, saveSlotChar(slot));
}

struct SnapshotTrapData
{
	constexpr SnapshotTrapData() {}
	bool hasError = true;
	FS::PathString pathStr{};
};

static void loadSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("loading state: %s", snapData->pathStr.data());
	if(plugin.machine_read_snapshot(snapData->pathStr.data(), 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

static void saveSnapshotTrap(WORD, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("saving state: %s", snapData->pathStr.data());
	if(plugin.machine_write_snapshot(snapData->pathStr.data(), 1, 1, 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

std::error_code EmuSystem::saveState()
{
	SnapshotTrapData data;
	data.pathStr = sprintStateFilename(saveStateSlot);
	fixFilePermissions(data.pathStr);
	plugin.interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	return data.hasError ? std::error_code{EIO, std::system_category()} : std::error_code{};
}

std::system_error EmuSystem::loadState(int saveStateSlot)
{
	plugin.resources_set_int("WarpMode", 0);
	SnapshotTrapData data;
	data.pathStr = sprintStateFilename(saveStateSlot);
	runFrame(0, 0, 0); // run extra frame in case C64 was just started
	plugin.interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap, snapshot load may cause reboot from a C64 model change
	if(data.hasError)
		return {{EIO, std::system_category()}};
	// reload snapshot in case last load caused a reboot
	plugin.interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	runFrame(0, 0, 0); // execute cpu trap
	bool hasError = data.hasError;
	isPal = sysIsPal();
	return hasError ? std::system_error{{EIO, std::system_category()}} : std::system_error{{}};
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		SnapshotTrapData data;
		data.pathStr = sprintStateFilename(-1);
		fixFilePermissions(data.pathStr);
		plugin.interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
		runFrame(0, 0, 0); // execute cpu trap
		if(data.hasError)
		{
			logErr("error writing auto-save state %s", data.pathStr.data());
		}
	}
}

void EmuSystem::saveBackupMem()
{
	if(gameIsRunning())
	{
		// nothing to do for now
	}
}

bool EmuSystem::vidSysIsPAL() { return isPal; }

void EmuSystem::closeSystem()
{
	assert(gameIsRunning());
	logMsg("closing game %s", gameName().data());
	saveBackupMem();
	plugin.resources_set_int("WarpMode", 0);
	plugin.tape_image_detach(1);
	plugin.file_system_detach_disk(8);
	plugin.file_system_detach_disk(9);
	plugin.file_system_detach_disk(10);
	plugin.file_system_detach_disk(11);
	plugin.cartridge_detach_image(-1);
	setSysModel(optionModel(currSystem));
	plugin.machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

static void popupC64FirmwareError()
{
	popup.printf(6, 1, "System files missing, place C64, DRIVES, & PRINTER directories from VICE"
		" in a path below, or set a custom path in options:\n"
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		"%s\n%s\n%s", Base::assetPath().data(), "~/.local/share/C64.emu", "/usr/share/games/vice");
		#else
		"%s/C64.emu", Base::storagePath().data());
		#endif
}

static bool initC64()
{
	if(c64IsInit)
		return true;
	logMsg("initializing C64");
  if(plugin.init_main() < 0)
  {
  	logErr("error in init_main()");
  	c64FailedInit = true;
  	return false;
	}
	c64IsInit = true;
	return true;
}

int loadGame(const char *path, bool autoStartMedia)
{
	if(!initC64())
	{
		popupC64FirmwareError();
		return 0;
	}
	if(c64FailedInit)
	{
		auto &ynAlertView = *EmuSystem::makeYesNoAlertView(
			"A previous system file load failed, you must restart the app to run any C64 software", "Exit Now", "Cancel");
		ynAlertView.setOnYes(
			[](TextMenuItem &, View &, Input::Event e)
			{
				Base::exit();
			});
		modalViewController.pushAndShow(ynAlertView, Input::defaultEvent()); // TODO: loadGame should propagate input event
		return 0;
	}
	EmuSystem::closeGame();
	applyInitialOptionResources();
	EmuSystem::setupGamePaths(path);
	logMsg("loading %s", path);
	if(autoStartMedia)
	{
		if(plugin.autostart_autodetect_)
		{
			if(plugin.autostart_autodetect(path, nullptr, 0, AUTOSTART_MODE_RUN) != 0)
			{
				popup.postError("Error loading file");
				return 0;
			}
		}
		else
		{
			// TODO
		}
	}
	else
	{
		if(hasC64DiskExtension(path))
		{
			if(plugin.file_system_attach_disk(8, path) != 0)
			{
				popup.postError("Error loading file");
				return 0;
			}
		}
	}
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	return ::loadGame(path, true);
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *origFilename)
{
	return 0; // TODO
}

static void execC64Frame()
{
	// signal C64 thread to execute one frame and wait for it to finish
	execSem.notify();
	execDoneSem.wait();
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	runningFrame = 1;
	// "Warp" mode frame
	if(unlikely(*plugin.warp_mode_enabled && renderGfx))
	{
		setCanvasSkipFrame(true);
		iterateTimes(8, i)
		{
			execC64Frame();
		}
	}

	// Normal frame
	doAudio = renderAudio;
	setCanvasSkipFrame(!processGfx);
	execC64Frame();
	if(renderGfx)
	{
		emuVideo.initImage(0, canvasSrcPix.w(), canvasSrcPix.h());
		emuVideo.writeFrame(canvasSrcPix);
		updateAndDrawEmuVideo();
	}
	runningFrame = 0;
}

void EmuSystem::configAudioRate(double frameTime)
{
	logMsg("set audio rate %d", (int)optionSoundRate);
	pcmFormat.rate = optionSoundRate;
	int mixRate = std::round(optionSoundRate * (systemFrameRate * frameTime));
	int currRate = 0;
	plugin.resources_get_int("SoundSampleRate", &currRate);
	if(currRate != mixRate)
	{
		plugin.resources_set_int("SoundSampleRate", mixRate);
	}
}

void EmuSystem::onCustomizeNavView(EmuNavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build(48./255., 36./255., 144./255., 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((48./255.) * .4, (36./255.) * .4, (144./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onMainWindowCreated(Base::Window &)
{
	sysFilePath[0] = firmwareBasePath;
	vController.updateKeyboardMapping();
	setSysModel(optionModel(currSystem));
	plugin.resources_set_string("AutostartPrgDiskImage", "AutostartPrgDisk.d64");
}

CallResult EmuSystem::onInit()
{
	emuVideo.initFormat(pixFmt);
	IG::makeDetachedThread(
		[]()
		{
			execSem.wait();
			logMsg("running C64");
			plugin.maincpu_mainloop();
		});

	#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
	sysFilePath[1] = Base::assetPath();
	sysFilePath[2] = FS::makePathStringPrintf("%s/C64.emu.zip", Base::assetPath().data());
	sysFilePath[3] = {"~/.local/share/C64.emu"};
	sysFilePath[4] = {"/usr/share/games/vice"};
	#else
	{
		sysFilePath[1] = FS::makePathStringPrintf("%s/C64.emu", Base::storagePath().data());
		sysFilePath[2] = FS::makePathStringPrintf("%s/C64.emu.zip", Base::storagePath().data());
	}
	#endif

	EmuSystem::pcmFormat.channels = 1;
	return OK;
}
