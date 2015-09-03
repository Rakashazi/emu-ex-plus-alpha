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

#define LOGTAG "ResFontAndroid"
#include <imagine/resource/font/ResourceFontAndroid.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/strings.h>
#include <imagine/util/jni.hh>
#include "../../base/android/android.hh"
#include <android/bitmap.h>

using namespace Base;

static JavaInstMethod<jobject()> jCharBitmap;
static JavaInstMethod<jobject(jint)> jNewSize;
static JavaInstMethod<void(jobject)> jApplySize, jFreeSize, jUnlockCharBitmap;
static JavaInstMethod<jboolean(jint)> jActiveChar;
static JavaInstMethod<jint()> jCurrentCharXSize, jCurrentCharYSize, jCurrentCharXOffset, jCurrentCharYOffset,
	jCurrentFaceDescender, jCurrentFaceAscender, jCurrentCharXAdvance;

static void setupResourceFontAndroidJni(JNIEnv *env, jobject renderer)//jClsLoader, const JavaInstMethod<jobject> &jLoadClass)
{
	if(jCharBitmap)
		return; // already setup
	//logMsg("setting up JNI methods");
	/*jstring classStr = env->NewStringUTF("com/imagine/FontRenderer");
	jFontRendererCls = (jclass)env->NewGlobalRef(jLoadClass(env, jClsLoader, classStr));
	env->DeleteLocalRef(classStr);
	jFontRenderer.setup(env, jFontRendererCls, "<init>", "()V");*/
	auto jFontRendererCls = env->GetObjectClass(renderer);
	jCharBitmap.setup(env, jFontRendererCls, "charBitmap", "()Landroid/graphics/Bitmap;");
	jUnlockCharBitmap.setup(env, jFontRendererCls, "unlockCharBitmap", "(Landroid/graphics/Bitmap;)V");
	jActiveChar.setup(env, jFontRendererCls, "activeChar", "(I)Z");
	jCurrentCharXSize.setup(env, jFontRendererCls, "currentCharXSize", "()I");
	jCurrentCharYSize.setup(env, jFontRendererCls, "currentCharYSize", "()I");
	jCurrentCharXOffset.setup(env, jFontRendererCls, "currentCharXOffset", "()I");
	jCurrentCharYOffset.setup(env, jFontRendererCls, "currentCharYOffset", "()I");
	jCurrentCharXAdvance.setup(env, jFontRendererCls, "currentCharXAdvance", "()I");
	//jCurrentFaceDescender.setup(env, jFontRendererCls, "currentFaceDescender", "()I");
	//jCurrentFaceAscender.setup(env, jFontRendererCls, "currentFaceAscender", "()I");
	jNewSize.setup(env, jFontRendererCls, "newSize", "(I)Landroid/graphics/Paint;");
	jApplySize.setup(env, jFontRendererCls, "applySize", "(Landroid/graphics/Paint;)V");
	jFreeSize.setup(env, jFontRendererCls, "freeSize", "(Landroid/graphics/Paint;)V");
}

ResourceFont *ResourceFontAndroid::loadSystem()
{
	ResourceFontAndroid *inst = new ResourceFontAndroid;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}
	auto env = jEnv();
	inst->renderer = Base::newFontRenderer(env);
	setupResourceFontAndroidJni(env, inst->renderer);
	if(env->ExceptionCheck())
	{
		logErr("exception");
		env->ExceptionClear();
		inst->free();
		return nullptr;
	}
	inst->renderer = env->NewGlobalRef(inst->renderer);

	return inst;
}

void ResourceFontAndroid::free()
{
	if(renderer)
		Base::jEnv()->DeleteGlobalRef(renderer);
	delete this;
}

static const char *androidBitmapResultToStr(int result)
{
	switch(result)
	{
		case ANDROID_BITMAP_RESULT_SUCCESS: return "Success";
		case ANDROID_BITMAP_RESULT_BAD_PARAMETER: return "Bad Parameter";
		case ANDROID_BITMAP_RESULT_JNI_EXCEPTION: return "JNI Exception";
		case ANDROID_BITMAP_RESULT_ALLOCATION_FAILED: return "Allocation Failed";
		default: return "Unknown";
	}
}

IG::Pixmap ResourceFontAndroid::charBitmap()
{
	assert(!lockedBitmap);
	auto env = jEnv();
	lockedBitmap = jCharBitmap(env, renderer);
	assert(lockedBitmap);
	//logMsg("got bitmap @ %p", lockedBitmap);
	//lockedBitmap = env->NewGlobalRef(lockedBitmap);
	AndroidBitmapInfo info;
	{
		auto res = AndroidBitmap_getInfo(env, lockedBitmap, &info);
		//logMsg("AndroidBitmap_getInfo returned %s", androidBitmapResultToStr(res));
		assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
		//logMsg("size %dx%d, pitch %d", info.width, info.height, info.stride);
	}
	void *data{};
	{
		auto res = AndroidBitmap_lockPixels(env, lockedBitmap, &data);
		//logMsg("AndroidBitmap_lockPixels returned %s", androidBitmapResultToStr(res));
		assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
	}
	return {{{(int)info.width, (int)info.height}, PIXEL_A8}, data, {info.stride, IG::Pixmap::BYTE_UNITS}};
}

void ResourceFontAndroid::unlockCharBitmap()
{
	auto env = jEnv();
	AndroidBitmap_unlockPixels(env, lockedBitmap);
	jUnlockCharBitmap(env, renderer, lockedBitmap);
	env->DeleteLocalRef(lockedBitmap);
	lockedBitmap = nullptr;
}

CallResult ResourceFontAndroid::activeChar(int idx, GlyphMetrics &metrics)
{
	//logMsg("active char: %c", idx);
	auto env = jEnv();
	if(jActiveChar(env, renderer, idx))
	{
		metrics.xSize = jCurrentCharXSize(env, renderer);
		metrics.ySize = jCurrentCharYSize(env, renderer);
		metrics.xOffset = jCurrentCharXOffset(env, renderer);
		metrics.yOffset = jCurrentCharYOffset(env, renderer);
		metrics.xAdvance = jCurrentCharXAdvance(env, renderer);
		//logMsg("char metrics: size %dx%d offset %dx%d advance %d", metrics.xSize, metrics.ySize,
		//		metrics.xOffset, metrics.yOffset, metrics.xAdvance);
		return OK;
	}
	else
	{
		logMsg("char not available");
		return INVALID_PARAMETER;
	}
}

/*int ResourceFontAndroid::currentFaceDescender () const
{ return jCurrentFaceDescender(jEnv(), renderer); }
int ResourceFontAndroid::currentFaceAscender () const
{ return jCurrentFaceAscender(jEnv(), renderer); }*/

CallResult ResourceFontAndroid::newSize(const FontSettings &settings, FontSizeRef &sizeRef)
{
	freeSize(sizeRef);
	auto env = jEnv();
	auto size = jNewSize(env, renderer, settings.pixelHeight);
	assert(size);
	logMsg("allocated new size %dpx @ 0x%p", settings.pixelHeight, size);
	sizeRef.ptr = env->NewGlobalRef(size);
	return OK;
}

CallResult ResourceFontAndroid::applySize(FontSizeRef &sizeRef)
{
	jApplySize(jEnv(), renderer, (jobject)sizeRef.ptr);
	return OK;
}

void ResourceFontAndroid::freeSize(FontSizeRef &sizeRef)
{
	if(!sizeRef.ptr)
		return;
	auto env = jEnv();
	jFreeSize(env, renderer, (jobject)sizeRef.ptr);
	env->DeleteGlobalRef((jobject)sizeRef.ptr);
	sizeRef.ptr = nullptr;
}
