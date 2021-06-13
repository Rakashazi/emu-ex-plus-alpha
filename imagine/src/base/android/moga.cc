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
#include <imagine/base/Application.hh>
#include <imagine/input/android/MogaManager.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include "input.hh"
#include <android/input.h>

namespace Input
{

static const int ACTION_VERSION_MOGAPRO = 1;
static const int STATE_CONNECTION = 1;
static const int STATE_SELECTED_VERSION = 4;

MogaManager::MogaManager(Base::ApplicationContext ctx, bool notify):
	onExit
	{
		[this, env = ctx.mainThreadJniEnv()](Base::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				jMOGAOnPause(env, mogaHelper);
				ctx.addOnResume(
					[this, env](Base::ApplicationContext, bool)
					{
						onResumeMOGA(env, true);
						return false;
					}, Base::INPUT_DEVICE_ON_RESUME_PRIORITY
				);
			}
			return true;
		}, ctx, Base::INPUT_DEVICE_ON_EXIT_PRIORITY
	}
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	JNI::InstMethod<jobject(jlong)> jNewMOGAHelper{env, baseActivity, "mogaHelper", "(J)Lcom/imagine/MOGAHelper;"};
	auto newMogaHelper = jNewMOGAHelper(env, baseActivity, (jlong)this);
	if(env->ExceptionCheck())
	{
		env->ExceptionClear();
		logErr("error creating MOGA helper object");
		return;
	}
	mogaHelper = {env, newMogaHelper};
	initMOGAJNIAndDevice(env, mogaHelper);
	logMsg("init MOGA input system");
	onResumeMOGA(env, notify);
}

MogaManager::~MogaManager()
{
	if(!mogaHelper)
		return;
	logMsg("deinit MOGA input system");
	jMOGAExit(mogaHelper.jniEnv(), mogaHelper);
	appContext().application().removeInputDevice(0, false);
}

AndroidInputDevice MogaManager::makeMOGADevice(const char *name)
{
	AndroidInputDevice dev{0, Device::TYPE_BIT_GAMEPAD | Device::TYPE_BIT_JOYSTICK, name,
		Device::AXIS_BITS_STICK_1 | Device::AXIS_BITS_STICK_2};
	dev.subtype_ = Device::SUBTYPE_GENERIC_GAMEPAD;
	// set joystick axes
	{
		const uint8_t stickAxes[] { AXIS_X, AXIS_Y, AXIS_Z, AXIS_RZ };
		for(auto axisId : stickAxes)
		{
			//logMsg("joystick axis: %d", axisId);
			auto size = 2.0f;
			dev.jsAxes().emplace_back(axisId, (AxisKeyEmu<float>){-1.f + size/4.f, 1.f - size/4.f,
				Key(axisToKeycode(axisId)+1), axisToKeycode(axisId), Key(axisToKeycode(axisId)+1), axisToKeycode(axisId)});
		}
	}
	// set trigger axes
	{
		const uint8_t triggerAxes[] { AXIS_LTRIGGER, AXIS_RTRIGGER };
		for(auto axisId : triggerAxes)
		{
			//logMsg("trigger axis: %d", axisId);
			// use unreachable lowLimit value so only highLimit is used
			dev.jsAxes().emplace_back(axisId, (AxisKeyEmu<float>){-1.f, 0.25f,
				0, axisToKeycode(axisId), 0, axisToKeycode(axisId)});
		}
	}
	dev.setJoystickAxisAsDpadBitsDefault(Device::AXIS_BITS_STICK_1);
	dev.setJoystickAxisAsDpadBits(Device::AXIS_BITS_STICK_1);
	return dev;
}

void MogaManager::updateMOGAState(JNIEnv *env, bool connected, bool notify)
{
	bool mogaConnected = mogaDev;
	if(connected != mogaConnected)
	{
		auto &app = appContext().application();
		if(connected)
		{
			logMsg("MOGA connected");
			const char *name = jMOGAGetState(env, mogaHelper, STATE_SELECTED_VERSION) == ACTION_VERSION_MOGAPRO ? "MOGA Pro Controller" : "MOGA Controller";
			mogaDev = app.addInputDevice(makeMOGADevice(name), false, notify);
		}
		else
		{
			logMsg("MOGA disconnected");
			mogaDev = {};
			app.removeInputDevice(0, notify);
		}
	}
}

void MogaManager::initMOGAJNIAndDevice(JNIEnv *env, jobject mogaHelper)
{
	logMsg("init MOGA JNI");
	auto mogaHelperCls = env->GetObjectClass(mogaHelper);
	jMOGAGetState = {env, mogaHelperCls, "getState", "(I)I"};
	jMOGAOnPause = {env, mogaHelperCls, "onPause", "()V"};
	jMOGAOnResume = {env, mogaHelperCls, "onResume", "()V"};
	jMOGAExit = {env, mogaHelperCls, "exit", "()V"};
	JNINativeMethod method[]
	{
		{
			"keyEvent", "(JIIJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint, jlong))
			([](JNIEnv* env, jobject thiz, jlong mogaManagerPtr, jint action, jint keyCode, jlong timestamp)
			{
				auto &mogaManager = *((MogaManager*)mogaManagerPtr);
				auto &mogaDev = *mogaManager.mogaDev;
				auto ctx = mogaManager.appContext();
				//logMsg("MOGA key event: %d %d %d", action, keyCode, (int)time);
				assert((uint32_t)keyCode < Keycode::COUNT);
				ctx.endIdleByUserActivity();
				Key key = keyCode & 0x1ff;
				auto time = IG::Nanoseconds(timestamp);
				Event event{0, Map::SYSTEM, key, key, (action == AKEY_EVENT_ACTION_DOWN) ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &mogaDev};
				ctx.application().dispatchRepeatableKeyInputEvent(event);
			})
		},
		{
			"motionEvent", "(JFFFFFFJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jfloat, jfloat, jfloat, jfloat, jfloat, jfloat, jlong))
			([](JNIEnv* env, jobject thiz, jlong mogaManagerPtr, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong timestamp)
			{
				auto &mogaManager = *((MogaManager*)mogaManagerPtr);
				auto &mogaDev = *mogaManager.mogaDev;
				auto ctx = mogaManager.appContext();
				ctx.endIdleByUserActivity();
				auto time = IG::Nanoseconds(timestamp);
				logMsg("MOGA motion event: %f %f %f %f %f %f %d", (double)x, (double)y, (double)z, (double)rz, (double)lTrigger, (double)rTrigger, (int)timestamp);
				auto &win = ctx.mainWindow();
				auto &axis = mogaDev.jsAxes();
				axis[0].keyEmu.dispatch(x, 0, Map::SYSTEM, time, mogaDev, win);
				axis[1].keyEmu.dispatch(y, 0, Map::SYSTEM, time, mogaDev, win);
				axis[2].keyEmu.dispatch(z, 0, Map::SYSTEM, time, mogaDev, win);
				axis[3].keyEmu.dispatch(rz, 0, Map::SYSTEM, time, mogaDev, win);
				axis[4].keyEmu.dispatch(lTrigger, 0, Map::SYSTEM, time, mogaDev, win);
				axis[5].keyEmu.dispatch(rTrigger, 0, Map::SYSTEM, time, mogaDev, win);
			})
		},
		{
			"stateEvent", "(JII)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint))
			([](JNIEnv* env, jobject thiz, jlong mogaManagerPtr, jint state, jint action)
			{
				logMsg("MOGA state event: %d %d", state, action);
				if(state == STATE_CONNECTION)
				{
					auto &mogaManager = *((MogaManager*)mogaManagerPtr);
					mogaManager.updateMOGAState(env, action, true); // "action" maps directly to boolean type
				}
			})
		}
	};
	env->RegisterNatives(mogaHelperCls, method, std::size(method));
}

void MogaManager::onResumeMOGA(JNIEnv *env, bool notify)
{
	jMOGAOnResume(env, mogaHelper);
	bool isConnected = jMOGAGetState(env, mogaHelper, STATE_CONNECTION);
	logMsg("checked MOGA connection state:%s", isConnected ? "yes" : "no");
	updateMOGAState(env, isConnected, notify);
}

Base::ApplicationContext MogaManager::appContext() const
{
	return onExit.appContext();
}

}
