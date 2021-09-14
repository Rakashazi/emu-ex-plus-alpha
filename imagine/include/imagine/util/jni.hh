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
#include <imagine/util/concepts.hh>
#include <cassert>
#include <cstddef>
#include <iterator>

namespace JNI
{

jmethodID getJNIStaticMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);
jmethodID getJNIMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);

template<class R> requires IG::same_as<R, void>
static void callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	env->CallVoidMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jobject>
static jobject callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallObjectMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jboolean>
static jboolean callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallBooleanMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jbyte>
static jbyte callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallByteMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jchar>
static jchar callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallCharMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jshort>
static jshort callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallShortMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jint>
static jint callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallIntMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jlong>
static jlong callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallLongMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jfloat>
static jfloat callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallFloatMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jdouble>
static jdouble callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallDoubleMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, void>
static void callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	env->CallStaticVoidMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jobject>
static jobject callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticObjectMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jboolean>
static jboolean callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticBooleanMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jbyte>
static jbyte callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticByteMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jchar>
static jchar callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticCharMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jshort>
static jshort callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticShortMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jint>
static jint callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticIntMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jlong>
static jlong callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticLongMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jfloat>
static jfloat callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticFloatMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<class R> requires IG::same_as<R, jdouble>
static jdouble callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticDoubleMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template <class T> class ClassMethod {};

template <class R, class ...Args>
class ClassMethod<R(Args...)>
{
public:
	jmethodID method{};

	constexpr ClassMethod() {}

	ClassMethod(JNIEnv *env, auto obj, const char *fName, const char *sig)
	{
		setMethod(env, obj, fName, sig);
	}

	bool setMethod(JNIEnv *env, IG::same_as<jclass> auto obj, const char *fName, const char *sig)
	{
		method = getJNIStaticMethodID(env, obj, fName, sig);
		return method;
	}

	bool setMethod(JNIEnv *env, IG::same_as<jobject> auto obj, const char *fName, const char *sig)
	{
		return setMethod(env, (jclass)env->GetObjectClass(obj), fName, sig);
	}

	explicit operator bool() const
	{
		return method;
	}

	R operator()(JNIEnv *env, jclass cls, Args ... args) const
	{
		return callJNIStaticMethod<R>(env, method, cls, std::forward<Args>(args)...);
	}
};

template <class T> class InstMethod {};

template <class R, class ...Args>
class InstMethod<R(Args...)>
{
public:
	jmethodID method{};

	constexpr InstMethod() {}

	template <class JavaObject>
	InstMethod(JNIEnv *env, JavaObject obj, const char *fName, const char *sig)
	{
		setMethod(env, obj, fName, sig);
	}

	bool setMethod(JNIEnv *env, IG::same_as<jclass> auto obj, const char *fName, const char *sig)
	{
		method = getJNIMethodID(env, obj, fName, sig);
		return method;
	}

	bool setMethod(JNIEnv *env, IG::same_as<jobject> auto obj, const char *fName, const char *sig)
	{
		return setMethod(env, (jclass)env->GetObjectClass(obj), fName, sig);
	}

	explicit operator bool() const
	{
		return method;
	}

	R operator()(JNIEnv *env, jobject obj, Args ... args) const
	{
		return callJNIMethod<R>(env, method, obj, std::forward<Args>(args)...);
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

template <class Container>
static Container stringCopy(JNIEnv *env, jstring jstr)
{
	auto utfChars = env->GetStringUTFChars(jstr, nullptr);
	if(!utfChars)
	{
		return {}; // OutOfMemoryError thrown
	}
	Container c;
	string_copy(c, utfChars);
	env->ReleaseStringUTFChars(jstr, utfChars);
	return c;
}

class LockedLocalBitmap
{
public:
	constexpr LockedLocalBitmap() {}
	constexpr LockedLocalBitmap(JNIEnv *env, jobject bitmap, JNI::InstMethod<void()> recycle):
		env{env}, bitmap{bitmap}, jRecycle{recycle} {}
	LockedLocalBitmap(LockedLocalBitmap &&o);
	LockedLocalBitmap &operator=(LockedLocalBitmap &&o);
	~LockedLocalBitmap();
	constexpr operator jobject() const { return bitmap; }
	explicit constexpr operator bool() const { return bitmap; }

protected:
	JNIEnv *env{};
	jobject bitmap{};
	JNI::InstMethod<void()> jRecycle{};

	void deinit();
};

}
