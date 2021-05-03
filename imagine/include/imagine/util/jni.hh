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

#include <jni.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include <assert.h>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace JNI
{

jmethodID getJNIStaticMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);
jmethodID getJNIMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);

template<typename R>
R callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args);

template<typename R>
R callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args);

template <typename T> class ClassMethod {};

template <typename R, typename ...ARGS>
class ClassMethod<R(ARGS...)>
{
public:
	jmethodID method{};

	constexpr ClassMethod() {}

	template <class JavaObject>
	ClassMethod(JNIEnv *env, JavaObject obj, const char *fName, const char *sig)
	{
		setMethod(env, obj, fName, sig);
	}

	template <class JavaObject>
	bool setMethod(JNIEnv *env, JavaObject obj, const char *fName, const char *sig)
	{
		if constexpr(std::is_same_v<JavaObject, jclass>)
		{
			method = getJNIStaticMethodID(env, obj, fName, sig);
		}
		else
		{
			static_assert(std::is_same_v<JavaObject, jobject>, "object type must be a jclass or jobject");
			method = getJNIStaticMethodID(env, (jclass)env->GetObjectClass(obj), fName, sig);
		}
		if(!method)
		{
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

template <typename T> class InstMethod {};

template <typename R, typename ...ARGS>
class InstMethod<R(ARGS...)>
{
public:
	jmethodID method{};

	constexpr InstMethod() {}

	template <class JavaObject>
	InstMethod(JNIEnv *env, JavaObject obj, const char *fName, const char *sig)
	{
		setMethod(env, obj, fName, sig);
	}

	template <class JavaObject>
	bool setMethod(JNIEnv *env, JavaObject obj, const char *fName, const char *sig)
	{
		if constexpr(std::is_same_v<JavaObject, jclass>)
		{
			method = getJNIMethodID(env, obj, fName, sig);
		}
		else
		{
			static_assert(std::is_same_v<JavaObject, jobject>, "object type must be a jclass or jobject");
			method = getJNIMethodID(env, (jclass)env->GetObjectClass(obj), fName, sig);
		}
		if(!method)
		{
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

class UniqueGlobalRef
{
public:
	constexpr UniqueGlobalRef() {};
	UniqueGlobalRef(JNIEnv *env, jobject o);
	UniqueGlobalRef(UniqueGlobalRef &&);
	UniqueGlobalRef &operator=(UniqueGlobalRef &&);
	~UniqueGlobalRef();
	void reset();

	constexpr operator jobject() const
	{
		return obj;
	}

	constexpr explicit operator jclass() const
	{
		return (jclass)obj;
	}

	constexpr JNIEnv *jniEnv() const
	{
		return env;
	}

protected:
	JNIEnv *env;
	jobject obj{};
};

template <size_t S>
static void stringCopy(JNIEnv *env, std::array<char, S> &dest, jstring jstr)
{
	auto utfChars = env->GetStringUTFChars(jstr, nullptr);
	if(!utfChars)
	{
		return; // OutOfMemoryError thrown
	}
	string_copy(dest, utfChars);
	env->ReleaseStringUTFChars(jstr, utfChars);
}

template <class CONTAINER>
static CONTAINER stringCopy(JNIEnv *env, jstring jstr)
{
	auto utfChars = env->GetStringUTFChars(jstr, nullptr);
	if(!utfChars)
	{
		return {}; // OutOfMemoryError thrown
	}
	CONTAINER c;
	string_copy(c, utfChars);
	env->ReleaseStringUTFChars(jstr, utfChars);
	return c;
}

}
