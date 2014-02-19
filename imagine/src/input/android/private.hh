#pragma once

#include <jni.h>
#include <android/input.h>
#include <base/Window.hh>

namespace Input
{

// JNI callbacks
jboolean JNICALL touchEvent(JNIEnv *env, jobject thiz, jint action, jint x, jint y, jint pid);
jboolean JNICALL trackballEvent(JNIEnv *env, jobject thiz, jint action, jfloat x, jfloat y);
jboolean JNICALL keyEvent(JNIEnv *env, jobject thiz, jint key, jint down, jboolean metaState);

// dlsym extra functions from supplied libandroid.so
bool dlLoadAndroidFuncs(void *libandroid);

// EditText-based Input
void textInputEndedMsg(const char* str, jstring jStr);

void devicesChanged(JNIEnv* jEnv);
void devicesChanged();

bool hasXperiaPlayGamepad();

void setBuiltInKeyboardState(bool shown);

int32_t onInputEvent(AInputEvent* event, Base::Window &win);
extern bool sendInputToIME;

void onPauseMOGA(JNIEnv *jEnv);
void onResumeMOGA(JNIEnv *jEnv, bool notify);

}
