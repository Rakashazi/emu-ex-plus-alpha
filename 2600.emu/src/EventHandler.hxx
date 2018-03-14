// Customized minimal EvenHandler class needed for 2600.emu

#pragma once

#include <stella/emucore/Event.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>

class Console;
class OSystem;

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

	Event myEvent{};
	EventHandlerState myState{EventHandlerState::NONE};
	bool myAllowAllDirectionsFlag = false;

	EventHandler(OSystem& osystem) {}
	Event& event() { return myEvent; }
	void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }
	EventHandlerState state() const { return myState; }
};
