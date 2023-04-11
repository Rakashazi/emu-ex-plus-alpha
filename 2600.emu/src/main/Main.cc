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
#include <stella/emucore/M6532.hxx>
#include <stella/emucore/DispatchResult.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/common/AudioSettings.hxx>
#include <OSystem.hxx>
#include <EventHandler.hxx>
#include <SoundEmuEx.hh>
// TODO: Stella includes can clash with PAGE_SHIFT & PAGE_MASK based on order
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#undef Debugger
#include <imagine/util/format.hh>
#include <imagine/util/string.h>

namespace EmuEx
{

constexpr size_t MAX_ROM_SIZE = 512 * 1024;
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2023\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella-emu.github.io";
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
IG::Audio::SampleFormat EmuSystem::audioSampleFormat = IG::Audio::SampleFormats::f32;
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](std::string_view name)
	{
		return IG::endsWithAnyCaseless(name, ".a26", ".bin");
	};

const char *EmuSystem::shortSystemName() const
{
	return "2600";
}

const char *EmuSystem::systemName() const
{
	return "Atari 2600";
}

FS::FileString A2600System::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}.sta", name, saveSlotChar(slot));
}

void A2600System::closeSystem()
{
	osystem.deleteConsole();
}

void A2600System::updateSwitchValues()
{
	auto switches = osystem.console().switches().read();
	logMsg("updating switch values to %X", switches);
	p1DiffB = !(switches & 0x40);
	p2DiffB = !(switches & 0x80);
	vcsColor = switches & 0x08;
}

VideoSystem A2600System::videoSystem() const
{
	return osystem.hasConsole()
		&& osystem.console().timing() != ConsoleTiming::ntsc ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC;
}

void A2600System::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	auto &os = osystem;
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
	settings.setValue("plr.tv.jitter", false);
	auto cartridge = CartCreator::create(fsNode, image, size, md5, romType, settings);
	cartridge->setMessageCallback([](const string& msg){ logMsg("%s", msg.c_str()); });
	if((int)optionTVPhosphor != TV_PHOSPHOR_AUTO)
	{
		props.set(PropType::Display_Phosphor, optionTVPhosphor ? "YES" : "NO");
	}
	os.frameBuffer().enablePhosphor(props.get(PropType::Display_Phosphor) == "YES", optionTVPhosphorBlend);
	if((int)optionVideoSystem) // not auto
	{
		logMsg("forcing video system to:%s", optionVideoSystemToStr(optionVideoSystem));
		props.set(PropType::Display_Format, optionVideoSystemToStr(optionVideoSystem));
	}
	os.makeConsole(cartridge, props, contentFileName().data());
	auto &console = os.console();
	autoDetectedInput1 = limitToSupportedControllerTypes(console.leftController().type());
	setControllerType(EmuApp::get(appContext()), console, Controller::Type(optionInputPort1.val));
	Paddles::setDigitalSensitivity(optionPaddleDigitalSensitivity);
	console.initializeVideo();
	console.initializeAudio();
	logMsg("is PAL: %s", videoSystem() == VideoSystem::PAL ? "yes" : "no");
}

static auto consoleFrameRate(const OSystem &osystem)
{
	if(!osystem.hasConsole())
		return 60.f;
	if(!osystem.console().tia().frameBufferScanlinesLastFrame())
		return osystem.console().timing() == ConsoleTiming::ntsc ? 60.f : 50.f;
	return osystem.console().currentFrameRate();
}

FloatSeconds A2600System::frameTime() const
{
	return FloatSeconds{1. / consoleFrameRate(osystem)};
}

void A2600System::configAudioRate(FloatSeconds outputFrameTime, int outputRate)
{
	if(!osystem.hasConsole())
		return;
	configuredInputVideoFrameRate = consoleFrameRate(osystem);
	osystem.setSoundMixRate(std::round(audioMixRate(outputRate, configuredInputVideoFrameRate, outputFrameTime)),
		AudioSettings::ResamplingQuality(optionAudioResampleQuality.val));
}

static void renderVideo(EmuSystemTaskContext taskCtx, EmuVideo &video, FrameBuffer &fb, TIA &tia)
{
	auto fmt = video.renderPixelFormat();
	auto img = video.startFrameWithFormat(taskCtx, {{(int)tia.width(), (int)tia.height()}, fmt});
	fb.render(img.pixmap(), tia);
	img.endFrame();
}

void A2600System::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	auto &os = osystem;
	auto &console = os.console();
	auto &sound = os.soundEmuEx();
	sound.setEmuAudio(audio);
	console.riot().update();
	auto &tia = console.tia();
	static constexpr uInt64 maxCyclesPerFrame = 32768;
	DispatchResult res;
	tia.update(res, maxCyclesPerFrame);
	if(res.getCycles() > maxCyclesPerFrame)
		logWarn("frame ran %u cycles", (unsigned)res.getCycles());
	tia.renderToFrameBuffer();
	if(video)
	{
		renderVideo(taskCtx, *video, os.frameBuffer(), tia);
	}
	if(auto newInputVideoFrameRate = osystem.console().currentFrameRate();
		configuredInputVideoFrameRate != newInputVideoFrameRate
		&& newInputVideoFrameRate >= 40.0 && newInputVideoFrameRate <= 70.0) [[unlikely]]
	{
		onFrameTimeChanged();
	}
}

void A2600System::renderFramebuffer(EmuVideo &video)
{
	auto &tia = osystem.console().tia();
	auto &fb = osystem.frameBuffer();
	renderVideo({}, video, fb, tia);
}

void A2600System::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	if(mode == ResetMode::HARD)
	{
		osystem.console().system().reset();
	}
	else
	{
		Event &ev = osystem.eventHandler().event();
		ev.clear();
		ev.set(Event::ConsoleReset, 1);
		auto &console = osystem.console();
		console.switches().update();
		TIA& tia = console.tia();
		tia.update(console.emulationTiming().cyclesPerFrame());
		ev.set(Event::ConsoleReset, 0);
	}
}

void A2600System::saveState(IG::CStringView path)
{
	Serializer state{path.data()};
	if(!osystem.state().saveState(state))
	{
		throwFileWriteError();
	}
}

void A2600System::loadState(EmuApp &, IG::CStringView path)
{
	Serializer state{path.data(), Serializer::Mode::ReadOnly};
	if(!osystem.state().loadState(state))
	{
		throwFileReadError();
	}
	updateSwitchValues();
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((75./255.) * .4, (37.5/255.) * .4, (0./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

bool A2600System::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	osystem.frameBuffer().setPixelFormat(fmt);
	if(osystem.hasConsole())
	{
		osystem.frameBuffer().paletteHandler().setPalette();
	}
	return false;
}

}
