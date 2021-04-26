#pragma once

// Customized minimal OSystem class needed for 2600.emu

class Cartridge;
class CheatManager;
class CommandMenu;
class Console;
class Debugger;
class EventHandler;
class FrameBuffer;
class Launcher;
class Menu;
class SoundEmuEx;
class Properties;
class Random;
class Settings;
class Sound;
class VideoDialog;
class EmuAudio;
class EmuApp;

#include <stella/common/bspf.hxx>
#include <stella/common/StateManager.hxx>
#include <stella/common/AudioSettings.hxx>
#include <stella/common/audio/Resampler.hxx>
#include <stella/emucore/PropsSet.hxx>
#include <stella/emucore/Console.hxx>
#include <stella/emucore/FSNode.hxx>
#include <stella/emucore/FrameBufferConstants.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>

class OSystem
{
	friend class EventHandler;

public:
	OSystem(EmuApp &);
	EventHandler& eventHandler() const;
	FrameBuffer& frameBuffer() const;
	Sound& sound() const;
	Settings& settings() const;
	Random& random() const;
	PropertiesSet& propSet() const;
	StateManager& state() const;
	void makeConsole(unique_ptr<Cartridge>& cart, const Properties& props, const char *gamePath);
	void deleteConsole();
	void setFrameTime(double frameTime, int rate);
	void setResampleQuality(AudioSettings::ResamplingQuality quality);
	void processAudio(EmuAudio *audio);

	Console& console() const { return *myConsole; }

	bool hasConsole() const { return myConsole != nullptr; }

	#ifdef DEBUGGER_SUPPORT
	void createDebugger(Console& console);
	Debugger& debugger() const { return *myDebugger; }
	#endif

	#ifdef CHEATCODE_SUPPORT
	CheatManager& cheat() const { return *myCheatManager; }
	#endif

	string stateDir() const;
	string nvramDir() const;

	string configFile() const { return ""; }

	bool checkUserPalette(bool outputError = false) const { return false; }
	string paletteFile() const { return ""; }

	const FilesystemNode& romFile() const { return myRomFile; };

	void resetFps() {}

	EmuApp &app();

protected:
	EmuApp *appPtr{};
	std::unique_ptr<Console> myConsole{};
	std::unique_ptr<StateManager> myStateManager{};
	std::unique_ptr<Random> myRandom{};
	std::unique_ptr<EventHandler> myEventHandler{};
	std::unique_ptr<FrameBuffer> myFrameBuffer{};
	std::unique_ptr<PropertiesSet> myPropSet{};
	std::unique_ptr<Settings> mySettings{};
	std::unique_ptr<SoundEmuEx> mySound{};
	std::unique_ptr<AudioSettings> myAudioSettings{};
	FilesystemNode myRomFile{};
};
