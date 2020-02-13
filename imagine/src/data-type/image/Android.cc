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
#include <imagine/util/jni.hh>
#include "../../base/android/android.hh"

using namespace IG;

static jclass jBitmapFactory{};
static JavaClassMethod<jobject(jstring)> jDecodeFile{};
static JavaInstMethod<jobject(jstring)> jDecodeAsset{};

uint32_t BitmapFactoryImage::width()
{
	return info.width;
}

uint32_t BitmapFactoryImage::height()
{
	return info.height;
}

bool BitmapFactoryImage::isGrayscale()
{
	return info.format == ANDROID_BITMAP_FORMAT_A_8;
}

PixelFormat BitmapFactoryImage::pixelFormat() const
{
	switch(info.format)
	{
		case ANDROID_BITMAP_FORMAT_RGBA_8888: return PIXEL_FMT_RGBA8888;
		case ANDROID_BITMAP_FORMAT_RGB_565: return PIXEL_FMT_RGB565;
		case ANDROID_BITMAP_FORMAT_RGBA_4444: return PIXEL_FMT_RGBA4444;
		case ANDROID_BITMAP_FORMAT_A_8: return PIXEL_FMT_I8;
		default:
		{
			if(info.format == ANDROID_BITMAP_FORMAT_NONE)
				logWarn("format wasn't provided");
			else
				logErr("unhandled format");
			return PIXEL_FMT_RGBA8888;
		}
	}
}

std::error_code BitmapFactoryImage::load(const char *name)
{
	freeImageData();
	auto env = Base::jEnvForThread();
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
		return {EINVAL, std::system_category()};
	}
	AndroidBitmap_getInfo(env, bitmap, &info);
	bitmap = env->NewGlobalRef(bitmap);
	return {};
}

std::error_code BitmapFactoryImage::loadAsset(const char *name)
{
	freeImageData();
	logMsg("loading PNG asset: %s", name);
	auto env = Base::jEnvForThread();
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
		return {EINVAL, std::system_category()};
	}
	AndroidBitmap_getInfo(env, bitmap, &info);
	//logMsg("%d %d %d", info.width, info.height, info.stride);
	bitmap = env->NewGlobalRef(bitmap);
	return {};
}

bool BitmapFactoryImage::hasAlphaChannel()
{
	return info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 || info.format == ANDROID_BITMAP_FORMAT_RGBA_4444;
}

std::errc BitmapFactoryImage::readImage(IG::Pixmap &dest)
{
	assert(dest.format() == pixelFormat());
	auto env = Base::jEnvForThread();
	void *buff;
	AndroidBitmap_lockPixels(env, bitmap, &buff);
	IG::Pixmap src{{{(int)info.width, (int)info.height}, pixelFormat()}, buff, {info.stride, IG::Pixmap::BYTE_UNITS}};
	dest.write(src, {});
	AndroidBitmap_unlockPixels(env, bitmap);
	return {};
}

void BitmapFactoryImage::freeImageData()
{
	if(bitmap)
	{
		auto env = Base::jEnvForThread();
		Base::recycleBitmap(env, bitmap);
		env->DeleteGlobalRef(bitmap);
		bitmap = nullptr;
	}
}

BitmapFactoryImage::operator bool() const
{
	return (bool)bitmap;
}

PngFile::PngFile() {}

PngFile::~PngFile()
{
	deinit();
}

std::errc PngFile::write(IG::Pixmap dest)
{
	return(png.readImage(dest));
}

IG::Pixmap PngFile::lockPixmap()
{
	IG::Pixmap pix{{{(int)png.width(), (int)png.height()}, png.pixelFormat()}, nullptr};
	return pix;
}

void PngFile::unlockPixmap() {}

std::error_code PngFile::load(const char *name)
{
	deinit();
	return png.load(name);
}

std::error_code PngFile::loadAsset(const char *name, const char *appName)
{
	deinit();
	return png.loadAsset(name);
}

void PngFile::deinit()
{
	png.freeImageData();
}

PngFile::operator bool() const
{
	return (bool)png;
}
