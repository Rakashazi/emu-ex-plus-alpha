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
// TODO: Stella includes can clash with PAGE_SHIFT & PAGE_MASK based on order
#undef HAVE_UNISTD_H
// TODO: Some Stella types collide with MacTypes.h
#define BytePtr BytePtrMac
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppInlines.hh>
#undef BytePtr
#undef Debugger
#ifdef Success
#undef Success // conflict with macro in X11 headers
#endif
#include <stella/emucore/Cart.hxx>
#include <stella/emucore/CartDetector.hxx>
#include <stella/emucore/Props.hxx>
#include <stella/emucore/MD5.hxx>
#include <stella/emucore/Sound.hxx>
#include <stella/emucore/SerialPort.hxx>
#include <stella/emucore/tia/TIA.hxx>
#include <stella/emucore/Switches.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Paddles.hxx>
#include <EventHandler.hxx>
#include "SoundGeneric.hh"
#include "internal.hh"

static constexpr uint MAX_ROM_SIZE = 512 * 1024;
std::unique_ptr<OSystem> osystem{};
Properties defaultGameProps{};
bool p1DiffB = true, p2DiffB = true, vcsColor = true;
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2018\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella.sourceforge.net";
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;
EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter =
	[](const char *name)
	{
		return string_hasDotExtension(name, "a26") || string_hasDotExtension(name, "bin");
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

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *savePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.sta", savePath, gameName, saveSlotChar(slot));
}

void EmuSystem::closeSystem()
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
	return osystem->settings().getFloat("framerate") == 50.0;
}

EmuSystem::Error EmuSystem::loadGame(IO &io, OnLoadProgressDelegate)
{
	auto size = io.size();
	if(size > MAX_ROM_SIZE)
	{
		return makeError("ROM size is too large");
	}
	BytePtr image = std::make_unique<uInt8[]>(MAX_ROM_SIZE);
	if(io.read(image.get(), size) != (ssize_t)size)
	{
		return makeFileReadError();
	}
	string md5 = MD5::hash(image, size);
	Properties props;
	osystem->propSet().getMD5(md5, props);
	defaultGameProps = props;
	string romType = props.get(Cartridge_Type);
	auto &settings = osystem->settings();
	settings.setValue("romloadcount", 0);
	auto cartridge = CartDetector::create(image, size, md5, romType, *osystem);
	if((int)optionTVPhosphor != TV_PHOSPHOR_AUTO)
	{
		props.set(Display_Phosphor, optionTVPhosphor ? "YES" : "NO");
	}
	osystem->frameBuffer().enablePhosphor(props.get(Display_Phosphor) == "YES", optionTVPhosphorBlend);
	if((int)optionVideoSystem) // not auto
	{
		logMsg("forcing video system to: %s", optionVideoSystemToStr());
		props.set(Display_Format, optionVideoSystemToStr());
	}
	osystem->makeConsole(cartridge, props);
	auto &console = osystem->console();
	settings.setValue("framerate", (int)console.getFramerate());
	console.initializeVideo();
	console.initializeAudio();
	logMsg("is PAL: %s", EmuSystem::vidSysIsPAL() ? "yes" : "no");
	return {};
}

void EmuSystem::configAudioRate(double frameTime, int rate)
{
	osystem->soundGeneric().setFrameTime(*osystem, rate, frameTime);
}

void EmuSystem::runFrame(EmuSystemTask *task, EmuVideo *video, bool renderAudio)
{
	auto &console = osystem->console();
	console.leftController().update();
	console.rightController().update();
	console.switches().update();
	auto &tia = console.tia();
	tia.update();
	if(video)
	{
		auto img = video->startFrameWithFormat(task, {{(int)tia.width(), (int)tia.height()}, IG::PIXEL_FMT_RGB565});
		osystem->frameBuffer().render(img.pixmap(), tia);
		img.endFrame();
	}
	auto frames = audioFramesForThisFrame();
	Int16 buff[frames * soundChannels];
	uint writtenFrames = osystem->soundGeneric().processAudio(buff, frames);
	if(renderAudio)
		writeSound(buff, writtenFrames);
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
		osystem->console().switches().update();
		TIA& tia = osystem->console().tia();
		tia.update();
		ev.set(Event::ConsoleReset, 0);
	}
}

EmuSystem::Error EmuSystem::saveState(const char *path)
{
	Serializer state(string(path), 0);
	if(!osystem->state().saveState(state))
	{
		return makeFileWriteError();
	}
	return {};
}

EmuSystem::Error EmuSystem::loadState(const char *path)
{
	Serializer state(string(path), 1);
	if(!osystem->state().loadState(state))
	{
		return makeFileReadError();
	}
	updateSwitchValues();
	return {};
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

EmuSystem::Error EmuSystem::onInit()
{
	osystem = make_unique<OSystem>();
	osystem->settings().setValue("framerate", 60); // set to avoid auto-frame calculation
	Paddles::setDigitalSensitivity(5);
	Paddles::setMouseSensitivity(7);
	EmuSystem::pcmFormat.channels = soundChannels;
	EmuSystem::pcmFormat.sample = Audio::SampleFormats::getFromBits(sizeof(Int16)*8);
	return {};
}
