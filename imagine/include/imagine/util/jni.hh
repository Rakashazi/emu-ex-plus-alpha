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

#include <imagine/util/concepts.hh>
#include <cassert>
#include <cstddef>
#include <iterator>
#if __has_include(<jni.h>)
#include <jni.h>
#else
// dummy implementation to allow header inclusion but not use
#define DUMMY_JNI_IMPL
using jobject  = void*;
using jclass  = int*;
using jstring = char*;
using jfieldID = void*;
using jmethodID = void*;
using jboolean = bool;
using jbyte = int8_t;
using jchar = uint16_t;
using jshort = int16_t;
using jint = int32_t;
using jlong = int64_t;
using jfloat = float;
using jdouble = double;
struct JNIEnv
{
	constexpr jclass GetObjectClass(jobject) const { return {}; }
};
#endif

namespace JNI
{

jmethodID getJNIStaticMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);
jmethodID getJNIMethodID(JNIEnv *env, jclass cls, const char *fName, const char *sig);

#ifndef DUMMY_JNI_IMPL
template<IG::same_as<void> R>
static void callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	env->CallVoidMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jobject> R>
static jobject callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallObjectMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jstring> R>
static jstring callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return (jstring)env->CallObjectMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jboolean> R>
static jboolean callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallBooleanMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jbyte> R>
static jbyte callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallByteMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jchar> R>
static jchar callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallCharMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jshort> R>
static jshort callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallShortMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jint> R>
static jint callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallIntMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jlong> R>
static jlong callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallLongMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jfloat> R>
static jfloat callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallFloatMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jdouble> R>
static jdouble callJNIMethod(JNIEnv *env, jmethodID method, jobject obj, auto &&... args)
{
	return env->CallDoubleMethod(obj, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<void> R>
static void callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	env->CallStaticVoidMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jobject> R>
static jobject callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticObjectMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jstring> R>
static jstring callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return (jstring)env->CallStaticObjectMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jboolean> R>
static jboolean callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticBooleanMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jbyte> R>
static jbyte callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticByteMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jchar> R>
static jchar callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticCharMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jshort> R>
static jshort callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticShortMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jint> R>
static jint callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticIntMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jlong> R>
static jlong callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticLongMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jfloat> R>
static jfloat callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticFloatMethod(cls, method, std::forward<decltype(args)>(args)...);
}

template<IG::same_as<jdouble> R>
static jdouble callJNIStaticMethod(JNIEnv *env, jmethodID method, jclass cls, auto &&... args)
{
	return env->CallStaticDoubleMethod(cls, method, std::forward<decltype(args)>(args)...);
}
#endif

template <class T> class ClassMethod {};

template <class R, class ...Args>
class ClassMethod<R(Args...)>
{
public:
	jmethodID method{};

	constexpr ClassMethod() = default;

	[[gnu::nonnull]]
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

	constexpr InstMethod() = default;

	template <class JavaObject> [[gnu::nonnull]]
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
	constexpr UniqueGlobalRef() = default;
	[[gnu::nonnull]]
	UniqueGlobalRef(JNIEnv *env, jobject o);
	operator jobject() const { return obj.get(); }
	explicit operator jclass() const { return (jclass)obj.get(); }
	JNIEnv *jniEnv() const { return obj.get_deleter().env; }
	void reset() { obj.reset(); }

protected:
	struct GlobalRefDeleter
	{
		JNIEnv *env;

		void operator()(jobject ptr) const
		{
			deleteGlobalRef(env, ptr);
		}
	};

	std::unique_ptr<std::remove_pointer_t<jobject>, GlobalRefDeleter> obj{};

	static void deleteGlobalRef(JNIEnv *, jobject);
};

class StringChars
{
public:
	constexpr StringChars() = default;
	[[gnu::nonnull]]
	StringChars(JNIEnv *, jstring);
	const char *cString() const { return str.get(); }
	operator const char*() const { return cString(); }
	JNIEnv *jniEnv() const { return str.get_deleter().env; }
	jstring jString() const { return str.get_deleter().jStr; }

protected:
	struct JStringCharsDeleter
	{
		JNIEnv *env;
		jstring jStr;

		void operator()(const char *charsPtr) const
		{
			releaseStringChars(env, jStr, charsPtr);
		}
	};
	using UniqueJStringChars = std::unique_ptr<const char, JStringCharsDeleter>;

	UniqueJStringChars str{};

	static void releaseStringChars(JNIEnv *, jstring, const char *charsPtr);
};

class LockedLocalBitmap
{
public:
	constexpr LockedLocalBitmap() = default;
	[[gnu::nonnull]]
	LockedLocalBitmap(JNIEnv *env, jobject bitmap, JNI::InstMethod<void()> recycle):
		bitmap{bitmap, {env, recycle}} {}
	operator jobject() const { return bitmap.get(); }
	explicit operator bool() const { return (bool)bitmap; }

protected:
	struct BitmapDeleter
	{
		JNIEnv *env;
		JNI::InstMethod<void()> recycle;

		void operator()(jobject ptr) const
		{
			deleteBitmap(env, ptr, recycle);
		}
	};

	std::unique_ptr<std::remove_pointer_t<jobject>, BitmapDeleter> bitmap{};

	static void deleteBitmap(JNIEnv *, jobject bitmap, JNI::InstMethod<void()> recycle);
};

}

#undef DUMMY_JNI_IMPL
