#pragma once

// Customized minimal OSystem class needed for 2600.emu

class Cartridge;
class CheatManager;
class CommandMenu;
class Debugger;
class Launcher;
class Menu;
class Properties;
class Sound;
class VideoDialog;

#include <stella/common/bspf.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/common/AudioSettings.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Console.hxx>
#include <stella/emucore/FrameBufferConstants.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>
#include <stella/emucore/Settings.hxx>
#include <stella/emucore/Random.hxx>
#include <FSNode.hxx>
#include <SoundEmuEx.hh>
#include <EventHandler.hxx>
#include <FrameBuffer.hxx>
#include <optional>

namespace EmuEx
{
class EmuAudio;
class EmuApp;
}

class OSystem
{
	friend class EventHandler;

public:
	OSystem(EmuEx::EmuApp &);
	EventHandler& eventHandler() { return myEventHandler; }
	const EventHandler& eventHandler() const { return myEventHandler; }
	Random& random() { return myRandom; }
	const Random& random() const { return myRandom; }
	FrameBuffer& frameBuffer() { return myFrameBuffer; }
	const FrameBuffer& frameBuffer() const { return myFrameBuffer; }
	Sound& sound() { return mySound; }
	const Sound& sound() const { return mySound; }
	Settings& settings() { return mySettings; }
	const Settings& settings() const { return mySettings; }
	PropertiesSet& propSet() { return myPropSet; }
	const PropertiesSet& propSet() const { return myPropSet; }
	StateManager& state() { return myStateManager; }
	const StateManager& state() const { return myStateManager; }
	SoundEmuEx &soundEmuEx() { return mySound; }
	const SoundEmuEx &soundEmuEx() const { return mySound; }
	Console& console() { return *myConsole; }
	const Console& console() const { return *myConsole; }
	bool hasConsole() const { return (bool)myConsole; }
	void makeConsole(unique_ptr<Cartridge>& cart, const Properties& props, const char *gamePath);
	void deleteConsole();
	void setSoundMixRate(int mixRate, AudioSettings::ResamplingQuality);

	#ifdef DEBUGGER_SUPPORT
	void createDebugger(Console& console);
	Debugger& debugger() const { return *myDebugger; }
	#endif

	#ifdef CHEATCODE_SUPPORT
	CheatManager& cheat() const { return *myCheatManager; }
	#endif

	FilesystemNode stateDir() const;
	FilesystemNode nvramDir(std::string_view name) const;

	bool checkUserPalette(bool outputError = false) const { return false; }
	FilesystemNode paletteFile() const { return FilesystemNode{""}; }

	const FilesystemNode& romFile() const { return myRomFile; };

	void resetFps() {}

	EmuEx::EmuApp &app();

protected:
	EmuEx::EmuApp *appPtr{};
	std::optional<Console> myConsole{};
	Settings mySettings{};
	AudioSettings myAudioSettings{mySettings};
	Random myRandom;
	FrameBuffer myFrameBuffer{*this};
	EventHandler myEventHandler{*this};
	PropertiesSet myPropSet{};
	StateManager myStateManager{*this};
	SoundEmuEx mySound{*this};
	FilesystemNode myRomFile{};
};
