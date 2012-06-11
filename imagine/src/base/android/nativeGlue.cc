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

// Based on android_native_app_glue

#define thisModuleName "base:android"
#include <jni.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include "nativeGlue.hh"
#include "sdk.hh"
#include "private.hh"
#include <util/builtins.h>
#include <logger/interface.h>
#include <assert.h>

namespace Input
{
int32_t onInputEvent(android_app* app, AInputEvent* event);
extern fbool sendInputToIME;
}

static struct android_app aAppInst; // global app instance

namespace Base
{

android_app *appInstance()
{
	return &aAppInst;
}

}

using namespace Base;

static void free_saved_state(struct android_app* android_app)
{
	/*pthread_mutex_lock(&android_app->mutex);
	if (android_app->savedState != NULL) {
			free(android_app->savedState);
			android_app->savedState = NULL;
			android_app->savedStateSize = 0;
	}
	pthread_mutex_unlock(&android_app->mutex);*/
}

static uint32 android_app_read_cmd(struct android_app* android_app)
{
	uint32 cmd;
	if(read(android_app->msgread, &cmd, sizeof(cmd)) == sizeof(cmd))
	{
		switch (cmd)
		{
			case APP_CMD_SAVE_STATE:
				free_saved_state(android_app);
				break;
		}
		return cmd;
	}

	logWarn("No data on command pipe from read()");
	return -1;
}

static void printAppConfig(struct android_app* android_app) {
    char lang[2], country[2];
    AConfiguration_getLanguage(android_app->config, lang);
    AConfiguration_getCountry(android_app->config, country);

    logMsg("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
            "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
            "modetype=%d modenight=%d",
            AConfiguration_getMcc(android_app->config),
            AConfiguration_getMnc(android_app->config),
            lang[0], lang[1], country[0], country[1],
            AConfiguration_getOrientation(android_app->config),
            AConfiguration_getTouchscreen(android_app->config),
            AConfiguration_getDensity(android_app->config),
            AConfiguration_getKeyboard(android_app->config),
            AConfiguration_getNavigation(android_app->config),
            AConfiguration_getKeysHidden(android_app->config),
            AConfiguration_getNavHidden(android_app->config),
            AConfiguration_getSdkVersion(android_app->config),
            AConfiguration_getScreenSize(android_app->config),
            AConfiguration_getScreenLong(android_app->config),
            AConfiguration_getUiModeType(android_app->config),
            AConfiguration_getUiModeNight(android_app->config));
}

static void android_app_pre_exec_cmd(struct android_app* android_app, uint32 cmd) {
    switch (cmd) {
        case APP_CMD_INPUT_CHANGED:
        	//logMsg("APP_CMD_INPUT_CHANGED");
            pthread_mutex_lock(&android_app->mutex);
            if (android_app->inputQueue != NULL) {
                AInputQueue_detachLooper(android_app->inputQueue);
            }
            android_app->inputQueue = android_app->pendingInputQueue;
            if (android_app->inputQueue != NULL) {
            	logMsg("Attaching input queue to looper");
                AInputQueue_attachLooper(android_app->inputQueue,
                        android_app->looper, LOOPER_ID_INPUT, NULL, 0);
            }
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_INIT_WINDOW:
        	//logMsg("APP_CMD_INIT_WINDOW");
            pthread_mutex_lock(&android_app->mutex);
            android_app->window = android_app->pendingWindow;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_TERM_WINDOW:
        	logMsg("APP_CMD_TERM_WINDOW");
            pthread_cond_broadcast(&android_app->cond);
            break;

        case APP_CMD_RESUME:
        case APP_CMD_START:
        case APP_CMD_PAUSE:
        case APP_CMD_STOP:
        	//logMsg("activityState=%d", cmd);
            pthread_mutex_lock(&android_app->mutex);
            android_app->activityState = cmd;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_CONFIG_CHANGED:
        	logMsg("APP_CMD_CONFIG_CHANGED");
            AConfiguration_fromAssetManager(android_app->config,
                    android_app->activity->assetManager);
            //print_cur_config(android_app);
            break;

        /*case APP_CMD_DESTROY:
        	logMsg("APP_CMD_DESTROY");
            android_app->destroyRequested = 1;
            break;*/
    }
}

static void android_app_post_exec_cmd(struct android_app* android_app, uint32 cmd) {
    switch (cmd) {
        case APP_CMD_TERM_WINDOW:
        	logMsg("APP_CMD_TERM_WINDOW");
            pthread_mutex_lock(&android_app->mutex);
            android_app->window = NULL;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_SAVE_STATE:
        	logMsg("APP_CMD_SAVE_STATE");
            pthread_mutex_lock(&android_app->mutex);
            android_app->stateSaved = 1;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_RESUME:
            free_saved_state(android_app);
            break;
    }
}

/*static void android_app_destroy(struct android_app* android_app) {
	logMsg("android_app_destroy!");
    free_saved_state(android_app);
    pthread_mutex_lock(&android_app->mutex);
    if (android_app->inputQueue != NULL) {
        AInputQueue_detachLooper(android_app->inputQueue);
    }
    AConfiguration_delete(android_app->config);
    android_app->destroyed = 1;
    pthread_cond_broadcast(&android_app->cond);
    pthread_mutex_unlock(&android_app->mutex);
    // Can't touch android_app object after this.
}*/

void process_input(struct android_app* app)
{
	AInputEvent* event = NULL;
	if(AInputQueue_getEvent(app->inputQueue, &event) >= 0)
	{
		//LOGI("New input event: type=%d\n", AInputEvent_getType(event));
		if(Input::sendInputToIME && AInputQueue_preDispatchEvent(app->inputQueue, event))
		{
			return;
		}
		int32_t handled = Input::onInputEvent(app, event);
		AInputQueue_finishEvent(app->inputQueue, event, handled);
	}
	else
	{
		logWarn("Failure reading next input event: %s\n", strerror(errno));
	}
}

void process_cmd(struct android_app* app)
{
	uint32 cmd = android_app_read_cmd(app);
	android_app_pre_exec_cmd(app, cmd);
	Base::onAppCmd(app, cmd);
	android_app_post_exec_cmd(app, cmd);
}

void* android_app_entry(void* param)
{
	struct android_app* android_app = (struct android_app*)param;

	android_app->config = AConfiguration_new();
	AConfiguration_fromAssetManager(android_app->config, android_app->activity->assetManager);

	#ifndef NDEBUG
	printAppConfig(android_app);
	#endif

	ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	ALooper_addFd(looper, android_app->msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL, 0);
	android_app->looper = looper;

	pthread_mutex_lock(&android_app->mutex);
	android_app->running = 1;
	pthread_cond_broadcast(&android_app->cond);
	pthread_mutex_unlock(&android_app->mutex);

	android_main(android_app);

	//android_app_destroy(android_app);
	return NULL;
}

// --------------------------------------------------------------------
// Native activity interaction (called from main thread)
// --------------------------------------------------------------------

static struct android_app* android_app_create(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	struct android_app* android_app = &aAppInst;
	memset(android_app, 0, sizeof(struct android_app));
	android_app->activity = activity;

	pthread_mutex_init(&android_app->mutex, NULL);
	pthread_cond_init(&android_app->cond, NULL);

	/*if (savedState != NULL)
	{
		android_app->savedState = malloc(savedStateSize);
		android_app->savedStateSize = savedStateSize;
		memcpy(android_app->savedState, savedState, savedStateSize);
	}*/

	int msgpipe[2];
	{
		int pipeSuccess = pipe(msgpipe);
		assert(pipeSuccess == 0);
	}
	android_app->msgread = msgpipe[0];
	android_app->msgwrite = msgpipe[1];

	// thread created after java setup

	return android_app;
}

static void android_app_write_cmd(struct android_app* android_app, uint32 cmd)
{
	if(write(android_app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd))
	{
		logWarn("Failure writing android_app cmd: %s", strerror(errno));
	}
}

static void android_app_set_input(struct android_app* android_app, AInputQueue* inputQueue)
{
	pthread_mutex_lock(&android_app->mutex);
	android_app->pendingInputQueue = inputQueue;
	android_app_write_cmd(android_app, APP_CMD_INPUT_CHANGED);
	while (android_app->inputQueue != android_app->pendingInputQueue)
	{
		pthread_cond_wait(&android_app->cond, &android_app->mutex);
	}
	pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_set_window(struct android_app* android_app, ANativeWindow* window)
{
	pthread_mutex_lock(&android_app->mutex);
	if (android_app->pendingWindow != NULL)
	{
		android_app_write_cmd(android_app, APP_CMD_TERM_WINDOW);
	}
	android_app->pendingWindow = window;
	if (window != NULL)
	{
		android_app_write_cmd(android_app, APP_CMD_INIT_WINDOW);
	}
	while (android_app->window != android_app->pendingWindow)
	{
		pthread_cond_wait(&android_app->cond, &android_app->mutex);
	}
	pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_set_activity_state(struct android_app* android_app, uint32 cmd)
{
	pthread_mutex_lock(&android_app->mutex);
	android_app_write_cmd(android_app, cmd);
	while (android_app->activityState != cmd)
	{
		pthread_cond_wait(&android_app->cond, &android_app->mutex);
	}
	pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_free(struct android_app* android_app)
{
	//pthread_mutex_lock(&android_app->mutex);
	android_app_write_cmd(android_app, APP_CMD_DESTROY);
	/*while (!android_app->destroyed)
	{
		pthread_cond_wait(&android_app->cond, &android_app->mutex);
	}
	pthread_mutex_unlock(&android_app->mutex);

	close(android_app->msgread);
	close(android_app->msgwrite);
	pthread_cond_destroy(&android_app->cond);
	pthread_mutex_destroy(&android_app->mutex);
	free(android_app);*/
}

static void onDestroy(ANativeActivity* activity)
{
	//logMsg("Destroy: %p", activity);
	android_app_free(appInstance());
}

static void onStart(ANativeActivity* activity)
{
	//logMsg("Start: %p", activity);
	android_app_set_activity_state(appInstance(), APP_CMD_START);
}

static void onResume(ANativeActivity* activity)
{
	//logMsg("Resume: %p", activity);
	android_app_set_activity_state(appInstance(), APP_CMD_RESUME);
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen)
{
	struct android_app* android_app = appInstance();
	void* savedState = NULL;

	//logMsg("SaveInstanceState: %p", activity);
	pthread_mutex_lock(&android_app->mutex);
	android_app->stateSaved = 0;
	android_app_write_cmd(android_app, APP_CMD_SAVE_STATE);
	while (!android_app->stateSaved)
	{
		pthread_cond_wait(&android_app->cond, &android_app->mutex);
	}

	/*if (android_app->savedState != NULL)
	{
		savedState = android_app->savedState;
		*outLen = android_app->savedStateSize;
		android_app->savedState = NULL;
		android_app->savedStateSize = 0;
	}*/

	pthread_mutex_unlock(&android_app->mutex);

	return savedState;
}

static void onPause(ANativeActivity* activity)
{
	//logMsg("Pause: %p", activity);
	android_app_set_activity_state(appInstance(), APP_CMD_PAUSE);
}

static void onStop(ANativeActivity* activity)
{
	//logMsg("Stop: %p", activity);
	android_app_set_activity_state(appInstance(), APP_CMD_STOP);
}

static void onConfigurationChanged(ANativeActivity* activity)
{
	struct android_app* android_app = appInstance();
	//logMsg("ConfigurationChanged: %p", activity);
	android_app_write_cmd(android_app, APP_CMD_CONFIG_CHANGED);
}

static void onLowMemory(ANativeActivity* activity)
{
	struct android_app* android_app = appInstance();
	//logMsg("LowMemory: %p", activity);
	android_app_write_cmd(android_app, APP_CMD_LOW_MEMORY);
}

static void onWindowFocusChanged(ANativeActivity* activity, int focused)
{
	//logMsg("WindowFocusChanged: %p -- %d", activity, focused);
	android_app_write_cmd(appInstance(), focused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
{
	//logMsg("NativeWindowCreated: %p -- %p", activity, window);
	android_app_set_window(appInstance(), window);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
{
	//logMsg("NativeWindowDestroyed: %p -- %p", activity, window);
	struct android_app* app = appInstance();
	android_app_set_window(app, NULL);
	pthread_mutex_lock(&app->mutex);
	if(Base::windowIsDrawable())
	{
		logMsg("waiting for app thread to release surface...");
		pthread_cond_wait(&app->cond, &app->mutex);
		logMsg("...got signal");
	}
	pthread_mutex_unlock(&app->mutex);
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
{
	//logMsg("InputQueueCreated: %p -- %p", activity, queue);
	android_app_set_input(appInstance(), queue);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
{
	//logMsg("InputQueueDestroyed: %p -- %p", activity, queue);
	android_app_set_input(appInstance(), NULL);
}

static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window)
{
	//logMsg("NativeWindowResized: %p -- %p", activity, window);
	android_app_write_cmd(appInstance(), APP_CMD_WINDOW_RESIZED);
}

static void onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window)
{
	//logMsg("NativeWindowRedrawNeeded: %p -- %p", activity, window);
	android_app_write_cmd(appInstance(), APP_CMD_WINDOW_REDRAW_NEEDED);
}

static void onContentRectChanged(ANativeActivity* activity, const ARect* rect)
{
	struct android_app* app = appInstance();
	app->contentRect = *rect;
	android_app_write_cmd(app, APP_CMD_CONTENT_RECT_CHANGED);
}

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	logMsg("called ANativeActivity_onCreate");
	activity->callbacks->onDestroy = onDestroy;
	activity->callbacks->onStart = onStart;
	activity->callbacks->onResume = onResume;
	activity->callbacks->onSaveInstanceState = onSaveInstanceState;
	activity->callbacks->onPause = onPause;
	activity->callbacks->onStop = onStop;
	activity->callbacks->onConfigurationChanged = onConfigurationChanged;
	activity->callbacks->onLowMemory = onLowMemory;
	activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
	activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
	activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
	activity->callbacks->onNativeWindowResized = onNativeWindowResized;
	activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
	activity->callbacks->onInputQueueCreated = onInputQueueCreated;
	activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
	activity->callbacks->onContentRectChanged = onContentRectChanged;

	//LOGI("sdk: %d, internal path: %s, external path: %s", activity->sdkVersion, activity->internalDataPath, activity->externalDataPath);
	using namespace Base;
	setSDK(activity->sdkVersion);
	jniInit(activity->env, activity->clazz);
	activity->instance = android_app_create(activity, savedState, savedStateSize);
}
