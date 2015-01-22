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
#include <imagine/util/basicString.h>

template <class T>
class JavaClassMethod
{
public:
	jclass c = nullptr;
	jmethodID m = nullptr;

	constexpr JavaClassMethod() {}

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s in %s", fName, cName);
		c = cls;
		assert(c);
		m = j->GetStaticMethodID(c, fName, sig);
		assert(m);
	}

	explicit operator bool() const
	{
		return m;
	}

	template<class... ARGS>
	T operator()(JNIEnv* j, ARGS&&... args);
};

template<> template<class... ARGS>
inline void JavaClassMethod<void>::operator()(JNIEnv* j, ARGS&&... args)
{
	j->CallStaticVoidMethod(c, m, std::forward<ARGS>(args)...);
}

template<> template<class... ARGS>
inline jobject JavaClassMethod<jobject>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticObjectMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jboolean JavaClassMethod<jboolean>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticBooleanMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jbyte JavaClassMethod<jbyte>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticByteMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jchar JavaClassMethod<jchar>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticCharMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jshort JavaClassMethod<jshort>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticShortMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jint JavaClassMethod<jint>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticIntMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jlong JavaClassMethod<jlong>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticLongMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jfloat JavaClassMethod<jfloat>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticFloatMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jdouble JavaClassMethod<jdouble>::operator()(JNIEnv* j, ARGS&&... args)
{
	auto ret = j->CallStaticDoubleMethod(c, m, std::forward<ARGS>(args)...);
	return ret;
}

template <class T>
class JavaInstMethod
{
public:
	jmethodID m = nullptr;

	constexpr JavaInstMethod() {}

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s with sig %s", fName, sig);
		if(!cls)
		{
			bug_exit("class missing for java method: %s (%s)", fName, sig);
		}
		m = j->GetMethodID(cls, fName, sig);
		if(!m)
		{
			bug_exit("java method not found: %s (%s)", fName, sig);
		}
	}

	explicit operator bool() const
	{
		return m;
	}

	template<class... ARGS>
	T operator()(JNIEnv* j, jobject obj, ARGS&&... args) const;
};

template<> template<class... ARGS>
inline void JavaInstMethod<void>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	j->CallVoidMethod(obj, m, std::forward<ARGS>(args)...);
}

template<> template<class... ARGS>
inline jobject JavaInstMethod<jobject>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallObjectMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jboolean JavaInstMethod<jboolean>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallBooleanMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jbyte JavaInstMethod<jbyte>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallByteMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jchar JavaInstMethod<jchar>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallCharMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jshort JavaInstMethod<jshort>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallShortMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jint JavaInstMethod<jint>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallIntMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jlong JavaInstMethod<jlong>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallLongMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jfloat JavaInstMethod<jfloat>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallFloatMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

template<> template<class... ARGS>
inline jdouble JavaInstMethod<jdouble>::operator()(JNIEnv* j, jobject obj, ARGS&&... args) const
{
	auto ret = j->CallDoubleMethod(obj, m, std::forward<ARGS>(args)...);
	return ret;
}

static void javaStringDup(JNIEnv* env, const char *&dest, jstring jstr)
{
	const char *str = env->GetStringUTFChars(jstr, nullptr);
	if(!str)
	{
		dest = nullptr;
		return; // OutOfMemoryError thrown
	}
	dest = string_dup(str);
	env->ReleaseStringUTFChars(jstr, str);
}

class JObject
{
protected:
	jobject o = nullptr;

	constexpr JObject() {};
	constexpr JObject(jobject o): o(o) {};

public:
	operator jobject() const
	{
		return o;
	}
};
