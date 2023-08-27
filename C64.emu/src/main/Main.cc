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
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/gui/AlertView.hh>
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

namespace EmuEx
{

const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2013-2023\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nvice-emu.sourceforge.io";
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::handlesGenericIO = false;
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;

const char *EmuSystem::shortSystemName() const
{
	return "C64";
}

const char *EmuSystem::systemName() const
{
	return "Commodore 64";
}

void C64System::setModel(int model)
{
	logMsg("setting model id:%d", model);
	plugin.model_set(model);
}

int C64System::sysModel() const
{
	return plugin.model_get();
}

void C64System::setSysModel(int model)
{
	setModel(model);
}

const char *C64System::videoChipStr() const
{
	switch(currSystem)
	{
		case ViceSystem::VIC20:
		return "VIC";

		case ViceSystem::C64:
		case ViceSystem::C64SC:
		case ViceSystem::SUPER_CPU:
		case ViceSystem::C64DTV:
		case ViceSystem::C128:
		case ViceSystem::CBM5X0:
		return "VICII";

		case ViceSystem::PLUS4:
		return "TED";

		case ViceSystem::PET:
		case ViceSystem::CBM2:
		return "Crtc";
	}
	return "";
}

bool C64System::currSystemIsC64() const
{
	switch(currSystem)
	{
		case ViceSystem::C64:
		case ViceSystem::C64SC:
		case ViceSystem::SUPER_CPU:
		case ViceSystem::C64DTV:
			return true;
		default:
			return false;
	}
}

bool C64System::currSystemIsC64Or128() const
{
	return currSystemIsC64() || currSystem == ViceSystem::C128;
}

void C64System::setRuntimeReuSize(int size)
{
	 // REU may be in use mid-frame so use a trap & wait 2 frames
	struct ReuTrapData
	{
		C64System &sys;
		int size;
	};
	ReuTrapData reuData{*this, size};
	plugin.interrupt_maincpu_trigger_trap(
		[](uint16_t, void *data)
		{
			auto &reuData = *(ReuTrapData*)data;
			if(reuData.size)
			{
				logMsg("enabling REU size:%d", reuData.size);
				reuData.sys.setIntResource("REUsize", reuData.size);
				reuData.sys.setIntResource("REU", 1);
			}
			else
			{
				logMsg("disabling REU");
				reuData.sys.setIntResource("REU", 0);
			}
		}, (void*)&reuData);
	execC64Frame();
	execC64Frame();
}

void C64System::applyInitialOptionResources()
{
	setIntResource("JoyPort1Device", JOYPORT_ID_JOYSTICK);
	setIntResource("JoyPort2Device", JOYPORT_ID_JOYSTICK);
	applySessionOptions();
	setBorderMode(optionBorderMode);
	setSidEngine(optionSidEngine);
	setReSidSampling(optionReSidSampling);
}

int systemCartType(ViceSystem system)
{
	switch(system)
	{
		case ViceSystem::CBM2:
		case ViceSystem::CBM5X0:
			return CARTRIDGE_CBM2_8KB_1000;
		case ViceSystem::PLUS4:
			return CARTRIDGE_PLUS4_DETECT;
		case ViceSystem::VIC20:
			return CARTRIDGE_VIC20_DETECT;
		default:
			return CARTRIDGE_CRT;
	}
}

bool hasC64DiskExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name,
		".d64", ".d67", ".d71", ".d80", ".d81", ".d82", ".d1m", ".d2m", ".d4m", ".g64", ".p64", ".g41", ".x64", ".dsk");
}

bool hasC64TapeExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".t64", ".tap");
}

bool hasC64CartExtension(std::string_view name)
{
	return endsWithAnyCaseless(name, ".bin", ".crt",
		".20", ".40", ".60", ".70", ".a0", ".b0"); // VIC-20 headerless carts
}

static bool hasC64Extension(std::string_view name)
{
	return hasC64DiskExtension(name) ||
			hasC64TapeExtension(name) ||
			hasC64CartExtension(name) ||
			IG::endsWithAnyCaseless(name, ".prg", ".p00");
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasC64Extension;

void C64System::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	plugin.machine_trigger_reset(mode == ResetMode::HARD ? MACHINE_RESET_MODE_HARD : MACHINE_RESET_MODE_SOFT);
}

FS::FileString C64System::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.{}.vsf", name, saveSlotChar(slot));
}

struct SnapshotTrapData
{
	VicePlugin &plugin;
	const char *pathStr{};
	bool hasError = true;
};

static void loadSnapshotTrap(uint16_t, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("loading state: %s", snapData->pathStr);
	if(snapData->plugin.machine_read_snapshot(snapData->pathStr, 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

static void saveSnapshotTrap(uint16_t, void *data)
{
	auto snapData = (SnapshotTrapData*)data;
	logMsg("saving state: %s", snapData->pathStr);
	if(snapData->plugin.machine_write_snapshot(snapData->pathStr, 1, 1, 0) < 0)
		snapData->hasError = true;
	else
		snapData->hasError = false;
}

void C64System::saveState(IG::CStringView path)
{
	SnapshotTrapData data{.plugin{plugin}, .pathStr{path}};
	plugin.interrupt_maincpu_trigger_trap(saveSnapshotTrap, (void*)&data);
	execC64Frame(); // execute cpu trap
	if(data.hasError)
		throwFileWriteError();
}

void C64System::loadState(EmuApp &, IG::CStringView path)
{
	plugin.vsync_set_warp_mode(0);
	SnapshotTrapData data{.plugin{plugin}, .pathStr{path}};
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

VideoSystem C64System::videoSystem() const
{
	switch(intResource("MachineVideoStandard"))
	{
		case MACHINE_SYNC_PAL:
		case MACHINE_SYNC_PALN:
			return VideoSystem::PAL;
		default: return VideoSystem::NATIVE_NTSC;
	}
}

void C64System::closeSystem()
{
	if(!hasContent())
	{
		return;
	}
	plugin.vsync_set_warp_mode(0);
	if(intResource("REU"))
	{
		setRuntimeReuSize(0);
	}
	plugin.tape_image_detach(1);
	plugin.file_system_detach_disk(8, 0);
	plugin.file_system_detach_disk(9, 0);
	plugin.file_system_detach_disk(10, 0);
	plugin.file_system_detach_disk(11, 0);
	plugin.cartridge_detach_image(-1);
	plugin.machine_trigger_reset(MACHINE_RESET_MODE_HARD);
}

static bool hasSysFilePath(ApplicationContext ctx, const auto &paths)
{
	for(const auto &path : paths)
	{
		if(!path.empty() && !ctx.fileUriDisplayName(path).empty())
			return true;
	}
	return false;
}

void C64System::initC64(EmuApp &app)
{
	if(c64IsInit)
		return;
	if(!hasSysFilePath(appContext(), sysFilePath))
		throw std::runtime_error{"Missing system file path, please check Options➔File Paths➔VICE System Files"};
	logMsg("initializing C64");
  if(plugin.init_main() < 0)
  {
  	logErr("error in init_main()");
  	c64FailedInit = true;
  	throw std::runtime_error{std::format("Missing system file {}, please check Options➔File Paths➔VICE System Files", lastMissingSysFile)};
	}
	c64IsInit = true;
}

bool C64App::willCreateSystem(ViewAttachParams attach, const Input::Event &e)
{
	if(!system().c64FailedInit)
		return true;
	pushAndShowModalView(std::make_unique<YesNoAlertView>(attach,
		std::format("A system file {} failed loading, you must restart the app and try again after verifying the file", system().lastMissingSysFile),
		"Exit Now", "Cancel", YesNoAlertView::Delegates{ .onYes = [](View &v) { v.appContext().exit(); } }), e);
	return false;
}

static FS::PathString vic20ExtraCartPath(ApplicationContext ctx, std::string_view baseCartName, std::string_view searchPath)
{
	auto findAddrSuffixOffset = [](std::string_view baseCartName) -> uintptr_t
	{
		for(auto suffixStr : std::array{
			"-2000.", "-4000.", "-6000.", "-a000.", "-b000.",
			"[2000]", "[4000]", "[6000]", "[A000]", "[B000]", // TOSEC names
			".20", ".40", ".60", ".a0", ".b0"})
		{
			if(auto offset = baseCartName.rfind(suffixStr);
				offset != baseCartName.npos)
			{
				return offset + 1; // skip '-', '[', or '.'
			}
		}
		return 0;
	};
	auto addrSuffixOffset = findAddrSuffixOffset(baseCartName);
	if(!addrSuffixOffset)
	{
		return {};
	}
	const auto &addrSuffixChar = baseCartName[addrSuffixOffset];
	bool addrCharIsUpper = addrSuffixChar == 'A' || addrSuffixChar == 'B';
	for(auto c : std::array{'2', '4', '6', 'a', 'b'}) // looks for a matching file with a valid memory address suffix
	{
		if(c == tolower(addrSuffixChar))
			continue; // skip original filename
		FS::FileString cartName{baseCartName};
		cartName[addrSuffixOffset] = addrCharIsUpper ? toupper(c) : c;
		auto cartPath = FS::uriString(searchPath, cartName);
		if(ctx.fileUriExists(cartPath))
		{
			return cartPath;
		}
	}
	return {};
}

void C64System::tryLoadingSplitVic20Cart()
{
	if(!contentDirectory().size())
		return;
	auto extraCartPath = vic20ExtraCartPath(appContext(), contentFileName(), contentDirectory());
	if(extraCartPath.size())
	{
		logMsg("loading extra cart image:%s", extraCartPath.data());
		if(plugin.cartridge_attach_image(CARTRIDGE_VIC20_DETECT, extraCartPath.data()) != 0)
		{
			EmuSystem::throwFileReadError();
		}
	}
}

void C64System::loadContent(IO &, EmuSystemCreateParams params, OnLoadProgressDelegate)
{
	initC64(EmuApp::get(appContext()));
	applyInitialOptionResources();
	bool shouldAutostart = !(params.systemFlags & SYSTEM_FLAG_NO_AUTOSTART) && optionAutostartOnLaunch;
	if(shouldAutostart && plugin.autostart_autodetect_)
	{
		logMsg("loading & autostarting:%s", contentLocation().data());
		if(endsWithAnyCaseless(contentFileName(), ".prg"))
		{
			// needed to store AutostartPrgDisk.d64
			fallbackSaveDirectory(true);
		}
		if(currSystem == ViceSystem::VIC20 && hasC64CartExtension(contentFileName()))
		{
			if(plugin.cartridge_attach_image(CARTRIDGE_VIC20_DETECT, contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
			tryLoadingSplitVic20Cart();
		}
		else if(plugin.autostart_autodetect(contentLocation().data(), nullptr, 0, AUTOSTART_MODE_RUN) != 0)
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
			if(currSystem == ViceSystem::VIC20)
			{
				tryLoadingSplitVic20Cart();
			}
		}
		optionAutostartOnLaunch = false;
		sessionOptionSet();
	}
}

void C64System::execC64Frame()
{
	startCanvasRunningFrame();
	// signal C64 thread to execute one frame and wait for it to finish
	execSem.release();
	execDoneSem.acquire();
}

void C64System::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	audioPtr = audio;
	setCanvasSkipFrame(!video);
	execC64Frame();
	if(video)
	{
		video->startFrameWithAltFormat(taskCtx, canvasSrcPix);
	}
	audioPtr = {};
}

void C64System::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithAltFormat({}, canvasSrcPix);
}

void C64System::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	int mixRate = std::round(audioMixRate(outputRate, systemFrameRate, outputFrameTime));
	int currRate = 0;
	plugin.resources_get_int("SoundSampleRate", &currRate);
	if(currRate == mixRate)
		return;
	logMsg("set sound mix rate:%d", mixRate);
	setIntResource("SoundSampleRate", mixRate);
}

bool C64System::shouldFastForward() const
{
	return *plugin.warp_mode_enabled;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build(48./255., 36./255., 144./255., 1.) },
		{ .3, Gfx::PackedColor::format.build(48./255., 36./255., 144./255., 1.) },
		{ .97, Gfx::PackedColor::format.build((48./255.) * .4, (36./255.) * .4, (144./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

static FS::PathString autostartPrgDiskImagePath(EmuSystem &sys)
{
	return FS::pathString(sys.fallbackSaveDirectory(), "AutostartPrgDisk.d64");
}

void EmuApp::onMainWindowCreated(ViewAttachParams attach, const Input::Event &e)
{
	auto &sys = static_cast<C64System&>(system());
	sys.plugin.init();
	inputManager.updateKeyboardMapping();
	sys.setSysModel(sys.optionDefaultModel);
	auto ctx = attach.appContext();
	auto prgDiskPath = autostartPrgDiskImagePath(system());
	FS::remove(prgDiskPath);
	sys.plugin.resources_set_string("AutostartPrgDiskImage", prgDiskPath.data());
}

bool C64System::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	if(fmt == IG::PIXEL_FMT_RGB565)
		fmt = IG::PIXEL_FMT_RGBA8888; // internally render in 32bpp
	pixFmt = fmt;
	if(activeCanvas)
	{
		updateCanvasPixelFormat(activeCanvas, fmt);
	}
	return false;
}

}
