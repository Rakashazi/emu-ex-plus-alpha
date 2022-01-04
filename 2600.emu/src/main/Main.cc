/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "main"
#include <stella/emucore/Cart.hxx>
#include <stella/emucore/CartCreator.hxx>
#include <stella/emucore/Props.hxx>
#include <stella/emucore/MD5.hxx>
#include <stella/emucore/Sound.hxx>
#include <stella/emucore/tia/TIA.hxx>
#include <stella/emucore/Switches.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Paddles.hxx>
#include <stella/emucore/M6532.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/common/AudioSettings.hxx>
#include <OSystem.hxx>
#include <EventHandler.hxx>
#include <SoundEmuEx.hh>
// TODO: Stella includes can clash with PAGE_SHIFT & PAGE_MASK based on order
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#undef Debugger
#include "internal.hh"
#include <imagine/util/format.hh>
#include <imagine/util/string.h>

namespace EmuEx
{

static constexpr uint MAX_ROM_SIZE = 512 * 1024;
IG::ApplicationContext appCtx{};
std::optional<OSystem> osystem{};
Properties defaultGameProps{};
bool p1DiffB = true, p2DiffB = true, vcsColor = true;
Controller::Type autoDetectedInput1{};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2022\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella-emu.github.io";
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
IG::Audio::SampleFormat EmuSystem::audioSampleFormat = IG::Audio::SampleFormats::f32;
EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::stringEndsWithAny(name, ".a26", ".bin", ".A26", ".BIN");
	};
EmuSystem::NameFilterFunc EmuSystem::defaultBenchmarkFsFilter = EmuSystem::defaultFsFilter;

const BundledGameInfo &EmuSystem::bundledGameInfo(uint idx)
{
	static const BundledGameInfo info[]
	{
		{ "Test Game", "game.bin"	}
	};

	return info[0];
}

const char *EmuSystem::shortSystemName()
{
	return "2600";
}

const char *EmuSystem::systemName()
{
	return "Atari 2600";
}

FS::FileString EmuSystem::stateFilename(int slot, std::string_view name)
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotChar(slot));
}

void EmuSystem::closeSystem(IG::ApplicationContext)
{
	osystem->deleteConsole();
}

static void updateSwitchValues()
{
	auto switches = osystem->console().switches().read();
	logMsg("updating switch values to %X", switches);
	p1DiffB = !(switches & 0x40);
	p2DiffB = !(switches & 0x80);
	vcsColor = switches & 0x08;
}

bool EmuSystem::vidSysIsPAL()
{
	return osystem->hasConsole() && osystem->console().timing() != ConsoleTiming::ntsc;
}

void EmuSystem::loadGame(IG::ApplicationContext ctx, IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	auto &os = *osystem;
	if(io.size() > MAX_ROM_SIZE)
		throw std::runtime_error{"ROM size is too large"};
	auto image = std::make_unique<uInt8[]>(MAX_ROM_SIZE);
	auto size = io.read(image.get(), MAX_ROM_SIZE);
	if(size == -1)
	{
		throwFileReadError();
	}
	string md5 = MD5::hash(image, size);
	Properties props{};
	os.propSet().getMD5(md5, props);
	defaultGameProps = props;
	auto &romType = props.get(PropType::Cart_Type);
	FilesystemNode fsNode{contentFileName().data()};
	auto &settings = os.settings();
	settings.setValue("romloadcount", 0);
	auto cartridge = CartCreator::create(fsNode, image, size, md5, romType, settings);
	if((int)optionTVPhosphor != TV_PHOSPHOR_AUTO)
	{
		props.set(PropType::Display_Phosphor, optionTVPhosphor ? "YES" : "NO");
	}
	os.frameBuffer().enablePhosphor(props.get(PropType::Display_Phosphor) == "YES", optionTVPhosphorBlend);
	if((int)optionVideoSystem) // not auto
	{
		logMsg("forcing video system to: %s", optionVideoSystemToStr());
		props.set(PropType::Display_Format, optionVideoSystemToStr());
	}
	os.makeConsole(cartridge, props, contentFileName().data());
	auto &console = os.console();
	autoDetectedInput1 = limitToSupportedControllerTypes(console.leftController().type());
	setControllerType(EmuApp::get(ctx), console, (Controller::Type)optionInputPort1.val);
	Paddles::setDigitalSensitivity(optionPaddleDigitalSensitivity);
	console.initializeVideo();
	console.initializeAudio();
	logMsg("is PAL: %s", EmuSystem::vidSysIsPAL() ? "yes" : "no");
}

void EmuSystem::configAudioRate(IG::FloatSeconds frameTime, uint32_t rate)
{
	osystem->setFrameTime(frameTime.count(), rate);
}

static void renderVideo(EmuSystemTaskContext taskCtx, EmuVideo &video, FrameBuffer &fb, TIA &tia)
{
	auto fmt = video.renderPixelFormat();
	auto img = video.startFrameWithFormat(taskCtx, {{(int)tia.width(), (int)tia.height()}, fmt});
	fb.render(img.pixmap(), tia);
	img.endFrame();
}

void EmuSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	auto &os = *osystem;
	auto &console = os.console();
	console.leftController().update();
	console.rightController().update();
	console.switches().update();
	console.riot().update();
	auto &tia = console.tia();
	tia.update(0xFFFFFFFF);
	tia.renderToFrameBuffer();
	if(video)
	{
		renderVideo(taskCtx, *video, os.frameBuffer(), tia);
	}
	os.processAudio(audio);
}

void EmuSystem::renderFramebuffer(EmuVideo &video)
{
	auto &tia = osystem->console().tia();
	auto &fb = osystem->frameBuffer();
	renderVideo({}, video, fb, tia);
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
	{
		osystem->console().system().reset();
	}
	else
	{
		Event &ev = osystem->eventHandler().event();
		ev.clear();
		ev.set(Event::ConsoleReset, 1);
		auto &console = osystem->console();
		console.switches().update();
		TIA& tia = console.tia();
		tia.update(console.emulationTiming().cyclesPerFrame());
		ev.set(Event::ConsoleReset, 0);
	}
}

void EmuSystem::saveState(IG::CStringView path)
{
	Serializer state{path.data()};
	if(!osystem->state().saveState(state))
	{
		throwFileWriteError();
	}
}

void EmuSystem::loadState(IG::CStringView path)
{
	Serializer state{path.data(), Serializer::Mode::ReadOnly};
	if(!osystem->state().loadState(state))
	{
		throwFileReadError();
	}
	updateSwitchValues();
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, Gfx::VertexColorPixelFormat.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, Gfx::VertexColorPixelFormat.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, Gfx::VertexColorPixelFormat.build((75./255.) * .4, (37.5/255.) * .4, (0./255.) * .4, 1.) },
		{ 1., Gfx::VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};
	view.setBackgroundGradient(navViewGrad);
}

void EmuSystem::onPrepareAudio(EmuAudio &audio)
{
	audio.setStereo(false); // TODO: stereo mode
}

void EmuSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	osystem->frameBuffer().setPixelFormat(fmt);
	if(osystem->hasConsole())
	{
		osystem->frameBuffer().paletteHandler().setPalette(osystem->settings().getString("palette"));
	}
}

void EmuSystem::onInit(IG::ApplicationContext ctx)
{
	appCtx = ctx;
	auto &app = EmuApp::get(ctx);
	osystem.emplace(app);
	Paddles::setDigitalSensitivity(5);
	Paddles::setMouseSensitivity(7);
}

}
