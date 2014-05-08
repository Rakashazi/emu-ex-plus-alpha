/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "InputAndroidMOGA"
#include <imagine/base/Base.hh>
#include "../../base/android/private.hh"
#include "private.hh"
#include "../private.hh"
#include "AndroidInputDevice.hh"

namespace Input
{

static const int ACTION_VERSION_MOGAPRO = 1;
static const int STATE_CONNECTION = 1;
static const int STATE_SELECTED_VERSION = 4;
static jobject mogaHelper = nullptr;
static JavaInstMethod<jobject> jNewMOGAHelper;
static JavaInstMethod<jint> jMOGAGetState;
static JavaInstMethod<void> jMOGAOnPause, jMOGAOnResume, jMOGAExit;
static AndroidInputDevice mogaDev { 0, Device::TYPE_BIT_GAMEPAD | Device::TYPE_BIT_JOYSTICK };
static bool mogaConnected = false;

static void JNICALL mogaMotionEvent(JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong time)
{
	assert(mogaConnected);
	Base::endIdleByUserActivity();
	logMsg("MOGA motion event: %f %f %f %f %f %f %d", (double)x, (double)y, (double)z, (double)rz, (double)lTrigger, (double)rTrigger, (int)time);
	mogaDev.axis[0].keyEmu.dispatch(x, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
	mogaDev.axis[1].keyEmu.dispatch(y, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
	mogaDev.axis[2].keyEmu.dispatch(z, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
	mogaDev.axis[3].keyEmu.dispatch(rz, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
	mogaDev.axis[4].keyEmu.dispatch(lTrigger, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
	mogaDev.axis[5].keyEmu.dispatch(rTrigger, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
}

static void updateMOGAState(JNIEnv *jEnv, bool connected, bool notify)
{
	if(connected != mogaConnected)
	{
		if(connected)
		{
			logMsg("MOGA connected");
			string_copy(mogaDev.nameStr, jMOGAGetState(jEnv, mogaHelper, STATE_SELECTED_VERSION) == ACTION_VERSION_MOGAPRO ? "MOGA Pro Controller" : "MOGA Controller");
			Input::addDevice(mogaDev);
			if(notify && onDeviceChange)
				onDeviceChange(mogaDev, {Device::Change::ADDED});
		}
		else
		{
			logMsg("MOGA disconnected");
			Input::removeDevice(mogaDev);
			if(notify && onDeviceChange)
				onDeviceChange(mogaDev, {Device::Change::REMOVED});
		}
		mogaConnected = connected;
	}
}

static void initMOGAJNI(JNIEnv *jEnv)
{
	if(jNewMOGAHelper)
		return;
	logMsg("init MOGA JNI & input system");
	jNewMOGAHelper.setup(jEnv, Base::jBaseActivityCls, "mogaHelper", "()Lcom/imagine/MOGAHelper;");
	mogaHelper = jNewMOGAHelper(jEnv, Base::jBaseActivity);
	assert(mogaHelper);
	mogaHelper = jEnv->NewGlobalRef(mogaHelper);
	auto mogaHelperCls = jEnv->GetObjectClass(mogaHelper);
	jMOGAGetState.setup(jEnv, mogaHelperCls, "getState", "(I)I");
	jMOGAOnPause.setup(jEnv, mogaHelperCls, "onPause", "()V");
	jMOGAOnResume.setup(jEnv, mogaHelperCls, "onResume", "()V");
	jMOGAExit.setup(jEnv, mogaHelperCls, "exit", "()V");
	JNINativeMethod method[] =
	{
		{
			"keyEvent", "(IIJ)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jint action, jint keyCode, jlong time))
			([](JNIEnv* env, jobject thiz, jint action, jint keyCode, jlong time)
			{
				assert(mogaConnected);
				//logMsg("MOGA key event: %d %d %d", action, keyCode, (int)time);
				assert((uint)keyCode < Keycode::COUNT);
				Base::endIdleByUserActivity();
				Event event{0, Event::MAP_SYSTEM, Key(keyCode & 0xff), (action == AKEY_EVENT_ACTION_DOWN) ? PUSHED : RELEASED, 0, 0, &mogaDev};
				startKeyRepeatTimer(event);
				Base::mainWindow().onInputEvent(Base::mainWindow(), event);
			})
		},
		{
			"motionEvent", "(FFFFFFJ)V",
			(void*)mogaMotionEvent
			// TODO: can't inline as lambda because gcc 4.8 seems to ignore JNICALL's hard-float attribute
			/*(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong time))
			([](JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong time)
			{
				assert(mogaConnected);
				logMsg("MOGA motion event: %f %f %f %f %f %f %d", (double)x, (double)y, (double)z, (double)rz, (double)lTrigger, (double)rTrigger, (int)time);
				mogaDev.axis[0].keyEmu.dispatch(x, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
				mogaDev.axis[1].keyEmu.dispatch(y, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
				mogaDev.axis[2].keyEmu.dispatch(z, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
				mogaDev.axis[3].keyEmu.dispatch(rz, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
				mogaDev.axis[4].keyEmu.dispatch(lTrigger, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
				mogaDev.axis[5].keyEmu.dispatch(rTrigger, 0, Event::MAP_SYSTEM, mogaDev, Base::mainWindow());
			})*/
		},
		{
			"stateEvent", "(II)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jint state, jint action))
			([](JNIEnv* env, jobject thiz, jint state, jint action)
			{
				logMsg("MOGA state event: %d %d", state, action);
				if(state == STATE_CONNECTION)
				{
					updateMOGAState(env, action, true); // "action" maps directly to boolean type
				}
			})
		}
	};
	jEnv->RegisterNatives(mogaHelperCls, method, sizeofArray(method));

	mogaDev.subtype_ = Input::Device::SUBTYPE_GENERIC_GAMEPAD;
	mogaDev.axisBits = Device::AXIS_BITS_STICK_1 | Device::AXIS_BITS_STICK_2;
	// set joystick axes
	{
		const uint8 stickAxes[] { AXIS_X, AXIS_Y, AXIS_Z, AXIS_RZ };
		for(auto axisId : stickAxes)
		{
			//logMsg("joystick axis: %d", axisId);
			auto size = 2.0f;
			mogaDev.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f, Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
		}
	}
	// set trigger axes
	{
		const uint8 triggerAxes[] { AXIS_LTRIGGER, AXIS_RTRIGGER };
		for(auto axisId : triggerAxes)
		{
			//logMsg("trigger axis: %d", axisId);
			// use unreachable lowLimit value so only highLimit is used
			mogaDev.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 0.25f, 0, axisToKeycode(axisId)});
		}
	}
	mogaDev.joystickAxisAsDpadBitsDefault_ = Device::AXIS_BITS_STICK_1;
	mogaDev.setJoystickAxisAsDpadBits(Device::AXIS_BITS_STICK_1);
}

void initMOGA(bool notify)
{
	auto jEnv = Base::eEnv();
	if(mogaHelper)
		return;
	initMOGAJNI(jEnv);
	if(!mogaHelper) // initMOGAJNI() allocates MOGA helper on first init
	{
		logMsg("init MOGA input system");
		mogaHelper = jEnv->NewGlobalRef(jNewMOGAHelper(jEnv, Base::jBaseActivity));
	}
	onResumeMOGA(jEnv, notify);
}

void deinitMOGA()
{
	if(!mogaHelper)
		return;
	logMsg("deinit MOGA input system");
	auto jEnv = Base::eEnv();
	jMOGAExit(jEnv, mogaHelper);
	jEnv->DeleteGlobalRef(mogaHelper);
	mogaHelper = nullptr;
	if(contains(devList, &mogaDev))
	{
		mogaConnected = false;
		Input::removeDevice(mogaDev);
		if(onDeviceChange)
			onDeviceChange(mogaDev, { Device::Change::REMOVED });
	}
}

bool mogaSystemIsActive()
{
	return mogaHelper;
}

void onPauseMOGA(JNIEnv *jEnv)
{
	if(mogaHelper)
		jMOGAOnPause(jEnv, mogaHelper);
}

void onResumeMOGA(JNIEnv *jEnv, bool notify)
{
	if(mogaHelper)
	{
		jMOGAOnResume(jEnv, mogaHelper);
		bool isConnected = jMOGAGetState(jEnv, mogaHelper, STATE_CONNECTION);
		logMsg("checked MOGA connection state: %s", isConnected ? "yes" : "no");
		updateMOGAState(jEnv, isConnected, notify);
	}
}

}
