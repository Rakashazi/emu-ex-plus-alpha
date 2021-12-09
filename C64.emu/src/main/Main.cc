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
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuAppInlines.hh>
#include "internal.hh"
#include <imagine/thread/Thread.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
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
	#include "sid/sid-resources.h"
	#include "c64/cart/c64cartsystem.h"
	#include "vicii.h"
	#include "diskimage.h"
	#include "vdrive-internal.h"
	#include "autostart-prg.h"
	#include "joyport.h"
}

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2013-2021\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nwww.viceteam.org";
std::binary_semaphore execSem{0}, execDoneSem{0};
EmuAudio *audioPtr{};
static bool c64IsInit = false, c64FailedInit = false;
FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3]{};
VicePlugin plugin{};
ViceSystem currSystem = VICE_SYSTEM_C64;
Base::ApplicationContext appContext{};
IG::PixelFormat pixFmt{};

bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::handlesGenericIO = false;

static void execC64Frame();

const char *EmuSystem::shortSystemName()
{
	return "C64";
}

const char *EmuSystem::systemName()
{
	return "Commodore 64";
}

void setAutostartWarp(bool on)
{
	setIntResource("AutostartWarp", on);
}

static bool autostartWarp()
{
	return intResource("AutostartWarp");
}

void setAutostartTDE(bool on)
{
	setIntResource("AutostartHandleTrueDriveEmulation", on);
}

static bool autostartTDE()
{
	return intResource("AutostartHandleTrueDriveEmulation");
}

void setAutostartBasicLoad(bool on)
{
	setIntResource("AutostartBasicLoad", on);
}

bool autostartBasicLoad()
{
	return intResource("AutostartBasicLoad");
}

static bool sysIsPal()
{
	switch(intResource("MachineVideoStandard"))
	{
		case MACHINE_SYNC_PAL:
		case MACHINE_SYNC_PALN:
			return true;
		default: return false;
	}
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
}

void setBorderMode(int mode)
{
	if(!plugin.borderModeStr)
		return;
	setIntResource(plugin.borderModeStr, mode);
}

static int borderMode()
{
	if(!plugin.borderModeStr)
		return -1;
	return intResource(plugin.borderModeStr);
}

void setSidEngine(int engine)
{
	logMsg("set SID engine %d", engine);
	setIntResource("SidEngine", engine);
}

static int sidEngine()
{
	return intResource("SidEngine");
}

void setReSidSampling(int sampling)
{
	logMsg("set ReSID sampling %d", sampling);
	setIntResource("SidResidSampling", sampling);
}

static int reSidSampling()
{
	return intResource("SidResidSampling");
}

void setDriveTrueEmulation(bool on)
{
	setIntResource("DriveTrueEmulation", on);
}

bool driveTrueEmulation()
{
	return intResource("DriveTrueEmulation");
}

void setVirtualDeviceTraps(bool on)
{
	setIntResource("VirtualDevices", on);
}

bool virtualDeviceTraps()
{
	return intResource("VirtualDevices");
}

void setDefaultC64Model(int model)
{
	optionC64Model = model;
}

void setDefaultDTVModel(int model)
{
	optionDTVModel = model;
}

void setDefaultC128Model(int model)
{
	optionC128Model = model;
}

void setDefaultSuperCPUModel(int model)
{
	optionSuperCPUModel = model;
}

void setDefaultCBM2Model(int model)
{
	optionCBM2Model = model;
}

void setDefaultCBM5x0Model(int model)
{
	optionCBM5x0Model = model;
}

void setDefaultPETModel(int model)
{
	optionPETModel = model;
}

void setDefaultPlus4Model(int model)
{
	optionPlus4Model = model;
}

void setDefaultVIC20Model(int model)
{
	optionVIC20Model = model;
}

const char *videoChipStr()
{
	switch (currSystem)
	{
		case VICE_SYSTEM_VIC20:
		return "VIC";

		case VICE_SYSTEM_C64:
		case VICE_SYSTEM_C64SC:
		case VICE_SYSTEM_SUPER_CPU:
		case VICE_SYSTEM_C64DTV:
		case VICE_SYSTEM_C128:
		case VICE_SYSTEM_CBM5X0:
		return "VICII";

		case VICE_SYSTEM_PLUS4:
		return "TED";

		case VICE_SYSTEM_PET:
		case VICE_SYSTEM_CBM2:
		return "Crtc";
	}
	return "";
}

static void setIntResourceToDefault(const char *name)
{
	int val;
	if(plugin.resources_get_default_value(name, &val) < 0)
	{
		return;
	}
	logMsg("setting resource %s to default:%d", name, val);
	setIntResource(name, val);
}

void applySessionOptions()
{
	if((int)optionModel == -1)
	{
		logMsg("using default model");
		setSysModel(optionDefaultModel(currSystem));
	}
	else
	{
		setSysModel(optionModel);
	}
	setVirtualDeviceTraps(optionVirtualDeviceTraps);
	setDriveTrueEmulation(optionDriveTrueEmulation);
	setAutostartWarp(optionAutostartWarp);
	setAutostartTDE(optionAutostartTDE);
	setAutostartBasicLoad(optionAutostartBasicLoad);
	if(currSystem == VICE_SYSTEM_VIC20)
	{
		uint8_t blocks = optionVic20RamExpansions;
		if(blocks & BLOCK_0)
			setIntResource("RamBlock0", 1);
		if(blocks & BLOCK_1)
			setIntResource("RamBlock1", 1);
		if(blocks & BLOCK_2)
			setIntResource("RamBlock2", 1);
		if(blocks & BLOCK_3)
			setIntResource("RamBlock3", 1);
		if(blocks & BLOCK_5)
			setIntResource("RamBlock5", 1);
	}
}

static void applyInitialOptionResources()
{
	setIntResource("JoyPort1Device", JOYPORT_ID_JOYSTICK);
	setIntResource("JoyPort2Device", JOYPORT_ID_JOYSTICK);
	applySessionOptions();
	setBorderMode(optionBorderMode);
	setSidEngine(optionSidEngine);
	setReSidSampling(optionReSidSampling);
	// default drive setup
	setIntResourceToDefault("Drive8Type");
	setIntResource("Drive9Type", DRIVE_TYPE_NONE);
	setIntResource("Drive10Type", DRIVE_TYPE_NONE);
	setIntResource("Drive11Type", DRIVE_TYPE_NONE);
}

int systemCartType(ViceSystem system)
{
	switch(system)
	{
		case VICE_SYSTEM_CBM2:
		case VICE_SYSTEM_CBM5X0:
			return CARTRIDGE_CBM2_8KB_1000;
		case VICE_SYSTEM_PLUS4:
			return CARTRIDGE_PLUS4_DETECT;
		case VICE_SYSTEM_VIC20:
			return CARTRIDGE_VIC20_DETECT;
		default:
			return CARTRIDGE_CRT;
	}
}

bool hasC64DiskExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name,
		".d64", ".d67", ".d71", ".d80", ".d81", ".d82", ".d1m", ".d2m", ".d4m", ".g64", ".p64", ".g41", ".x64", ".dsk");
}

bool hasC64TapeExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".t64", ".tap");
}

bool hasC64CartExtension(std::string_view name)
{
	return IG::stringEndsWithAny(name, ".bin", ".crt");
}

static bool hasC64Extension(std::string_view name)
{
	return hasC64DiskExtension(name) ||
			hasC64TapeExtension(name) ||
			hasC64CartExtension(name) ||
			IG::stringEndsWithAny(name, ".prg", ".p00");
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasC64Extension;
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = hasC64Extension;

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	plugin.machine_trigger_reset(mode == RESET_HARD ? MACHINE_RESET_MODE_HARD : MACHINE_RESET_MODE_SOFT);
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.{}.vsf", name, saveSlotChar(slot));
}

struct SnapshotTrapData
{
	constexpr SnapshotTrapData() {}
	bool hasError = true;
	const char *pathStr{};
};

static void loadSnapshotTrap(uint16_t, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("loading state: %s", snapData->pathStr);
	if(plugin.machine_read_snapshot(snapData->pathStr, 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

static void saveSnapshotTrap(uint16_t, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("saving state: %s", snapData->pathStr);
	if(plugin.machine_write_snapshot(snapData->pathStr, 1, 1, 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

void EmuSystem::saveState(IG::CStringView path)
{
	SnapshotTrapData data;
	data.pathStr = path;
	plugin.interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
	execC64Frame(); // execute cpu trap
	if(data.hasError)
		throwFileWriteError();
}

void EmuSystem::loadState(IG::CStringView path)
{
	setIntResource("WarpMode", 0);
	SnapshotTrapData data;
	data.pathStr = path;
	execC64Frame(); // run extra frame in case C64 was just started
	plugin.interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	execC64Frame(); // execute cpu trap, snapshot load may cause reboot from a C64 model change
	if(data.hasError)
		return throwFileReadError();
	// reload snapshot in case last load caused a reboot
	plugin.interrupt_maincpu_trigger_trap(loadSnapshotTrap, (void*)&data);
	execC64Frame(); // execute cpu trap
	if(data.hasError)
		return throwFileReadError();
}

void EmuSystem::saveBackupMem(Base::ApplicationContext)
{
	if(gameIsRunning())
	{
		// nothing to do for now
	}
}

bool EmuSystem::vidSysIsPAL() { return sysIsPal(); }

void EmuSystem::closeSystem(Base::ApplicationContext ctx)
{
	if(!gameIsRunning())
	{
		return;
	}
	saveBackupMem(ctx);
	setIntResource("WarpMode", 0);
	plugin.tape_image_detach(1);
	plugin.file_system_detach_disk(8, 0);
	plugin.file_system_detach_disk(9, 0);
	plugin.file_system_detach_disk(10, 0);
	plugin.file_system_detach_disk(11, 0);
	plugin.cartridge_detach_image(-1);
	plugin.machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

static const char *mainROMFilename(ViceSystem system)
{
	switch(system)
	{
		case VICE_SYSTEM_PET: return "kernal-4.901465-22.bin";
		case VICE_SYSTEM_SUPER_CPU: return "scpu64";
		default: return "kernal";
	}
}

static void throwC64FirmwareError(Base::ApplicationContext app)
{
	throw std::runtime_error{
		fmt::format("System files missing, place C64, DRIVES, & PRINTER directories from VICE"
		" in a path below, or set a custom path in options:\n"
		#if defined CONFIG_ENV_LINUX && !defined CONFIG_MACHINE_PANDORA
		"{}\n{}\n{}", EmuApp::assetPath(app), "~/.local/share/C64.emu", "/usr/share/games/vice")};
		#else
		"{}/C64.emu", app.sharedStoragePath())};
		#endif
}

static bool initC64(EmuApp &app)
{
	if(c64IsInit)
		return true;
	if(sysfile_locate(mainROMFilename(currSystem), nullptr) == -1)
	{
		return false;
	}
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

bool EmuApp::willCreateSystem(ViewAttachParams attach, Input::Event e)
{
	if(!c64FailedInit)
		return true;
	pushAndShowNewYesNoAlertView(attach, e,
		"A previous system file load failed, you must restart the app to run any C64 software",
		"Exit Now", "Cancel", [](View &v) { v.appContext().exit(); }, nullptr);
	return false;
}

static FS::FileString vic20ExtraCartName(Base::ApplicationContext ctx, std::string_view baseCartName, std::string_view searchPath)
{
	auto findAddrSuffixOffset =
	[](std::string_view baseCartName) -> uintptr_t
	{
		constexpr std::array<std::string_view, 5> addrSuffixStr
		{
			"-2000.", "-4000.", "-6000.", "-a000.", "-b000."
		};
		for(auto suffixStr : addrSuffixStr)
		{
			if(auto offset = baseCartName.rfind(suffixStr);
				offset != baseCartName.npos)
			{
				return offset;
			}
		}
		return 0;
	};
	auto addrSuffixOffset = findAddrSuffixOffset(baseCartName);
	if(!addrSuffixOffset)
	{
		return {};
	}
	auto cartName = FS::FileString{baseCartName};
	constexpr std::array<char, 5> addrSuffixChar
	{
		'2', '4', '6', 'a', 'b'
	};
	for(auto suffixChar : addrSuffixChar) // looks for a matching file with a valid memory address suffix
	{
		if(suffixChar == baseCartName[addrSuffixOffset])
			continue; // skip original filename
		cartName[addrSuffixOffset] = suffixChar;
		if(ctx.fileUriExists(ctx.fileUri(searchPath, cartName)))
		{
			return cartName;
		}
	}
	return {};
}

void EmuSystem::loadGame(Base::ApplicationContext ctx, IO &, EmuSystemCreateParams params, OnLoadProgressDelegate)
{
	if(!initC64(EmuApp::get(ctx)))
	{
		throwC64FirmwareError(ctx);
	}
	applyInitialOptionResources();
	bool shouldAutostart = !(params.systemFlags & SYSTEM_FLAG_NO_AUTOSTART) && optionAutostartOnLaunch;
	if(shouldAutostart && plugin.autostart_autodetect_)
	{
		logMsg("loading & autostarting:%s", contentLocation().data());
		if(contentFileName().ends_with(".prg"))
		{
			// needed to store AutostartPrgDisk.d64
			makeDefaultBaseSavePath(ctx);
		}
		if(plugin.autostart_autodetect(contentLocation().data(), nullptr, 0, AUTOSTART_MODE_RUN) != 0)
		{
			EmuSystem::throwFileReadError();
		}
	}
	else // no autostart
	{
		if(hasC64DiskExtension(contentFileName()))
		{
			logMsg("loading disk image:%s", contentLocation().data());
			if(plugin.file_system_attach_disk(8, 0, contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
		}
		else if(hasC64TapeExtension(contentFileName()))
		{
			logMsg("loading tape image:%s", contentLocation().data());
			if(plugin.tape_image_attach(1, contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
		}
		else // cart
		{
			logMsg("loading cart image:%s", contentLocation().data());
			if(plugin.cartridge_attach_image(systemCartType(currSystem), contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
			if(currSystem == VICE_SYSTEM_VIC20) // check if the cart is part of a *-x000.prg pair
			{
				auto extraCartFilename = vic20ExtraCartName(ctx, contentFileName(), contentDirectory());
				if(extraCartFilename.size())
				{
					logMsg("loading extra cart image:%s", extraCartFilename.data());
					if(plugin.cartridge_attach_image(systemCartType(currSystem),
						ctx.fileUri(contentDirectory(), extraCartFilename).data()) != 0)
					{
						EmuSystem::throwFileReadError();
					}
				}
			}
		}
		optionAutostartOnLaunch = false;
		sessionOptionSet();
	}
}

static void execC64Frame()
{
	startCanvasRunningFrame();
	// signal C64 thread to execute one frame and wait for it to finish
	execSem.release();
	execDoneSem.acquire();
}

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	audioPtr = audio;
	setCanvasSkipFrame(!video);
	execC64Frame();
	if(video)
	{
		video->startFrameWithFormat(taskCtx, canvasSrcPix);
	}
	audioPtr = {};
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithFormat({}, canvasSrcPix);
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	logMsg("set audio rate %d", rate);
	int mixRate = std::round(rate * (systemFrameRate * frameTime.count()));
	int currRate = 0;
	plugin.resources_get_int("SoundSampleRate", &currRate);
	if(currRate != mixRate)
	{
		setIntResource("SoundSampleRate", mixRate);
	}
}

bool EmuSystem::shouldFastForward()
{
	return *plugin.warp_mode_enabled;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
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

void EmuApp::onMainWindowCreated(ViewAttachParams attach, Input::Event e)
{
	sysFilePath[0] = EmuSystem::firmwarePath();
	plugin.init();
	updateKeyboardMapping();
	setSysModel(optionDefaultModel(currSystem));
	plugin.resources_set_string("AutostartPrgDiskImage",
		attach.appContext().fileUri(EmuSystem::baseDefaultGameSavePath(attach.appContext()), "AutostartPrgDisk.d64").data());
}

void EmuSystem::onPrepareAudio(EmuAudio &audio)
{
	audio.setStereo(false);
}

void EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	pixFmt = fmt;
	if(activeCanvas)
	{
		updateCanvasPixelFormat(activeCanvas, fmt);
	}
}

void EmuSystem::onInit(Base::ApplicationContext ctx)
{
	IG::makeDetachedThread(
		[]()
		{
			execSem.acquire();
			logMsg("starting maincpu_mainloop()");
			plugin.maincpu_mainloop();
		});
	appContext = ctx; // saved for sysfile_* functions

	if constexpr(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
	{
		sysFilePath[1] = ctx.assetPath();
		sysFilePath[2] = FS::pathString(ctx.assetPath(), "C64.emu.zip");
		sysFilePath[3] = "~/.local/share/C64.emu";
		sysFilePath[4] = "/usr/share/games/vice";
	}
	else
	{
		sysFilePath[1] = FS::pathString(ctx.sharedStoragePath(), "C64.emu");
		sysFilePath[2] = FS::pathString(ctx.sharedStoragePath(), "C64.emu.zip");
	}

	// higher quality ReSID sampling modes take orders of magnitude more CPU power,
	// set some reasonable defaults based on CPU type
	#if defined __x86_64__
	optionReSidSampling.initDefault(SID_RESID_SAMPLING_RESAMPLING);
	#elif defined __aarch64__
	optionReSidSampling.initDefault(SID_RESID_SAMPLING_INTERPOLATION);
	#else
	optionReSidSampling.initDefault(SID_RESID_SAMPLING_FAST);
	#endif
}
