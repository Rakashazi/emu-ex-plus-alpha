#pragma once

#include <input/DragPointer.hh>

#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static PointerState m[Input::maxCursors] = { { 0, 0, 0 } };
uint numCursors = Input::maxCursors;
static DragPointer dragStateArr[Input::maxCursors];

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

int cursorX(int p) { assert(p < Input::maxCursors); return m[p].x; }
int cursorY(int p) { assert(p < Input::maxCursors); return m[p].y; }
int cursorIsInView(int p) { assert(p < Input::maxCursors); return m[p].inWin; }

static void commonInit()
{
	/*iterateTimes(numCursors, i)
	{
		dragStateArr[i].init(i);
	}*/
}

static const char *androidEventEnumToStr(uint e)
{
	switch(e)
	{
		case AMOTION_EVENT_ACTION_DOWN: return "Down";
		case AMOTION_EVENT_ACTION_UP: return "Up";
		case AMOTION_EVENT_ACTION_MOVE: return "Move";
		case AMOTION_EVENT_ACTION_POINTER_DOWN: return "PDown";
		case AMOTION_EVENT_ACTION_POINTER_UP: return "PUp";
	}
	return "Unknown";
}

static bool handleTouchEvent(int action, int x, int y, int pid)
{
	if(unlikely(pid >= Input::maxCursors))
	{
		logMsg("got out of range pid %d", pid);
		return 0;
	}

	//logMsg("event %s for pid %d", androidEventEnumToStr(action), pid);
	var_copy(p, &Input::m[pid]);
	pointerPos(x, y, &p->x, &p->y);
	int funcAction = INPUT_MOVED;
	switch(action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//logMsg("touch down for %d", pid);
			p->inWin = 1;
			funcAction = INPUT_PUSHED;
		bcase AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
			//logMsg("touch up for %d", pid);
			p->inWin = 0;
			funcAction = INPUT_RELEASED;
		bdefault:
			// move event
			//logMsg("event id %d", action);
			break;
	}

	Input::dragStateArr[pid].pointerEvent(Pointer::LBUTTON, funcAction, p->x, p->y);
	//if(likely(onInputEventHandler != 0))
		Input::onInputEvent(InputEvent(pid, InputEvent::DEV_POINTER, Pointer::LBUTTON, funcAction, p->x, p->y));
		//onInputEventHandler(onInputEventHandlerCtx, InputEvent(pid, InputEvent::DEV_POINTER, Pointer::LBUTTON, funcAction, p->x, p->y));
	if(action == AMOTION_EVENT_ACTION_UP) // send released events for all other touches
	{
		forEachInArray(Input::m, p)
		{
			if(p->inWin)
			{
				//logMsg("touch up for %d from gesture end", p_i);
				p->inWin = 0;
				Input::onInputEvent(InputEvent(pid, InputEvent::DEV_POINTER, Pointer::LBUTTON, INPUT_RELEASED, p->x, p->y));
				//if(likely(onInputEventHandler != 0))
				//	onInputEventHandler(onInputEventHandlerCtx, InputEvent(pid, InputEvent::DEV_POINTER, Pointer::LBUTTON, INPUT_RELEASED, p->x, p->y));
			}
		}
	}

	/*logMsg("pointer state:");
	iterateTimes(sizeofArray(m), i)
	{
		logMsg("x: %d y: %d inWin: %d", m[i].x, m[i].y, m[i].inWin);
	}*/

	return 1;
}

static void handleTrackballEvent(int action, float x, float y)
{
	int iX = x * 1000., iY = y * 1000., xTrans, yTrans;
	pointerPos(iX, iY, &xTrans, &yTrans);
	//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);
	//if(likely(onInputEventHandler != 0))
	{
		if(action == AMOTION_EVENT_ACTION_MOVE)
			Input::onInputEvent(InputEvent(0, InputEvent::DEV_REL_POINTER, 0, INPUT_MOVED_RELATIVE, xTrans, yTrans));
			//onInputEventHandler(onInputEventHandlerCtx, InputEvent(0, InputEvent::DEV_REL_POINTER, 0, INPUT_MOVED_RELATIVE, xTrans, yTrans));
		else
			Input::onInputEvent(InputEvent(0, InputEvent::DEV_REL_POINTER, Key::ENTER, action == AMOTION_EVENT_ACTION_DOWN ? INPUT_PUSHED : INPUT_RELEASED));
			//onInputEventHandler(onInputEventHandlerCtx, InputEvent(0, InputEvent::DEV_REL_POINTER, Key::ENTER, action == AMOTION_EVENT_ACTION_DOWN ? INPUT_PUSHED : INPUT_RELEASED));
	}
}

static void handleKeyEvent(int key, int down, uint devId = 0)
{
	assert((uint)key < Key::COUNT);
	uint action = down ? INPUT_PUSHED : INPUT_RELEASED;
	//if(likely(onInputEventHandler != 0))
	{
		#ifdef CONFIG_INPUT_ICADE
		if(!iCadeActive() || (iCadeActive() && !processICadeKey(decodeAscii(key), action)))
		#endif
			Input::onInputEvent(InputEvent(devId, InputEvent::DEV_KEYBOARD, key & 0xff, action));
			//onInputEventHandler(onInputEventHandlerCtx, InputEvent(devId, InputEvent::DEV_KEYBOARD, key & 0xff, action));
	}
}

}
