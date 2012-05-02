#pragma once

#include <jni.h>
#include <util/basicString.h>

class JavaClassMethod
{
public:
	jclass c;
	jmethodID m;

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s in %s", fName, cName);
		c = cls;
		assert(c != 0);
		m = j->GetStaticMethodID(c, fName, sig);
		assert(m != 0);
	}
};

class JavaInstMethod
{
public:
	jmethodID m;

	void setup(JNIEnv* j, jclass cls, const char *fName, const char *sig)
	{
		//logMsg("find method %s in %s", fName, cName);
		assert(cls != 0);
		m = j->GetMethodID(cls, fName, sig);
		assert(m != 0);
	}
};

static void javaStringDup(JNIEnv* env, const char *&dest, jstring jstr)
{
	const char *str = env->GetStringUTFChars(jstr, 0);
	if (str == NULL)
	{
		return; // OutOfMemoryError thrown
	}
	dest = string_dup(str);
	env->ReleaseStringUTFChars(jstr, str);
}
