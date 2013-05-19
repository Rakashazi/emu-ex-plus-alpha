#pragma once

#include <base/android/private.hh>
#include <input/DragPointer.hh>
#ifdef CONFIG_INPUT_ICADE
#include <input/common/iCade.hh>
#endif

namespace Input
{

static Device *builtinKeyboardDev = nullptr;

static bool isXperiaPlayDeviceStr(const char *str)
{
	return strstr(str, "R800") || string_equal(str, "zeus");
}

bool hasXperiaPlayGamepad()
{
	return builtinKeyboardDev && builtinKeyboardDev->subtype == Device::SUBTYPE_XPERIA_PLAY;
}

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

static void processTouch(uint idx, uint action, TouchState &p, IG::Point2D<int> pos, bool isTouch)
{
	//logMsg("pointer: %d action: %s @ %d,%d", idx, eventActionToStr(action), pos.x, pos.y);
	p.dragState.pointerEvent(Pointer::LBUTTON, action, pos);
	onInputEvent(Event(idx, Event::MAP_POINTER, Pointer::LBUTTON, action, pos.x, pos.y, isTouch, nullptr));
}

static bool handleTouchEvent(int action, int x, int y, int pid, bool isTouch)
{
	//logMsg("action: %s", androidEventEnumToStr(action));
	auto pos = pointerPos(x, y);
	switch(action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//logMsg("touch down for %d", pid);
			iterateTimes((uint)maxCursors, i) // find a free touch element
			{
				if(m[i].id == -1)
				{
					auto &p = m[i];
					p.id = pid;
					p.s.inWin = 1;
					processTouch(i, PUSHED, p, pos, isTouch);
					break;
				}
			}
		bcase AMOTION_EVENT_ACTION_UP:
		//case AMOTION_EVENT_ACTION_CANCEL: // calling code always uses AMOTION_EVENT_ACTION_UP
			forEachInArray(m, p)
			{
				if(p->s.inWin)
				{
					//logMsg("touch up for %d from gesture end", p_i);
					int x = p->dragState.x;
					int y = p->dragState.y;
					p->id = -1;
					p->s.inWin = 0;
					processTouch(p_i, RELEASED, *p, {x, y}, isTouch);
				}
			}
		bcase AMOTION_EVENT_ACTION_POINTER_UP:
			//logMsg("touch up for %d", pid);
			iterateTimes((uint)maxCursors, i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					p.id = -1;
					p.s.inWin = 0;
					processTouch(i, RELEASED, p, pos, isTouch);
					break;
				}
			}
		bdefault:
			// move event
			//logMsg("event id %d", action);
			iterateTimes((uint)maxCursors, i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					processTouch(i, MOVED, p, pos, isTouch);
					break;
				}
			}
	}

	/*logMsg("pointer state:");
	iterateTimes(sizeofArray(m), i)
	{
		if(m[i].id != -1)
			logMsg("id: %d x: %d y: %d inWin: %d", m[i].id, m[i].dragState.x, m[i].dragState.y, m[i].s.inWin);
	}*/

	return 1;
}

static void handleTrackballEvent(int action, float x, float y)
{
	int iX = x * 1000., iY = y * 1000.;
	auto pos = pointerPos(iX, iY);
	//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);

	if(action == AMOTION_EVENT_ACTION_MOVE)
		onInputEvent(Event(0, Event::MAP_REL_POINTER, 0, MOVED_RELATIVE, pos.x, pos.y, false, nullptr));
	else
		onInputEvent(Event(0, Event::MAP_REL_POINTER, Keycode::ENTER, action == AMOTION_EVENT_ACTION_DOWN ? PUSHED : RELEASED, 0, nullptr));
}

static void handleKeyEvent(int key, int down, uint devId, uint metaState, const Device &dev)
{
	assert((uint)key < Keycode::COUNT);
	uint action = down ? PUSHED : RELEASED;
	#ifdef CONFIG_INPUT_ICADE
		if(!dev.iCadeMode() || (dev.iCadeMode() && !processICadeKey(decodeAscii(key, 0), action, dev)))
	#endif
			onInputEvent(Event(devId, Event::MAP_KEYBOARD, key & 0xff, action, metaState, &dev));
}

static InputTextDelegate vKeyboardTextDelegate;
static Rect2<int> textRect(8, 200, 8+304, 200+48);
static JavaInstMethod<void> jStartSysTextInput, jFinishSysTextInput, jPlaceSysTextInput;
static
#if CONFIG_ENV_ANDROID_MINSDK >= 9
void
#else
jboolean
#endif
JNICALL textInputEnded(JNIEnv* env, jobject thiz, jstring jStr);

static void setupTextInputJni(JNIEnv* jEnv)
{
	using namespace Base;
	if(!jStartSysTextInput.m)
	{
		logMsg("setting up text input JNI");
		jStartSysTextInput.setup(jEnv, jBaseActivityCls, "startSysTextInput", "(Ljava/lang/String;Ljava/lang/String;IIIII)V");
		jFinishSysTextInput.setup(jEnv, jBaseActivityCls, "finishSysTextInput", "(Z)V");
		jPlaceSysTextInput.setup(jEnv, jBaseActivityCls, "placeSysTextInput", "(IIII)V");

		static JNINativeMethod activityMethods[] =
		{
			#if CONFIG_ENV_ANDROID_MINSDK >= 9
				{"sysTextInputEnded", "(Ljava/lang/String;)V", (void *)&textInputEnded}
			#else
				{"sysTextInputEnded", "(Ljava/lang/String;)Z", (void *)&textInputEnded}
			#endif
		};
		jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
	}
}

uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels)
{
	using namespace Base;
	auto jEnv = eEnv();
	setupTextInputJni(jEnv);
	logMsg("starting system text input");
	vKeyboardTextDelegate = callback;
	jStartSysTextInput(jEnv, jBaseActivity, jEnv->NewStringUTF(initialText), jEnv->NewStringUTF(promptText),
		textRect.x, textRect.y, textRect.xSize(), textRect.ySize(), fontSizePixels);
	return 0;
}

void cancelSysTextInput()
{
	using namespace Base;
	auto jEnv = eEnv();
	setupTextInputJni(jEnv);
	vKeyboardTextDelegate = {};
	jFinishSysTextInput(jEnv, jBaseActivity, 1);
}

void finishSysTextInput()
{
	using namespace Base;
	auto jEnv = eEnv();
	setupTextInputJni(jEnv);
	jFinishSysTextInput(jEnv, jBaseActivity, 0);
}

void placeSysTextInput(const Rect2<int> &rect)
{
	using namespace Base;
	auto jEnv = eEnv();
	setupTextInputJni(jEnv);
	textRect = rect;
	jPlaceSysTextInput(jEnv, jBaseActivity, rect.x, rect.y, rect.xSize(), rect.ySize());
}

const Rect2<int> &sysTextInputRect()
{
	return textRect;
}

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if(typeBits & TYPE_BIT_KEYBOARD)
	{
		if(Base::keyboardType() == ACONFIGURATION_KEYBOARD_QWERTY)
		{
			if(Base::hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_YES || Base::hardKeyboardState() == ACONFIGURATION_KEYSHIDDEN_SOFT)
			{
				logMsg("keyboard present, but not in use");
			}
			else
			{
				logMsg("keyboard present");
				return 1;
			}
		}
		unsetBits(typeBits, TYPE_BIT_KEYBOARD); // ignore keyboards in device list
	}

	#ifdef __ARM_ARCH_7A__
	if(hasXperiaPlayGamepad() && //Base::runningDeviceType() == Base::DEV_TYPE_XPERIA_PLAY &&
		(typeBits & TYPE_BIT_GAMEPAD) && Base::hardKeyboardState() != ACONFIGURATION_KEYSHIDDEN_YES)
	{
		logMsg("Xperia-play gamepad in use");
		return 1;
	}
	#endif

	forEachInDLList(&Input::devList, e)
	{
		if((e.isVirtual() && ((typeBits & TYPE_BIT_KEY_MISC) & e.typeBits())) // virtual devices count as TYPE_BIT_KEY_MISC only
				|| (!e.isVirtual() && (e.typeBits() & typeBits)))
		{
			logMsg("device idx %d has bits 0x%X", e.idx, typeBits);
			return 1;
		}
	}
	return 0;
}

}
