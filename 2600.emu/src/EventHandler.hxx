// Customized minimal EvenHandler class needed for 2600.emu

#pragma once

#include <stella/emucore/Event.hxx>

class Console;
class OSystem;

enum MouseButton {
	EVENT_LBUTTONDOWN,
	EVENT_LBUTTONUP,
	EVENT_RBUTTONDOWN,
	EVENT_RBUTTONUP,
	EVENT_WHEELDOWN,
	EVENT_WHEELUP
};

enum JoyHat {
	EVENT_HATUP     = 0,  // make sure these are set correctly,
	EVENT_HATDOWN   = 1,  // since they'll be used as array indices
	EVENT_HATLEFT   = 2,
	EVENT_HATRIGHT  = 3,
	EVENT_HATCENTER = 4
};

enum JoyHatMask {
	EVENT_HATUP_M     = 1<<0,
	EVENT_HATDOWN_M   = 1<<1,
	EVENT_HATLEFT_M   = 1<<2,
	EVENT_HATRIGHT_M  = 1<<3,
	EVENT_HATCENTER_M = 1<<4
};

enum EventMode {
	kEmulationMode = 0,  // make sure these are set correctly,
	kMenuMode      = 1,  // since they'll be used as array indices
	kNumModes      = 2
};

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
	bool myAllowAllDirectionsFlag = false;

	EventHandler(OSystem& osystem) {}
	Event& event() { return myEvent; }
	void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }
};
