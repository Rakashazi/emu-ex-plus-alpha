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
#include <imagine/base/android/AndroidInputDevice.hh>
#include <imagine/input/android/MogaManager.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include <android/input.h>

namespace IG::Input
{

static constexpr int ACTION_VERSION_MOGAPRO = 1;
static constexpr int STATE_CONNECTION = 1;
static constexpr int STATE_SELECTED_VERSION = 4;
static constexpr int DEVICE_ID = -128; // arbitrary value to avoid collisions

MogaManager::MogaManager(ApplicationContext ctx, bool notify):
	onExit
	{
		[this, env = ctx.mainThreadJniEnv()](ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				jMOGAOnPause(env, mogaHelper);
				ctx.addOnResume(
					[this, env](ApplicationContext, bool)
					{
						onResumeMOGA(env, true);
						return false;
					}, INPUT_DEVICE_ON_RESUME_PRIORITY
				);
			}
			return true;
		}, ctx, INPUT_DEVICE_ON_EXIT_PRIORITY
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
	appContext().application().removeInputDevice(appContext(), Input::Map::SYSTEM, DEVICE_ID, false);
}

std::unique_ptr<Input::Device> MogaManager::makeMOGADevice(const char *name)
{
	auto devPtr = std::make_unique<Input::Device>(std::in_place_type<AndroidInputDevice>, DEVICE_ID, InputDeviceTypeFlags{.gamepad = true, .joystick = true}, name);
	devPtr->setSubtype(DeviceSubtype::GENERIC_GAMEPAD);
	auto &axes = getAs<AndroidInputDevice>(*devPtr).jsAxes();
	// set joystick axes
	{
		static constexpr AxisId stickAxes[] { AxisId::X, AxisId::Y, AxisId::Z, AxisId::RZ };
		for(auto axisId : stickAxes)
		{
			axes.emplace_back(axisId);
		}
	}
	// set trigger axes
	{
		static constexpr AxisId triggerAxes[] { AxisId::LTRIGGER, AxisId::RTRIGGER };
		for(auto axisId : triggerAxes)
		{
			axes.emplace_back(axisId);
		}
	}
	return devPtr;
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
			mogaDev = app.addAndroidInputDevice(appContext(), makeMOGADevice(name), notify);
		}
		else
		{
			logMsg("MOGA disconnected");
			mogaDev = {};
			app.removeInputDevice(appContext(), Input::Map::SYSTEM, DEVICE_ID, notify);
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
			([](JNIEnv*, jobject, jlong mogaManagerPtr, jint action, jint keyCode, jlong timestamp)
			{
				auto &mogaManager = *((MogaManager*)mogaManagerPtr);
				auto &mogaDev = *mogaManager.mogaDev;
				auto ctx = mogaManager.appContext();
				//logMsg("MOGA key event: %d %d %d", action, keyCode, (int)time);
				assert((uint32_t)keyCode < Keycode::COUNT);
				ctx.endIdleByUserActivity();
				Key key = keyCode & 0x1ff;
				auto time = SteadyClockTimePoint{Nanoseconds{timestamp}};
				KeyEvent event{Map::SYSTEM, key, (action == AKEY_EVENT_ACTION_DOWN) ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &mogaDev};
				ctx.application().dispatchRepeatableKeyInputEvent(event);
			})
		},
		{
			"motionEvent", "(JFFFFFFJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jfloat, jfloat, jfloat, jfloat, jfloat, jfloat, jlong))
			([](JNIEnv*, jobject, jlong mogaManagerPtr, jfloat x, jfloat y, jfloat z, jfloat rz, jfloat lTrigger, jfloat rTrigger, jlong timestamp)
			{
				auto &mogaManager = *((MogaManager*)mogaManagerPtr);
				auto &mogaDev = *mogaManager.mogaDev;
				auto ctx = mogaManager.appContext();
				ctx.endIdleByUserActivity();
				auto time = SteadyClockTimePoint{Nanoseconds{timestamp}};
				logMsg("MOGA motion event: %f %f %f %f %f %f %d", (double)x, (double)y, (double)z, (double)rz, (double)lTrigger, (double)rTrigger, (int)timestamp);
				auto &win = ctx.mainWindow();
				auto axis = mogaDev.motionAxes();
				axis[0].update(x, Map::SYSTEM, time, mogaDev, win, true);
				axis[1].update(y, Map::SYSTEM, time, mogaDev, win, true);
				axis[2].update(z, Map::SYSTEM, time, mogaDev, win, true);
				axis[3].update(rz, Map::SYSTEM, time, mogaDev, win, true);
				axis[4].update(lTrigger, Map::SYSTEM, time, mogaDev, win, true);
				axis[5].update(rTrigger, Map::SYSTEM, time, mogaDev, win, true);
			})
		},
		{
			"stateEvent", "(JII)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint))
			([](JNIEnv* env, jobject, jlong mogaManagerPtr, jint state, jint action)
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

ApplicationContext MogaManager::appContext() const
{
	return onExit.appContext();
}

}
