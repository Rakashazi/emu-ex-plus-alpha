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

LockedLocalBitmap::LockedLocalBitmap(LockedLocalBitmap &&o)
{
	*this = std::move(o);
}

LockedLocalBitmap &LockedLocalBitmap::operator=(LockedLocalBitmap &&o)
{
	deinit();
	env = o.env;
	bitmap = std::exchange(o.bitmap, {});
	jRecycle = o.jRecycle;
	return *this;
}

LockedLocalBitmap::~LockedLocalBitmap()
{
	deinit();
}

void LockedLocalBitmap::deinit()
{
	if(!bitmap)
		return;
	AndroidBitmap_unlockPixels(env, bitmap);
	jRecycle(env, bitmap);
	env->DeleteLocalRef(std::exchange(bitmap, {}));
}

}
