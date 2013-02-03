#pragma once
#include <input/common/common.h>
#include <input/DragPointer.hh>

#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static PointerState m[Input::maxCursors];
uint numCursors = Input::maxCursors;
static DragPointer dragStateArr[Input::maxCursors];

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

static void mouseEvent(uint button, int p, uint action, int x, int y)
{
	if(unlikely(p >= Input::maxCursors))
	{
		logMsg("touch index out of range");
		return;
	}
	pointerPos(x, y, &m[p].x, &m[p].y);
	#ifdef CONFIG_BASE_SDL_PDL
		button = Input::Pointer::LBUTTON;
		m[p].inWin = action != INPUT_RELEASED; // handle touch end on WebOS
	#else
		m[p].inWin = 1;
	#endif
	dragStateArr[p].pointerEvent(button, action, m[p].x, m[p].y);
	//logMsg("p %d @ %d,%d %d", p, m[p].x, m[p].y, action);
	Input::onInputEvent(Input::Event(p, Input::Event::MAP_POINTER, button, action, m[p].x, m[p].y));
}

static void keyEvent(SDL_keysym k, uint action)
{
	assert(k.sym < Keycode::COUNT);
	uint modifiers = k.mod & KMOD_SHIFT;
	//logMsg("key %d %s, action %d", k.sym, Input::buttonName(Input::Event::MAP_KEYBOARD, k.sym), action);
	#if CONFIG_ENV_WEBOS_OS >= 3
	if(unlikely(k.sym == 24)) // WebOS Dismiss Keyboard
	{
		if(action == INPUT_RELEASED)
		{
			hideSoftInput();
		}
		return;
	}
	#endif
	#ifdef CONFIG_INPUT_ICADE
	if(!iCadeActive() || (iCadeActive() && !processICadeKey(decodeAscii(k.sym, modifiers), action)))
	#endif
		Input::onInputEvent(Input::Event(0, InputEvent::MAP_KEYBOARD, k.sym & 0xFFF, action, modifiers));
}

void setKeyRepeat(bool on)
{
	if(on)
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	else
		SDL_EnableKeyRepeat(0, 0);
}

#if CONFIG_ENV_WEBOS_OS >= 3

static bool softInputActive = 0;
bool softInputIsActive() { return softInputActive; }

void showSoftInput()
{
	using namespace Base;
	logMsg("showing soft input");
	PDL_SetKeyboardState(PDL_TRUE);
	softInputActive = 1;
}

void hideSoftInput()
{
	using namespace Base;
	logMsg("hiding soft input");
	PDL_SetKeyboardState(PDL_FALSE);
	softInputActive = 0;
}

#endif

int cursorX(int p) { return m[p].x; }
int cursorY(int p) { return m[p].y; }
int cursorIsInView(int p) { return m[p].inWin; }

CallResult init()
{
	return OK;
}

}
