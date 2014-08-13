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

#define LOGTAG "BitmapFactory"

#include <imagine/data-type/image/Android.hh>
#include <assert.h>
#include <imagine/logger/logger.h>
#include <imagine/util/pixel.h>
#include <imagine/util/strings.h>
#include "../../base/android/android.hh"

static jclass jBitmapFactory = nullptr;
static JavaClassMethod<jobject> jDecodeFile;
static JavaInstMethod<jobject> jDecodeAsset;
static JavaInstMethod<void> jRecycle;

uint BitmapFactoryImage::width()
{
	return info.width;
}

uint BitmapFactoryImage::height()
{
	return info.height;
}

bool BitmapFactoryImage::isGrayscale()
{
	return info.format == ANDROID_BITMAP_FORMAT_A_8;
}

const PixelFormatDesc *BitmapFactoryImage::pixelFormat()
{
	switch(info.format)
	{
		case ANDROID_BITMAP_FORMAT_RGBA_8888: return &PixelFormatRGBA8888;
		case ANDROID_BITMAP_FORMAT_RGB_565: return &PixelFormatRGB565;
		case ANDROID_BITMAP_FORMAT_RGBA_4444: return &PixelFormatRGBA4444;
		case ANDROID_BITMAP_FORMAT_A_8: return &PixelFormatI8;
		default:
		{
			if(info.format == ANDROID_BITMAP_FORMAT_NONE)
				logWarn("format wasn't provided");
			else
				bug_exit("unhandled format");
			return &PixelFormatRGBA8888;
		}
	}
}

CallResult BitmapFactoryImage::load(const char *name)
{
	freeImageData();
	auto env = Base::jEnv();
	if(!jBitmapFactory)
	{
		jBitmapFactory = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/BitmapFactory"));
		jDecodeFile.setup(env, jBitmapFactory, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
	}
	auto nameJStr = env->NewStringUTF(name);
	bitmap = jDecodeFile(env, jBitmapFactory, nameJStr);
	env->DeleteLocalRef(nameJStr);
	if(!bitmap)
	{
		logErr("couldn't decode file: %s", name);
		return INVALID_PARAMETER;
	}
	if(!jRecycle)
	{
		jRecycle.setup(env, env->GetObjectClass(bitmap), "recycle", "()V");
	}
	AndroidBitmap_getInfo(env, bitmap, &info);
	bitmap = env->NewGlobalRef(bitmap);
	return OK;
}

CallResult BitmapFactoryImage::loadAsset(const char *name)
{
	freeImageData();
	logMsg("loading PNG asset: %s", name);
	auto env = Base::jEnv();
	using namespace Base;
	if(!jDecodeAsset)
	{
		jDecodeAsset.setup(env, jBaseActivityCls, "bitmapDecodeAsset", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
	}
	auto nameJStr = env->NewStringUTF(name);
	bitmap = jDecodeAsset(env, jBaseActivity, nameJStr);
	env->DeleteLocalRef(nameJStr);
	if(!bitmap)
	{
		logErr("couldn't decode file: %s", name);
		return INVALID_PARAMETER;
	}
	if(!jRecycle)
	{
		jRecycle.setup(env, env->GetObjectClass(bitmap), "recycle", "()V");
	}
	AndroidBitmap_getInfo(env, bitmap, &info);
	//logMsg("%d %d %d", info.width, info.height, info.stride);
	bitmap = env->NewGlobalRef(bitmap);
	return OK;
}

bool BitmapFactoryImage::hasAlphaChannel()
{
	return info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 || info.format == ANDROID_BITMAP_FORMAT_RGBA_4444;
}

CallResult BitmapFactoryImage::readImage(IG::Pixmap &dest)
{
	assert(dest.format.id == pixelFormat()->id);
	auto env = Base::jEnv();
	void *buff;
	AndroidBitmap_lockPixels(env, bitmap, &buff);
	IG::Pixmap src(*pixelFormat());
	src.init2((char*)buff, info.width, info.height, info.stride);
	src.copy(0, 0, 0, 0, dest, 0, 0);
	AndroidBitmap_unlockPixels(env, bitmap);
	return OK;
}

void BitmapFactoryImage::freeImageData()
{
	if(bitmap)
	{
		auto env = Base::jEnv();
		jRecycle(env, bitmap);
		env->DeleteGlobalRef(bitmap);
		bitmap = nullptr;
	}
}

CallResult PngFile::getImage(IG::Pixmap &dest)
{
	return(png.readImage(dest));
}

CallResult PngFile::load(const char *name)
{
	deinit();
	return png.load(name);
}

CallResult PngFile::loadAsset(const char *name)
{
	deinit();
	return png.loadAsset(name);
}

void PngFile::deinit()
{
	png.freeImageData();
}
