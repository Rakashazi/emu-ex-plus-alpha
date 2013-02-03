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

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

int cursorX(int p) { return m[p].x; }
int cursorY(int p) { return m[p].y; }
int cursorIsInView(int p) { return m[p].inWin; }

static bool allowKeyRepeats = 1;
void setKeyRepeat(bool on)
{
	allowKeyRepeats = on;
}

static void initPointer()
{
	// make a blank cursor
	X11Pixmap_ blank;
	XColor dummy;
	char data[1] = {0};
	blank = XCreateBitmapFromData(dpy, win, data, 1, 1);
	if(blank == None)
	{
		logErr("unable to create blank cursor");
	}
	blankCursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
	XFreePixmap (dpy, blank);

	normalCursor = XCreateFontCursor(dpy, XC_left_ptr);
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		return 1;
	}
	return 0;
}

static Device *kbDevice = nullptr;

CallResult init()
{
	// TODO: get actual device list from XI2
	addDevice(Device{0, Event::MAP_KEYBOARD, Device::TYPE_BIT_KEYBOARD, "Keyboard"});
	kbDevice = devList.last();
	initPointer();
	return OK;
}

}

static void hideCursor()
{
	XDefineCursor(dpy, win, Input::blankCursor);
}

static void showCursor()
{
	XDefineCursor(dpy, win, Input::normalCursor);
}

static void updatePointer(uint event, int p, uint action, int x, int y)
{
	Input::pointerPos(x, y, &Input::m[p].x, &Input::m[p].y);
	Input::dragStateArr[p].pointerEvent(event, action, Input::m[p].x, Input::m[p].y);
	Input::onInputEvent(Input::Event(p, Input::Event::MAP_POINTER, event, action, Input::m[p].x, Input::m[p].y, nullptr));
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
