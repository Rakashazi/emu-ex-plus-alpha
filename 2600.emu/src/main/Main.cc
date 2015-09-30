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
#include <stella/emucore/Console.hxx>
#include <stella/emucore/Cart.hxx>
#include <stella/emucore/Props.hxx>
#include <stella/emucore/MD5.hxx>
#include <stella/emucore/Sound.hxx>
#include <stella/emucore/SerialPort.hxx>
#include <stella/emucore/TIA.hxx>
#include <stella/emucore/Switches.hxx>
#include <stella/emucore/StateManager.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Paddles.hxx>
#include "SoundGeneric.hh"
#include <emuframework/EmuSystem.hh>
#include <emuframework/CommonFrameworkIncludes.hh>
#include <emuframework/CommonGui.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2014\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella.sourceforge.net";
static constexpr uint MAX_ROM_SIZE = 512 * 1024;
extern OSystem osystem;
static StateManager stateManager{osystem};
Properties defaultGameProps{};
bool p1DiffB = 1, p2DiffB = 1, vcsColor = 1;
static const uint vidBufferX = 160, vidBufferY = 320;
alignas(8) static uInt16 pixBuff[vidBufferX*vidBufferY]{};
const char *EmuSystem::inputFaceBtnName = "JS Buttons";
const char *EmuSystem::inputCenterBtnName = "Select/Reset";
const uint EmuSystem::inputFaceBtns = 2;
const uint EmuSystem::inputCenterBtns = 2;
const bool EmuSystem::inputHasTriggerBtns = false;
const bool EmuSystem::inputHasRevBtnLayout = false;
const uint EmuSystem::maxPlayers = 2;
const char *EmuSystem::configFilename = "2600emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = sizeofArray(EmuSystem::aspectRatioInfo);
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::hasResetModes = true;

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

void EmuSystem::initOptions() {}

void EmuSystem::onOptionsLoaded() {}

enum
{
	vcsKeyIdxUp = EmuControls::systemKeyMapStart,
	vcsKeyIdxRight,
	vcsKeyIdxDown,
	vcsKeyIdxLeft,
	vcsKeyIdxLeftUp,
	vcsKeyIdxRightUp,
	vcsKeyIdxRightDown,
	vcsKeyIdxLeftDown,
	vcsKeyIdxJSBtn,
	vcsKeyIdxJSBtnTurbo,

	vcsKeyIdxUp2,
	vcsKeyIdxRight2,
	vcsKeyIdxDown2,
	vcsKeyIdxLeft2,
	vcsKeyIdxLeftUp2,
	vcsKeyIdxRightUp2,
	vcsKeyIdxRightDown2,
	vcsKeyIdxLeftDown2,
	vcsKeyIdxJSBtn2,
	vcsKeyIdxJSBtnTurbo2,

	vcsKeyIdxSelect,
	vcsKeyIdxReset,
	vcsKeyIdxP1Diff,
	vcsKeyIdxP2Diff,
	vcsKeyIdxColorBW,
	vcsKeyIdxKeyboard1Base,
	vcsKeyIdxKeyboard2Base = vcsKeyIdxKeyboard1Base + 12,
};

enum
{
	CFGKEY_2600_TV_PHOSPHOR = 270, CFGKEY_VIDEO_SYSTEM = 271,
};

Byte1Option optionTVPhosphor(CFGKEY_2600_TV_PHOSPHOR, TV_PHOSPHOR_AUTO);
Byte1Option optionVideoSystem(CFGKEY_VIDEO_SYSTEM, 0, 0, optionIsValidWithMax<6>);

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_2600_TV_PHOSPHOR: optionTVPhosphor.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
	}
	return 1;
}

static const char *optionVideoSystemToStr()
{
	switch((int)optionVideoSystem)
	{
		case 1: return "NTSC";
		case 2: return "PAL";
		case 3: return "SECAM";
		case 4: return "NTSC50";
		case 5: return "PAL60";
		case 6: return "SECAM60";
		default: return "AUTO";
	}
}

void EmuSystem::writeConfig(IO &io)
{
	optionTVPhosphor.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
}

static bool hasVCSRomExtension(const char *name)
{
	return string_hasDotExtension(name, "a26") || string_hasDotExtension(name, "bin");
}

EmuNameFilterFunc EmuFilePicker::defaultFsFilter = hasVCSRomExtension;
EmuNameFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = hasVCSRomExtension;

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

FS::PathString EmuSystem::sprintStateFilename(int slot, const char *savePath, const char *gameName)
{
	return FS::makePathStringPrintf("%s/%s.0%c.sta", savePath, gameName, saveSlotChar(slot));
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		auto saveStr = sprintStateFilename(-1);
		logMsg("saving autosave-state %s", saveStr.data());
		fixFilePermissions(saveStr);
		Serializer state(string(saveStr.data()), 0);
		if(!stateManager.saveState(state))
		{
			logMsg("failed");
		}
	}
}

void EmuSystem::saveBackupMem() { }

void EmuSystem::closeSystem()
{
	osystem.deleteConsole();
}

static void updateSwitchValues()
{
	//assert(osystem.myConsole);
	auto switches = osystem.console().switches().read();
	logMsg("updating switch values to %X", switches);
	p1DiffB = !(switches & 0x40);
	p2DiffB = !(switches & 0x80);
	vcsColor = switches & 0x08;
}

bool EmuSystem::vidSysIsPAL()
{
	return osystem.settings().getFloat("framerate") == 50.0;
}
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

static int loadGameCommon(const uint8 *buff, uint size)
{
	string md5 = MD5(buff, size);
	Properties props;
	osystem.propSet().getMD5(md5, props);
	defaultGameProps = props;
	string romType = props.get(Cartridge_Type);
	string cartId;
	auto &settings = osystem.settings();
	settings.setValue("romloadcount", 0);
	auto cartridge = Cartridge::create(buff, size, md5, romType, cartId, osystem, settings);
	if((int)optionTVPhosphor != TV_PHOSPHOR_AUTO)
	{
		props.set(Display_Phosphor, optionTVPhosphor ? "YES" : "NO");
	}
	osystem.frameBuffer().enablePhosphor(props.get(Display_Phosphor) == "YES",
		atoi(props.get(Display_PPBlend).c_str()));
	if((int)optionVideoSystem) // not auto
	{
		logMsg("forcing video system to: %s", optionVideoSystemToStr());
		props.set(Display_Format, optionVideoSystemToStr());
	}
	osystem.makeConsole(cartridge, props);
	auto &console = osystem.console();
	settings.setValue("framerate", (int)console.getFramerate());
	emuVideo.initImage(0, vidBufferX, console.tia().height());
	console.initializeVideo();
	console.initializeAudio();
	logMsg("is PAL: %s", EmuSystem::vidSysIsPAL() ? "yes" : "no");
	EmuSystem::configAudioPlayback();
	return 1;
}

int EmuSystem::loadGame(const char *path)
{
	bug_exit("should only use loadGameFromIO()");
	return 0;
}

int EmuSystem::loadGameFromIO(IO &io, const char *path, const char *)
{
	closeGame();
	setupGamePaths(path);
	uint8 buff[MAX_ROM_SIZE];
	uint32 size = io.read(buff, MAX_ROM_SIZE);
	return loadGameCommon(buff, size);
}

void EmuSystem::clearInputBuffers()
{
	Event &ev = osystem.eventHandler().event();
	ev.clear();

	ev.set(Event::ConsoleLeftDiffB, p1DiffB);
	ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
	ev.set(Event::ConsoleRightDiffB, p2DiffB);
	ev.set(Event::ConsoleRightDiffA, !p2DiffB);
	ev.set(Event::ConsoleColor, vcsColor);
	ev.set(Event::ConsoleBlackWhite, !vcsColor);
}

void EmuSystem::configAudioRate(double frameTime)
{
	pcmFormat.rate = optionSoundRate;
	osystem.soundGeneric().setFrameTime(osystem, optionSoundRate, frameTime);
}

void updateVControllerMapping(uint player, SysVController::Map &map)
{
	uint playerShift = player ? 7 : 0;
	map[SysVController::F_ELEM] = Event::JoystickZeroFire + playerShift;
	map[SysVController::F_ELEM+1] = (Event::JoystickZeroFire + playerShift) | SysVController::TURBO_BIT;

	map[SysVController::C_ELEM] = Event::ConsoleSelect;
	map[SysVController::C_ELEM+1] = Event::ConsoleReset;

	map[SysVController::D_ELEM] = (((uint)Event::JoystickZeroUp) + playerShift)
																| (((uint)Event::JoystickZeroLeft + playerShift) << 8);
	map[SysVController::D_ELEM+1] = Event::JoystickZeroUp + playerShift; // up
	map[SysVController::D_ELEM+2] = ((uint)Event::JoystickZeroUp  + playerShift)
																	| (((uint)Event::JoystickZeroRight + playerShift) << 8);
	map[SysVController::D_ELEM+3] = Event::JoystickZeroLeft + playerShift; // left
	map[SysVController::D_ELEM+5] = Event::JoystickZeroRight + playerShift; // right
	map[SysVController::D_ELEM+6] = ((uint)Event::JoystickZeroDown + playerShift)
																	| (((uint)Event::JoystickZeroLeft + playerShift) << 8);
	map[SysVController::D_ELEM+7] = Event::JoystickZeroDown + playerShift; // down
	map[SysVController::D_ELEM+8] = ((uint)Event::JoystickZeroDown + playerShift)
																	| (((uint)Event::JoystickZeroRight + playerShift) << 8);
}

uint EmuSystem::translateInputAction(uint input, bool &turbo)
{
	turbo = 0;
	switch(input)
	{
		case vcsKeyIdxUp: return Event::JoystickZeroUp;
		case vcsKeyIdxRight: return Event::JoystickZeroRight;
		case vcsKeyIdxDown: return Event::JoystickZeroDown;
		case vcsKeyIdxLeft: return Event::JoystickZeroLeft;
		case vcsKeyIdxLeftUp: return Event::JoystickZeroLeft | (Event::JoystickZeroUp << 8);
		case vcsKeyIdxRightUp: return Event::JoystickZeroRight | (Event::JoystickZeroUp << 8);
		case vcsKeyIdxRightDown: return Event::JoystickZeroRight | (Event::JoystickZeroDown << 8);
		case vcsKeyIdxLeftDown: return Event::JoystickZeroLeft | (Event::JoystickZeroDown << 8);
		case vcsKeyIdxJSBtnTurbo: turbo = 1;
		case vcsKeyIdxJSBtn: return Event::JoystickZeroFire;

		case vcsKeyIdxUp2: return Event::JoystickOneUp;
		case vcsKeyIdxRight2: return Event::JoystickOneRight;
		case vcsKeyIdxDown2: return Event::JoystickOneDown;
		case vcsKeyIdxLeft2: return Event::JoystickOneLeft;
		case vcsKeyIdxLeftUp2: return Event::JoystickOneLeft | (Event::JoystickOneUp << 8);
		case vcsKeyIdxRightUp2: return Event::JoystickOneRight | (Event::JoystickOneUp << 8);
		case vcsKeyIdxRightDown2: return Event::JoystickOneRight | (Event::JoystickOneDown << 8);
		case vcsKeyIdxLeftDown2: return Event::JoystickOneLeft | (Event::JoystickOneDown << 8);
		case vcsKeyIdxJSBtnTurbo2: turbo = 1;
		case vcsKeyIdxJSBtn2: return Event::JoystickOneFire;

		case vcsKeyIdxSelect: return Event::ConsoleSelect;
		case vcsKeyIdxP1Diff: return Event::Combo1; // toggle P1 diff
		case vcsKeyIdxP2Diff: return Event::Combo2; // toggle P2 diff
		case vcsKeyIdxColorBW: return Event::Combo3; // toggle Color/BW
		case vcsKeyIdxReset: return Event::ConsoleReset;
		case vcsKeyIdxKeyboard1Base ... vcsKeyIdxKeyboard1Base + 11:
			return Event::KeyboardZero1 + (input - vcsKeyIdxKeyboard1Base);
		case vcsKeyIdxKeyboard2Base ... vcsKeyIdxKeyboard2Base + 11:
			return Event::KeyboardOne1 + (input - vcsKeyIdxKeyboard2Base);
		default: bug_branch("%d", input);
	}
	return 0;
}

void EmuSystem::handleInputAction(uint state, uint emuKey)
{
	auto &ev = osystem.eventHandler().event();
	uint event1 = emuKey & 0xFF;

	//logMsg("got key %d", emuKey);

	switch(event1)
	{
		bcase Event::Combo1:
			if(state != Input::PUSHED)
				break;
			toggle(p1DiffB);
			popup.post(p1DiffB ? "P1 Difficulty -> B" : "P1 Difficulty -> A", 1);
			ev.set(Event::ConsoleLeftDiffB, p1DiffB);
			ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
		bcase Event::Combo2:
			if(state != Input::PUSHED)
				break;
			toggle(p2DiffB);
			popup.post(p2DiffB ? "P2 Difficulty -> B" : "P2 Difficulty -> A", 1);
			ev.set(Event::ConsoleRightDiffB, p2DiffB);
			ev.set(Event::ConsoleRightDiffA, !p2DiffB);
		bcase Event::Combo3:
			if(state != Input::PUSHED)
				break;
			toggle(vcsColor);
			popup.post(vcsColor ? "Color Switch -> Color" : "Color Switch -> B&W", 1);
			ev.set(Event::ConsoleColor, vcsColor);
			ev.set(Event::ConsoleBlackWhite, !vcsColor);
		bcase Event::JoystickZeroFire5: // TODO: add turbo support for on-screen controls to framework
			ev.set(Event::Type(Event::JoystickZeroFire), state == Input::PUSHED);
			if(state == Input::PUSHED)
				turboActions.addEvent(Event::JoystickZeroFire);
			else
				turboActions.removeEvent(Event::JoystickZeroFire);
		bcase Event::JoystickOneFire5: // TODO: add turbo support for on-screen controls to framework
			ev.set(Event::Type(Event::JoystickOneFire), state == Input::PUSHED);
			if(state == Input::PUSHED)
				turboActions.addEvent(Event::JoystickOneFire);
			else
				turboActions.removeEvent(Event::JoystickOneFire);
		bcase Event::KeyboardZero1 ... Event::KeyboardOnePound:
			ev.set(Event::Type(event1), state == Input::PUSHED);
		bdefault:
			ev.set(Event::Type(event1), state == Input::PUSHED);
			uint event2 = emuKey >> 8;
			if(event2) // extra event for diagonals
			{
				ev.set(Event::Type(event2), state == Input::PUSHED);
			}
	}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	auto &console = osystem.console();
	console.leftController().update();
	console.rightController().update();
	console.switches().update();
	auto &tia = console.tia();
	tia.update();
	if(renderGfx)
	{
		osystem.frameBuffer().render(pixBuff, tia);
		updateAndDrawEmuVideo();
	}
	auto frames = audioFramesPerVideoFrame;
	Int16 buff[frames * soundChannels];
	uint writtenFrames = osystem.soundGeneric().processAudio(buff, frames);
	if(renderAudio)
		writeSound(buff, writtenFrames);
}

void EmuSystem::reset(ResetMode mode)
{
	assert(gameIsRunning());
	if(mode == RESET_HARD)
	{
		osystem.console().system().reset();
	}
	else
	{
		Event &ev = osystem.eventHandler().event();
		ev.clear();
		ev.set(Event::ConsoleReset, 1);
		osystem.console().switches().update();
		TIA& tia = osystem.console().tia();
		tia.update();
		ev.set(Event::ConsoleReset, 0);
	}
}

int EmuSystem::saveState()
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	logMsg("saving state %s", saveStr.data());
	fixFilePermissions(saveStr);
	Serializer state(string(saveStr.data()), 0);
	if(!stateManager.saveState(state))
	{
		return STATE_RESULT_IO_ERROR;
	}
	return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	auto saveStr = sprintStateFilename(saveStateSlot);
	logMsg("loading state %s", saveStr.data());
	fixFilePermissions(saveStr);
	Serializer state(string(saveStr.data()), 1);
	if(!stateManager.loadState(state))
	{
		return STATE_RESULT_IO_ERROR;
	}
	updateSwitchValues();
	return STATE_RESULT_OK;
}

void EmuSystem::savePathChanged() { }

bool EmuSystem::hasInputOptions() { return false; }

void EmuSystem::onCustomizeNavView(EmuNavView &view)
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

CallResult EmuSystem::onInit()
{
	osystem.settings().setValue("framerate", 60); // set to avoid auto-frame calculation
	Paddles::setDigitalSensitivity(5);
	Paddles::setMouseSensitivity(7);
	EmuSystem::pcmFormat.channels = soundChannels;
	EmuSystem::pcmFormat.sample = Audio::SampleFormats::getFromBits(sizeof(Int16)*8);
	emuVideo.initPixmap((char*)pixBuff, IG::PIXEL_FMT_RGB565, vidBufferX, vidBufferY);
	return OK;
}
