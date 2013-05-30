/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "main"
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
#include "ImagineSound.hh"

#include <logger/interface.h>
#include <util/area2.h>
#include <gfx/GfxSprite.hh>
#include <audio/Audio.hh>
#include <fs/sys.hh>
#include <io/sys.hh>
#include <gui/View.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <util/preprocessor/repeat.h>
#include <unzip.h>
#include <EmuSystem.hh>
#include <CommonFrameworkIncludes.hh>

const char *creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2013\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nStella Team\nstella.sourceforge.net";
static ImagineSound *vcsSound = 0;
static uint16 tiaColorMap[256];
static const PixelFormatDesc *pixFmt = &PixelFormatRGB565;
static uint tiaSoundRate = 0, tiaSamplesPerFrame = 0;
static Console *console = 0;
#include "MiscStella.hh"
#define MAX_ROM_SIZE  512 * 1024

static Cartridge *cartridge = 0;
static OSystem osystem;
static StateManager stateManager(&osystem);
static bool p1DiffB = 1, p2DiffB = 1, vcsColor = 1;

const uint EmuSystem::maxPlayers = 2;
uint EmuSystem::aspectRatioX = 4, EmuSystem::aspectRatioY = 3;
#include "CommonGui.hh"

void EmuSystem::initOptions()
{

}

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

enum {
//	CFGKEY_* = 256 // no config keys defined yet
};

bool EmuSystem::readConfig(Io *io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		// no config keys defined yet
	}
	return 1;
}

void EmuSystem::writeConfig(Io *io)
{

}

static const uint vidBufferX = 160, vidBufferY = 320;
static uint16 pixBuff[vidBufferX*vidBufferY] __attribute__ ((aligned (8))) {0};

static bool isVCSRomExtension(const char *name)
{
	return string_hasDotExtension(name, "a26") || string_hasDotExtension(name, "bin");
}

static bool isVCSExtension(const char *name)
{
	return isVCSRomExtension(name) || string_hasDotExtension(name, "zip");
}

static int vcsFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR || isVCSExtension(name);
}

FsDirFilterFunc EmuFilePicker::defaultFsFilter = vcsFsFilter;
FsDirFilterFunc EmuFilePicker::defaultBenchmarkFsFilter = vcsFsFilter;

static char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return '0' + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

void EmuSystem::sprintStateFilename(char *str, size_t size, int slot, const char *savePath, const char *gameName)
{
	snprintf(str, size, "%s/%s.0%c.sta", savePath, gameName, saveSlotChar(slot));
}

void EmuSystem::saveAutoState()
{
	if(gameIsRunning() && optionAutoSaveState)
	{
		FsSys::cPath saveStr;
		sprintStateFilename(saveStr, -1);
		logMsg("saving autosave-state %s", saveStr);
		if(Config::envIsIOSJB)
			fixFilePermissions(saveStr);
		Serializer state(string(saveStr), 0);
		if(!stateManager.saveState(state))
		{
			logMsg("failed");
		}
	}
}

void EmuSystem::saveBackupMem() { }

void EmuSystem::closeSystem()
{
	delete console;
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

bool EmuSystem::vidSysIsPAL() { return 0; }
uint EmuSystem::multiresVideoBaseX() { return 0; }
uint EmuSystem::multiresVideoBaseY() { return 0; }
bool touchControlsApplicable() { return 1; }

static bool openROM(uchar buff[MAX_ROM_SIZE], const char *path, uint32& size)
{
	if(string_hasDotExtension(path, "zip"))
	{
		unzFile zipFile = unzOpen(path);
		if(!zipFile) return 0;

		if(unzGoToFirstFile(zipFile) != UNZ_OK)
		{
			unzClose(zipFile);
			return 0;
		}

		// Find a valid file
		unz_file_info info;
		bool foundRom = 0;
		do
		{
			FsSys::cPath name;
			if(unzGetCurrentFileInfo(zipFile, &info, name, 128, NULL, 0, NULL, 0) != UNZ_OK)
			{
				unzClose(zipFile);
				return 0;
			}

			if(isVCSRomExtension(name))
			{
				foundRom = 1;
				break;
			}
		}
		while(unzGoToNextFile(zipFile) == UNZ_OK);

		if(!foundRom || info.uncompressed_size > MAX_ROM_SIZE)
		{
			unzClose(zipFile);
			return 0;
		}

		// read the ROM data
		if(unzOpenCurrentFile(zipFile) != UNZ_OK)
		{
			unzClose(zipFile);
			return 0;
		}

		size = info.uncompressed_size;
		if(unzReadCurrentFile(zipFile, buff, info.uncompressed_size) != (int)info.uncompressed_size)
		{
			unzCloseCurrentFile(zipFile);
			unzClose(zipFile);
			return 0;
		}

		unzCloseCurrentFile(zipFile);
		unzClose(zipFile);
		return 1;
	}
	else
	{
		Io *f = IoSys::open(path);
		if(!f)
			return 0;
		size = f->readUpTo(buff, MAX_ROM_SIZE);
		delete f;
		return 1;
	}
}

int EmuSystem::loadGame(const char *path)
{
	closeGame();
	setupGamePaths(path);
	uchar buff[MAX_ROM_SIZE];
	string md5;
	uint32 size;
	if(!openROM(buff, path, size))
	{
		popup.post("Error loading game", 1);
		return 0;
	}
	md5 = MD5(buff, size);
	Properties props;
	osystem.propSet().getMD5(md5, props);

	string romType = props.get(Cartridge_Type);
	string cartId;
	Settings &settings = osystem.settings();
	settings.setInt("romloadcount", 0);
	cartridge = Cartridge::create(buff, size, md5, romType, cartId, osystem, settings);
	console = new Console(&osystem, cartridge, props);
	osystem.myConsole = console;

	emuView.initImage(0, vidBufferX, console->tia().height());
	console->initializeVideo();
	console->initializeAudio();
	configAudioPlayback();
	return 1;
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

void EmuSystem::configAudioRate()
{
	pcmFormat.rate = optionSoundRate;
	tiaSoundRate = optionSoundRate;
	#if defined(CONFIG_ENV_WEBOS)
	if(optionFrameSkip != optionFrameSkipAuto)
		tiaSoundRate = (float)optionSoundRate * (42660./44100.); // better sync with Pre's refresh rate
	#endif
	tiaSamplesPerFrame = tiaSoundRate/60.;
	if(gameIsRunning())
	{
		vcsSound->tiaSound().outputFrequency(tiaSoundRate);
	}
	logMsg("set sound rate %d", tiaSoundRate);
}

static const uint audioMaxFramesPerUpdate = (Audio::maxRate/59)*2;

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

namespace Input
{
void onInputEvent(const Input::Event &e)
{
	handleInputEvent(e);
}
}

void EmuSystem::runFrame(bool renderGfx, bool processGfx, bool renderAudio)
{
	console->controller(Controller::Left).update();
	console->controller(Controller::Right).update();
	console->switches().update();
	TIA& tia = console->tia();
	tia.update();
	if(renderGfx)
	{
		assert(tia.height() <= 320);
		uint h = tia.height();
		uint8* currentFrame = tia.currentFrameBuffer() /*+ (tia.ystart() * 160)*/;
		iterateTimes(160 * h, i)
		{
			pixBuff[i] = tiaColorMap[currentFrame[i]];
		}
		emuView.updateAndDrawContent();
	}
	if(renderAudio)
	{
		#ifdef USE_NEW_AUDIO
		Audio::BufferContext *aBuff = Audio::getPlayBuffer(tiaSamplesPerFrame);
		if(!aBuff) return;
		vcsSound->processAudio((Int16*)aBuff->data, aBuff->frames);
		Audio::commitPlayBuffer(aBuff, aBuff->frames);
		#else
		Int16 buff[tiaSamplesPerFrame*soundChannels];
		vcsSound->processAudio(buff, tiaSamplesPerFrame);
		Audio::writePcm((uchar*)buff, tiaSamplesPerFrame);
		#endif
	}
}

void EmuSystem::resetGame()
{
	assert(gameIsRunning());
	console->system().reset();
}

int EmuSystem::saveState()
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	logMsg("saving state %s", saveStr);
	if(Config::envIsIOSJB)
		fixFilePermissions(saveStr);
	Serializer state(string(saveStr), 0);
	if(!stateManager.saveState(state))
	{
		return STATE_RESULT_IO_ERROR;
	}
	return STATE_RESULT_OK;
}

int EmuSystem::loadState(int saveStateSlot)
{
	FsSys::cPath saveStr;
	sprintStateFilename(saveStr, saveStateSlot);
	logMsg("loading state %s", saveStr);
	if(Config::envIsIOSJB)
		fixFilePermissions(saveStr);
	Serializer state(string(saveStr), 1);
	if(!stateManager.loadState(state))
	{
		return STATE_RESULT_IO_ERROR;
	}
	updateSwitchValues();
	return STATE_RESULT_OK;
}

void EmuSystem::savePathChanged() { }

namespace Base
{

void onAppMessage(int type, int shortArg, int intArg, int intArg2) { }

CallResult onInit(int argc, char** argv)
{
	//Audio::setHintPcmFramesPerWrite(950); // TODO: for PAL when supported
	EmuSystem::pcmFormat.channels = soundChannels;
	EmuSystem::pcmFormat.sample = Audio::SampleFormats::getFromBits(sizeof(Int16)*8);
	mainInitCommon();
	emuView.initPixmap((uchar*)pixBuff, pixFmt, vidBufferX, vidBufferY);

	Settings *settings = new Settings(&osystem);
	settings->setInt("framerate", 60);

	return OK;
}

CallResult onWindowInit()
{
	static const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, VertexColorPixelFormat.build(.5, .5, .5, 1.) },
		{ .03, VertexColorPixelFormat.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .3, VertexColorPixelFormat.build((200./255.) * .4, (100./255.) * .4, (0./255.) * .4, 1.) },
		{ .97, VertexColorPixelFormat.build((75./255.) * .4, (37.5/255.) * .4, (0./255.) * .4, 1.) },
		{ 1., VertexColorPixelFormat.build(.5, .5, .5, 1.) },
	};

	mainInitWindowCommon(navViewGrad);
	return OK;
}

}
