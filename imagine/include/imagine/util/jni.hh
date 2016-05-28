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
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include <assert.h>

jmethodID getJNIStaticMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);
jmethodID getJNIMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);

template<typename R>
R callJNIStaticMethodV(JNIEnv *env, jmethodID method, jclass cls, va_list args);

template<typename R>
R callJNIMethodV(JNIEnv *env, jmethodID method, jobject obj, va_list args);

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
		method = getJNIStaticMethodID(env, cls, fName, sig);
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
		method = getJNIMethodID(env, cls, fName, sig);
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
