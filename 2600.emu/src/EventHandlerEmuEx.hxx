#pragma once

// Customized minimal EventHandler class needed for 2600.emu

#include <stella/emucore/Event.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>
#include <stella/emucore/Control.hxx>

class OSystem;
class Properties;

class EventHandler
{
public:
	// Enumeration representing the different states of operation
	enum State {
		S_NONE,
		S_EMULATE,
		S_PAUSE,
		S_LAUNCHER,
		S_MENU,
		S_CMDMENU,
		S_DEBUGGER
	};

	EventHandler(OSystem& osystem) {}
	Event& event() { return myEvent; }
	const Event& event() const { return myEvent; }
	void allowAllDirections(bool allow) {}
	EventHandlerState state() const { return EventHandlerState::EMULATION; }
	bool inTIAMode() const { return true; }
	void defineKeyControllerMappings(const Controller::Type type, Controller::Jack port) {}
	void enableEmulationKeyMappings() {}
	void defineJoyControllerMappings(const Controller::Type type, Controller::Jack port) {}
	void enableEmulationJoyMappings() {}
	void setMouseControllerMode(const string& enable) {}
	void defineKeyControllerMappings(const Controller::Type, Controller::Jack, const Properties&) {}
	void set7800Mode() {}

private:
	Event myEvent{};
};
