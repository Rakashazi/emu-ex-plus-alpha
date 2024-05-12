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
#include <imagine/util/span.hh>
#include <sys/time.h>
#include <imagine/logger/logger.h>

extern "C"
{
	#include "machine.h"
	#include "maincpu.h"
	#include "drive.h"
	#include "lib.h"
	#include "util.h"
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

constexpr SystemLogger log{"C64.emu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2013-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nVice Team\nvice-emu.sourceforge.io";
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
bool EmuSystem::handlesGenericIO = false;
bool EmuSystem::hasRectangularPixels = true;
bool EmuSystem::stateSizeChangesAtRuntime = true;
bool EmuApp::needsGlobalInstance = true;

C64App::C64App(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, c64System{ctx}
{
	audio.setStereo(false);
}

C64System::C64System(ApplicationContext ctx):
	EmuSystem{ctx}
{
	makeDetachedThread(
		[this]()
		{
			emuThreadId = thisThreadId();
			execSem.acquire();
			log.info("starting maincpu_mainloop()");
			plugin.maincpu_mainloop();
		});

	if(sysFilePath.size() == 3)
	{
		sysFilePath[1] = "~/.local/share/C64.emu";
		sysFilePath[2] = "/usr/share/games/vice";
	}
}

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
	log.info("setting model id:{}", model);
	enterCPUTrap();
	plugin.model_set(model);
}

int C64System::model() const
{
	return plugin.model_get();
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
	plugin.machine_trigger_reset(mode == ResetMode::HARD ? MACHINE_RESET_MODE_POWER_CYCLE : MACHINE_RESET_MODE_RESET_CPU);
}

FS::FileString C64System::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.{}.vsf", name, saveSlotChar(slot));
}

void C64System::enterCPUTrap()
{
	assert(emuThreadId);
	if(inCPUTrap)
		return;
	plugin.interrupt_maincpu_trigger_trap([](uint16_t, void *data)
	{
		auto &sys = *((C64System*)data);
		sys.inCPUTrap = true;
		sys.signalEmuTaskThreadAndWait();
		sys.inCPUTrap = false;
	}, (void*)this);
	while(!inCPUTrap)
	{
		signalViceThreadAndWait();
	}
}

struct SnapshotData
{
	uint8_t *buffData{};
	size_t buffSize{};
};

static std::array<char, 32> snapshotVPath(SnapshotData &data)
{
	std::array<char, 32> fileStr;
	// Encode the data/size pointers after the string null terminator
	if(data.buffData)
	{
		auto it = std::ranges::copy(":::B", std::begin(fileStr)).out;
		it = std::ranges::copy(addressAsBytes(data.buffSize), it).out;
		std::ranges::copy(asBytes(data.buffData), it);
		//log.info("encoded size ptr:{} data ptr:{}", (void*)&data.buffSize, (void*)data.buffData);
	}
	else
	{
		auto it = std::ranges::copy(":::N", std::begin(fileStr)).out;
		std::ranges::copy(addressAsBytes(data.buffSize), it);
		//log.info("encoded size ptr:{}", (void*)&data.buffSize);
	}
	return fileStr;
}

static bool saveSnapshot(auto &plugin, SnapshotData &snapData)
{
	//log.info("saving state at:{} size:{}", (void*)snapData.buffData, snapData.buffSize);
	if(auto err = plugin.machine_write_snapshot(snapshotVPath(snapData).data(), 1, 1, 0);
		err < 0)
	{
		log.error("error writing snapshot:{}", err);
		return false;
	}
	return true;
}

static bool loadSnapshot(auto &plugin, SnapshotData &snapData)
{
	assumeExpr(snapData.buffData);
	log.info("loading state at:{} size:{}", (void*)snapData.buffData, snapData.buffSize);
	if(plugin.machine_read_snapshot(snapshotVPath(snapData).data(), 0) < 0)
		return false;
	return true;
}

size_t C64System::stateSize()
{
	enterCPUTrap();
	SnapshotData data{};
	saveSnapshot(plugin, data);
	return data.buffSize;
}

void C64System::readState(EmuApp &app, std::span<uint8_t> buff)
{
	signalViceThreadAndWait();
	enterCPUTrap();
	plugin.vsync_set_warp_mode(0);
	SnapshotData data{.buffData = buff.data(), .buffSize = buff.size()};
	if(!loadSnapshot(plugin, data))
		throw std::runtime_error("Invalid state data");
	// reload snapshot in case last load caused a reboot due to model change
	signalViceThreadAndWait();
	enterCPUTrap();
	if(!loadSnapshot(plugin, data))
		throw std::runtime_error("Invalid state data");
	updateJoystickDevices();
}

size_t C64System::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	enterCPUTrap();
	SnapshotData data{.buffData = buff.data(), .buffSize = buff.size()};
	if(!saveSnapshot(plugin, data))
		return 0;
	return data.buffSize;
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
	enterCPUTrap();
	plugin.machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
	plugin.vsync_set_warp_mode(0);
	plugin.tape_image_detach(1);
	plugin.file_system_detach_disk(8, 0);
	plugin.file_system_detach_disk(9, 0);
	plugin.file_system_detach_disk(10, 0);
	plugin.file_system_detach_disk(11, 0);
	plugin.cartridge_detach_image(-1);
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
	log.info("initializing C64");
  if(plugin.init_main() < 0)
  {
  	log.error("error in init_main()");
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
		if(c == toLower(addrSuffixChar))
			continue; // skip original filename
		FS::FileString cartName{baseCartName};
		cartName[addrSuffixOffset] = addrCharIsUpper ? toUpper(c) : c;
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
		log.info("loading extra cart image:{}", extraCartPath);
		if(plugin.cartridge_attach_image(CARTRIDGE_VIC20_DETECT, extraCartPath.data()) != 0)
		{
			EmuSystem::throwFileReadError();
		}
	}
}

void C64System::loadContent(IO &, EmuSystemCreateParams params, OnLoadProgressDelegate)
{
	bool shouldAutostart = !(params.systemFlags & SYSTEM_FLAG_NO_AUTOSTART) && optionAutostartOnLaunch;
	if(shouldAutostart && plugin.autostart_autodetect_)
	{
		log.info("loading & autostarting:{}", contentLocation());
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
			log.info("loading disk image:{}", contentLocation());
			if(plugin.file_system_attach_disk(8, 0, contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
		}
		else if(hasC64TapeExtension(contentFileName()))
		{
			log.info("loading tape image:{}", contentLocation());
			if(plugin.tape_image_attach(1, contentLocation().data()) != 0)
			{
				EmuSystem::throwFileReadError();
			}
		}
		else // cart
		{
			log.info("loading cart image:{}", contentLocation());
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

void C64System::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	audioPtr = audio;
	setCanvasSkipFrame(!video);
	signalViceThreadAndWait();
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
	int currRate = intResource("SoundSampleRate");
	if(currRate == mixRate)
		return;
	log.info("set sound mix rate:{}", mixRate);
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

void EmuApp::onMainWindowCreated(ViewAttachParams attach, const Input::Event &e)
{
	inputManager.updateKeyboardMapping();
}

bool C64System::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	if(fmt == IG::PixelFmtRGB565)
		fmt = IG::PixelFmtRGBA8888; // internally render in 32bpp
	pixFmt = fmt;
	if(activeCanvas)
	{
		updateCanvasPixelFormat(activeCanvas, fmt);
	}
	return false;
}

}
