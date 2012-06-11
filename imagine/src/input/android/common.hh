#pragma once

#include <input/DragPointer.hh>

#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static struct TouchState
{
	constexpr TouchState() { }
	int id = -1;
	PointerState s;
	DragPointer dragState;
} m[maxCursors];
uint numCursors = maxCursors;

DragPointer *dragState(int p)
{
	return &m[p].dragState;
}

int cursorX(int p) { assert(p < Input::maxCursors); return m[p].s.x; }
int cursorY(int p) { assert(p < Input::maxCursors); return m[p].s.y; }
int cursorIsInView(int p) { assert(p < Input::maxCursors); return m[p].s.inWin; }

static const char *androidEventEnumToStr(uint e)
{
	switch(e)
	{
		case AMOTION_EVENT_ACTION_DOWN: return "Down";
		case AMOTION_EVENT_ACTION_UP: return "Up";
		case AMOTION_EVENT_ACTION_MOVE: return "Move";
		case AMOTION_EVENT_ACTION_CANCEL: return "Cancel";
		case AMOTION_EVENT_ACTION_POINTER_DOWN: return "PDown";
		case AMOTION_EVENT_ACTION_POINTER_UP: return "PUp";
	}
	return "Unknown";
}

static void processTouch(uint idx, uint action, TouchState &p)
{
	p.dragState.pointerEvent(Pointer::LBUTTON, action, p.s.x, p.s.y);
	Input::onInputEvent(InputEvent(idx, InputEvent::DEV_POINTER, Pointer::LBUTTON, action, p.s.x, p.s.y));
}

static bool handleTouchEvent(int action, int x, int y, int pid)
{
	//logMsg("action: %s", androidEventEnumToStr(action));
	switch(action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//logMsg("touch down for %d", pid);
			iterateTimes((uint)Input::maxCursors, i) // find a free touch element
			{
				if(m[i].id == -1)
				{
					auto &p = m[i];
					p.id = pid;
					pointerPos(x, y, &p.s.x, &p.s.y);
					p.s.inWin = 1;
					processTouch(i, INPUT_PUSHED, p);
					break;
				}
			}
		bcase AMOTION_EVENT_ACTION_UP:
		//case AMOTION_EVENT_ACTION_CANCEL: // calling code always uses AMOTION_EVENT_ACTION_UP
			forEachInArray(Input::m, p)
			{
				if(p->s.inWin)
				{
					//logMsg("touch up for %d from gesture end", p_i);
					p->id = -1;
					p->s.inWin = 0;
					processTouch(p_i, INPUT_RELEASED, *p);
				}
			}
		bcase AMOTION_EVENT_ACTION_POINTER_UP:
			//logMsg("touch up for %d", pid);
			iterateTimes((uint)Input::maxCursors, i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					p.id = -1;
					pointerPos(x, y, &p.s.x, &p.s.y);
					p.s.inWin = 0;
					processTouch(i, INPUT_RELEASED, p);
					break;
				}
			}
		bdefault:
			// move event
			//logMsg("event id %d", action);
			iterateTimes((uint)Input::maxCursors, i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					pointerPos(x, y, &p.s.x, &p.s.y);
					processTouch(i, INPUT_MOVED, p);
					break;
				}
			}
	}

	/*logMsg("pointer state:");
	iterateTimes(sizeofArray(m), i)
	{
		logMsg("id: %d x: %d y: %d inWin: %d", m[i].id, m[i].s.x, m[i].s.y, m[i].s.inWin);
	}*/

	return 1;
}

static void handleTrackballEvent(int action, float x, float y)
{
	int iX = x * 1000., iY = y * 1000., xTrans, yTrans;
	pointerPos(iX, iY, &xTrans, &yTrans);
	//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);

	if(action == AMOTION_EVENT_ACTION_MOVE)
		Input::onInputEvent(InputEvent(0, InputEvent::DEV_REL_POINTER, 0, INPUT_MOVED_RELATIVE, xTrans, yTrans));
	else
		Input::onInputEvent(InputEvent(0, InputEvent::DEV_REL_POINTER, Key::ENTER, action == AMOTION_EVENT_ACTION_DOWN ? INPUT_PUSHED : INPUT_RELEASED, 0));
}

static void handleKeyEvent(int key, int down, uint devId, uint metaState)
{
	assert((uint)key < Key::COUNT);
	uint action = down ? INPUT_PUSHED : INPUT_RELEASED;
	#ifdef CONFIG_INPUT_ICADE
		if(!iCadeActive() || (iCadeActive() && !processICadeKey(decodeAscii(key, 0), action)))
	#endif
			Input::onInputEvent(InputEvent(devId, InputEvent::DEV_KEYBOARD, key & 0xff, action, metaState));
}

}
