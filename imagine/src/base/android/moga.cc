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

#define LOGTAG "MOGAInput"
#include <imagine/base/Base.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../../input/private.hh"
#include "AndroidInputDevice.hh"

namespace Input
{

static const int ACTION_VERSION_MOGAPRO = 1;
static const int STATE_CONNECTION = 1;
static const int STATE_SELECTED_VERSION = 4;
static jobject mogaHelper{};
static JavaInstMethod<jint(jint)> jMOGAGetState{};
static JavaInstMethod<void()> jMOGAOnPause{}, jMOGAOnResume{}, jMOGAExit{};
static AndroidInputDevice *mogaDev{};

static AndroidInputDevice makeMOGADevice(const char *name)
{
	AndroidInputDevice dev{0, Device::TYPE_BIT_GAMEPAD | Device::TYPE_BIT_JOYSTICK, name};
	dev.subtype_ = Device::SUBTYPE_GENERIC_GAMEPAD;
	dev.axisBits = Device::AXIS_BITS_STICK_1 | Device::AXIS_BITS_STICK_2;
	// set joystick axes
	{
		const uint8 stickAxes[] { AXIS_X, AXIS_Y, AXIS_Z, AXIS_RZ };
		for(auto axisId : stickAxes)
		{
			//logMsg("joystick axis: %d", axisId);
			auto size = 2.0f;
			dev.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f,
				Key(axisToKeycode(axisId)+1), axisToKeycode(axisId), Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
		}
	}
	// set trigger axes
	{
		const uint8 triggerAxes[] { AXIS_LTRIGGER, AXIS_RTRIGGER };
		for(auto axisId : triggerAxes)
		{
			//logMsg("trigger axis: %d", axisId);
			// use unreachable lowLimit value so only highLimit is used
			dev.axis.emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 0.25f,
				0, axisToKeycode(axisId), 0, axisToKeycode(axisId)});
		}
	}
	dev.joystickAxisAsDpadBitsDefault_ = Device::AXIS_BITS_STICK_1;
	dev.setJoystickAxisAsDpadBits(Device::AXIS_BITS_STICK_1);
	return dev;
}

static void updateMOGAState(JNIEnv *env, bool connected, bool notify)
{
	bool mogaConnected = mogaDev;
	if(connected != mogaConnected)
	{
		if(connected)
		{
			logMsg("MOGA connected");
			const char *name = jMOGAGetState(env, mogaHelper, STATE_SELECTED_VERSION) == ACTION_VERSION_MOGAPRO ? "MOGA Pro Controller" : "MOGA Controller";
			sysInputDev.reserve(2);
			addInputDevice(makeMOGADevice(name), false, notify);
			mogaDev = sysInputDev.back().get();
		}
		else
		{
			logMsg("MOGA disconnected");
			mogaDev = {};
			removeInputDevice(0, notify);
		}
	}
}

static void initMOGAJNIAndDevice(JNIEnv *env, jobject mogaHelper)
{
	if(jMOGAGetState)
		return;
	logMsg("init MOGA JNI");
	auto mogaHelperCls = env->GetObjectClass(mogaHelper);
	jMOGAGetState.setup(env, mogaHelperCls, "getState", "(I)I");
	jMOGAOnPause.setup(env, mogaHelperCls, "onPause", "()V");
	jMOGAOnResume.setup(env, mogaHelperCls, "onResume", "()V");
	jMOGAExit.setup(env, mogaHelperCls, "exit", "()V");
	JNINativeMethod method[]
	{
		{
			"keyEvent", "(IIJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jint, jint, jlong))
			([](JNIEnv* env, jobject thiz, jint action, jint keyCode, jlong timestamp)
			{
				assert(mogaDev);
				//logMsg("MOGA key event: %d %d %d", action, keyCode, (int)time);
				assert((uint)keyCode < Keycode::COUNT);
				Base::endIdleByUserActivity();
				Key key = keyCode & 0x1ff;
				auto time = Time::makeWithNSecs(timestamp);
				Event event{0, Event::MAP_SYSTEM, key, key, (action == AKEY_EVENT_ACTION_DOWN) ? PUSHED : RELEASED, 0, 0, time, mogaDev};
				startKeyRepeatTimer(event);
				Base::mainWindow().dispatchInputEvent(event);
			})
		},
		{
			"motionEvent", "(FFFFFFJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jfloat, jfloat, jfloat, jfloat, jfloat, jfloat, jlong))
			([](JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong timestamp)
			{
				assert(mogaDev);
				Base::endIdleByUserActivity();
				auto time = Time::makeWithNSecs(timestamp);
				logMsg("MOGA motion event: %f %f %f %f %f %f %d", (double)x, (double)y, (double)z, (double)rz, (double)lTrigger, (double)rTrigger, (int)timestamp);
				mogaDev->axis[0].keyEmu.dispatch(x, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
				mogaDev->axis[1].keyEmu.dispatch(y, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
				mogaDev->axis[2].keyEmu.dispatch(z, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
				mogaDev->axis[3].keyEmu.dispatch(rz, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
				mogaDev->axis[4].keyEmu.dispatch(lTrigger, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
				mogaDev->axis[5].keyEmu.dispatch(rTrigger, 0, Event::MAP_SYSTEM, time, *mogaDev, Base::mainWindow());
			})
		},
		{
			"stateEvent", "(II)V",
			(void*)(void (*)(JNIEnv*, jobject, jint, jint))
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
	env->RegisterNatives(mogaHelperCls, method, IG::size(method));
}

void initMOGA(bool notify)
{
	auto env = Base::jEnvForThread();
	if(mogaHelper)
		return;
	JavaInstMethod<jobject()> jNewMOGAHelper{env, Base::jBaseActivityCls, "mogaHelper", "()Lcom/imagine/MOGAHelper;"};
	mogaHelper = jNewMOGAHelper(env, Base::jBaseActivity);
	if(env->ExceptionCheck())
	{
		env->ExceptionClear();
		mogaHelper = nullptr;
		logErr("error creating MOGA helper object");
		return;
	}
	mogaHelper = env->NewGlobalRef(mogaHelper);
	initMOGAJNIAndDevice(env, mogaHelper);
	logMsg("init MOGA input system");
	onResumeMOGA(env, notify);
}

void deinitMOGA()
{
	if(!mogaHelper)
		return;
	logMsg("deinit MOGA input system");
	auto env = Base::jEnvForThread();
	jMOGAExit(env, mogaHelper);
	env->DeleteGlobalRef(mogaHelper);
	mogaHelper = {};
	removeInputDevice(0, true);
	mogaDev = {};
}

bool mogaSystemIsActive()
{
	return mogaHelper;
}

void onPauseMOGA(JNIEnv *env)
{
	if(mogaHelper)
		jMOGAOnPause(env, mogaHelper);
}

void onResumeMOGA(JNIEnv *env, bool notify)
{
	if(mogaHelper)
	{
		jMOGAOnResume(env, mogaHelper);
		bool isConnected = jMOGAGetState(env, mogaHelper, STATE_CONNECTION);
		logMsg("checked MOGA connection state: %s", isConnected ? "yes" : "no");
		updateMOGAState(env, isConnected, notify);
	}
}

}
