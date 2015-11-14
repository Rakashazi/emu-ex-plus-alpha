#pragma once

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

#include <cstddef>
#include <jni.h>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include <assert.h>

template<typename R>
R callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args);

template <typename T> class JavaClassMethod {};

template <typename R, typename ...ARGS>
class JavaClassMethod<R(ARGS...)>
{
public:
	jmethodID method{};

	constexpr JavaClassMethod() {}

	JavaClassMethod(JNIEnv *env, jclass cls, const char *fName, const char *sig)
	{
		setup(env, cls, fName, sig);
	}

	bool setup(JNIEnv *env, jclass cls, const char *fName, const char *sig)
	{
		if(!cls)
		{
			bug_exit("class missing for java static method: %s (%s)", fName, sig);
		}
		method = env->GetStaticMethodID(cls, fName, sig);
		if(!method)
		{
			logErr("java static method not found: %s (%s)", fName, sig);
			return false;
		}
		return true;
	}

	explicit operator bool() const
	{
		return method;
	}

	R operator()(JNIEnv *env, jclass cls, ARGS ... args) const
	{
		return callMethod(env, cls, args...);
	}

private:
	R callMethod(JNIEnv *env, jclass cls, ...) const
	{
		assert(method);
		va_list args;
		va_start(args, cls);
		auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
		return callJNIStaticMethodV<R>(env, method, cls, args);
	}
};

template<typename R>
R callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args);

template <typename T> class JavaInstMethod {};

template <typename R, typename ...ARGS>
class JavaInstMethod<R(ARGS...)>
{
public:
	jmethodID method{};

	constexpr JavaInstMethod() {}

	JavaInstMethod(JNIEnv *env, jclass cls, const char *fName, const char *sig)
	{
		setup(env, cls, fName, sig);
	}

	bool setup(JNIEnv *env, jclass cls, const char *fName, const char *sig)
	{
		if(!cls)
		{
			bug_exit("class missing for java method: %s (%s)", fName, sig);
		}
		method = env->GetMethodID(cls, fName, sig);
		if(!method)
		{
			logErr("java method not found: %s (%s)", fName, sig);
			return false;
		}
		return true;
	}

	explicit operator bool() const
	{
		return method;
	}

	R operator()(JNIEnv *env, jobject obj, ARGS ... args) const
	{
		return callMethod(env, obj, args...);
	}

private:
	R callMethod(JNIEnv *env, jobject obj, ...) const
	{
		assert(method);
		va_list args;
		va_start(args, obj);
		auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
		return callJNIMethodV<R>(env, method, obj, args);
	}
};

class JObject
{
protected:
	jobject o{};

	constexpr JObject() {};
	constexpr JObject(jobject o): o(o) {};

public:
	operator jobject() const
	{
		return o;
	}
};

template <size_t S>
static void javaStringCopy(JNIEnv *env, std::array<char, S> &dest, jstring jstr)
{
	auto utfChars = env->GetStringUTFChars(jstr, nullptr);
	if(!utfChars)
	{
		return; // OutOfMemoryError thrown
	}
	string_copy(dest, utfChars);
	env->ReleaseStringUTFChars(jstr, utfChars);
}

// method call specializations

template<>
inline void callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	env->CallVoidMethodV(obj, method, args);
}

template<>
inline jobject callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallObjectMethodV(obj, method, args);
}

template<>
inline jboolean callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallBooleanMethodV(obj, method, args);
}

template<>
inline jbyte callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallByteMethodV(obj, method, args);
}

template<>
inline jchar callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallCharMethodV(obj, method, args);
}

template<>
inline jshort callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallShortMethodV(obj, method, args);
}

template<>
inline jint callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallIntMethodV(obj, method, args);
}

template<>
inline jlong callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallLongMethodV(obj, method, args);
}

template<>
inline jfloat callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallFloatMethodV(obj, method, args);
}

template<>
inline jdouble callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args)
{
	return env->CallDoubleMethodV(obj, method, args);
}

template<>
inline void callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	env->CallStaticVoidMethodV(cls, method, args);
}

template<>
inline jobject callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticObjectMethodV(cls, method, args);
}

template<>
inline jboolean callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticBooleanMethodV(cls, method, args);
}

template<>
inline jbyte callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticByteMethodV(cls, method, args);
}

template<>
inline jchar callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticCharMethodV(cls, method, args);
}

template<>
inline jshort callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticShortMethodV(cls, method, args);
}

template<>
inline jint callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticIntMethodV(cls, method, args);
}

template<>
inline jlong callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticLongMethodV(cls, method, args);
}

template<>
inline jfloat callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticFloatMethodV(cls, method, args);
}

template<>
inline jdouble callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args)
{
	return env->CallStaticDoubleMethodV(cls, method, args);
}
