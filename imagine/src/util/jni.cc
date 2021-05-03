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

// method call specializations

template<>
void callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	env->CallVoidMethodV(obj, method, args);
}

template<>
jobject callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	//logMsg("calling object method:%p", method);
	return env->CallObjectMethodV(obj, method, args);
}

template<>
jboolean callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallBooleanMethodV(obj, method, args);
}

template<>
jbyte callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallByteMethodV(obj, method, args);
}

template<>
jchar callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallCharMethodV(obj, method, args);
}

template<>
jshort callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallShortMethodV(obj, method, args);
}

template<>
jint callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallIntMethodV(obj, method, args);
}

template<>
jlong callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallLongMethodV(obj, method, args);
}

template<>
jfloat callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallFloatMethodV(obj, method, args);
}

template<>
jdouble callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallDoubleMethodV(obj, method, args);
}

template<>
void callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	env->CallStaticVoidMethodV(cls, method, args);
}

template<>
jobject callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticObjectMethodV(cls, method, args);
}

template<>
jboolean callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticBooleanMethodV(cls, method, args);
}

template<>
jbyte callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticByteMethodV(cls, method, args);
}

template<>
jchar callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticCharMethodV(cls, method, args);
}

template<>
jshort callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticShortMethodV(cls, method, args);
}

template<>
jint callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticIntMethodV(cls, method, args);
}

template<>
jlong callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticLongMethodV(cls, method, args);
}

template<>
jfloat callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticFloatMethodV(cls, method, args);
}

template<>
jdouble callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticDoubleMethodV(cls, method, args);
}

UniqueGlobalRef::UniqueGlobalRef(JNIEnv *env_, jobject obj_):
	env{env_}
{
	assert(obj_);
	obj = env_->NewGlobalRef(obj_);
	assert(obj);
}

UniqueGlobalRef::UniqueGlobalRef(UniqueGlobalRef &&o)
{
	*this = std::move(o);
}

UniqueGlobalRef &UniqueGlobalRef::operator=(UniqueGlobalRef &&o)
{
	reset();
	env = o.env;
	obj = std::exchange(o.obj, {});
	return *this;
}

UniqueGlobalRef::~UniqueGlobalRef()
{
	reset();
}

void UniqueGlobalRef::reset()
{
	if(!obj)
		return;
	env->DeleteGlobalRef(std::exchange(obj, {}));
}

}
