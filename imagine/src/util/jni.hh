#pragma once

#include <jni.h>
#include <util/basicString.h>

template <class T>
class JavaClassMethod
{
public:
	constexpr JavaClassMethod() { }
	jclass c = nullptr;
	jmethodID m = nullptr;

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s in %s", fName, cName);
		c = cls;
		assert(c);
		m = j->GetStaticMethodID(c, fName, sig);
		assert(m);
	}

	T operator()(JNIEnv* j, ...);
};

template<> inline void JavaClassMethod<void>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	j->CallStaticVoidMethodV(c, m, args);
	va_end(args);
}

template<> inline jobject JavaClassMethod<jobject>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticObjectMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jboolean JavaClassMethod<jboolean>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticBooleanMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jbyte JavaClassMethod<jbyte>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticByteMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jchar JavaClassMethod<jchar>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticCharMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jshort JavaClassMethod<jshort>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticShortMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jint JavaClassMethod<jint>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticIntMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jlong JavaClassMethod<jlong>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticLongMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jfloat JavaClassMethod<jfloat>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticFloatMethodV(c, m, args);
	va_end(args);
	return ret;
}

template<> inline jdouble JavaClassMethod<jdouble>::operator()(JNIEnv* j, ...)
{
	va_list args;
	va_start(args, j);
	auto ret = j->CallStaticDoubleMethodV(c, m, args);
	va_end(args);
	return ret;
}

template <class T>
class JavaInstMethod
{
public:
	constexpr JavaInstMethod() { }
	jmethodID m = nullptr;

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s with sig %s", fName, sig);
		assert(cls != 0);
		m = j->GetMethodID(cls, fName, sig);
		assert(m);
	}

	T operator()(JNIEnv* j, jobject obj, ...) const;
};

template<> inline void JavaInstMethod<void>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	j->CallVoidMethodV(obj, m, args);
	va_end(args);
}

template<> inline jobject JavaInstMethod<jobject>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallObjectMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jboolean JavaInstMethod<jboolean>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallBooleanMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jbyte JavaInstMethod<jbyte>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallByteMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jchar JavaInstMethod<jchar>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallCharMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jshort JavaInstMethod<jshort>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallShortMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jint JavaInstMethod<jint>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallIntMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jlong JavaInstMethod<jlong>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallLongMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jfloat JavaInstMethod<jfloat>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallFloatMethodV(obj, m, args);
	va_end(args);
	return ret;
}

template<> inline jdouble JavaInstMethod<jdouble>::operator()(JNIEnv* j, jobject obj, ...) const
{
	va_list args;
	va_start(args, obj);
	auto ret = j->CallDoubleMethodV(obj, m, args);
	va_end(args);
	return ret;
}

static void javaStringDup(JNIEnv* env, const char *&dest, jstring jstr)
{
	const char *str = env->GetStringUTFChars(jstr, 0);
	if(!str)
	{
		dest = nullptr;
		return; // OutOfMemoryError thrown
	}
	dest = string_dup(str);
	env->ReleaseStringUTFChars(jstr, str);
}
