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

#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <android/bitmap.h>

namespace JNI
{

jmethodID getJNIStaticMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig)
{
	if(!cls)
	{
		bug_unreachable("class missing for java static method: %s (%s)", fName, sig);
	}
	auto method = env->GetStaticMethodID(cls, fName, sig);
	if(!method)
	{
		logErr("java static method not found: %s (%s)", fName, sig);
	}
	return method;
}

jmethodID getJNIMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig)
{
	if(!cls)
	{
		bug_unreachable("class missing for java method: %s (%s)", fName, sig);
	}
	auto method = env->GetMethodID(cls, fName, sig);
	if(!method)
	{
		logErr("java method not found: %s (%s)", fName, sig);
	}
	//logDMsg("%s = %p", fName, method);
	return method;
}

UniqueGlobalRef::UniqueGlobalRef(JNIEnv *env_, jobject obj_):
	obj{env_->NewGlobalRef(obj_), {env_}} {}

void UniqueGlobalRef::deleteGlobalRef(JNIEnv *env, jobject obj)
{
	env->DeleteGlobalRef(obj);
}

StringChars::StringChars(JNIEnv *env, jstring jStr):
	str{env->GetStringUTFChars(jStr, nullptr), {env, jStr}} {}

void StringChars::releaseStringChars(JNIEnv *env, jstring jStr, const char *charsPtr)
{
	env->ReleaseStringUTFChars(jStr, charsPtr);
}

void LockedLocalBitmap::deleteBitmap(JNIEnv *env, jobject bitmap, JNI::InstMethod<void()> recycle)
{
	AndroidBitmap_unlockPixels(env, bitmap);
	recycle(env, bitmap);
	env->DeleteLocalRef(bitmap);
}

}
