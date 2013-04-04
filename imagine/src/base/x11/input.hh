#pragma once
#include <engine-globals.h>
#include <input/Input.hh>
#include <logger/interface.h>
#include <util/bits.h>
#include <input/common/common.h>
#include <input/DragPointer.hh>

using namespace Base;

#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static PointerState m[Input::maxCursors];
static DragPointer dragStateArr[Input::maxCursors];
static Cursor blankCursor = (Cursor)0;
static Cursor normalCursor = (Cursor)0;
uint numCursors = 0;
bool translateKeycodes = 0;

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

static bool allowKeyRepeats = 1;
void setKeyRepeat(bool on)
{
	allowKeyRepeats = on;
}

static void initPerWindowData(X11Window win)
{
	// make a blank cursor
	char data[1] = {0};
	auto blank = XCreateBitmapFromData(dpy, win, data, 1, 1);
	if(blank == None)
	{
		logErr("unable to create blank cursor");
	}
	XColor dummy;
	blankCursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
	XFreePixmap(dpy, blank);
	normalCursor = XCreateFontCursor(dpy, XC_left_ptr);
}

void hideCursor()
{
	XDefineCursor(dpy, win, Input::blankCursor);
}

void showCursor()
{
	XDefineCursor(dpy, win, Input::normalCursor);
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		return 1;
	}
	return 0;
}

void setTranslateKeyboardEventsByModifiers(bool on)
{
	if(on)
		logMsg("translating key codes by modifier keys");
	else
		logMsg("using direct key codes");
	translateKeycodes = on;
}

static Device *kbDevice = nullptr;

CallResult init()
{
	// TODO: get actual device list from XI2
	addDevice(Device{0, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEYBOARD, "Keyboard"});
	kbDevice = devList.last();
	return OK;
}

}

static void updatePointer(uint event, int p, uint action, int x, int y)
{
	using namespace Input;
	auto &state = dragStateArr[p];
	auto pos = pointerPos(x, y);
	state.pointerEvent(event, action, pos);
	onInputEvent(Event(p, Event::MAP_POINTER, event, action, pos.x, pos.y, false, nullptr));
}

static void handlePointerButton(uint button, int p, uint action, int x, int y)
{
	updatePointer(button, p, action, x, y);
}

static void handlePointerMove(int x, int y, int p)
{
	Input::m[p].inWin = 1;
	updatePointer(0, p, Input::MOVED, x, y);
}

static void handlePointerEnter(int p, int x, int y)
{
	Input::m[p].inWin = 1;
	updatePointer(0, p, Input::ENTER_VIEW, x, y);
}

static void handlePointerLeave(int p, int x, int y)
{
	Input::m[p].inWin = 0;
	updatePointer(0, p, Input::EXIT_VIEW, x, y);
}

static void handleKeyEv(KeySym k, uint action, bool isShiftPushed)
{
	//logMsg("got keysym %d", (int)k);
	Input::onInputEvent(Input::Event(0, Input::Event::MAP_KEYBOARD, k & 0xFFFF, action, isShiftPushed, Input::kbDevice));
}
