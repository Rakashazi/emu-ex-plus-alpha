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
#include "../../base/android/android.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>

static jclass jBitmapFactory{};
static JNI::ClassMethod<jobject(jstring)> jDecodeFile{};
static JNI::InstMethod<jobject(jstring)> jDecodeAsset{};

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

IG::PixelFormat BitmapFactoryImage::pixelFormat() const
{
	return Base::makePixelFormatFromAndroidFormat(info.format);
}

std::error_code BitmapFactoryImage::load(const char *name)
{
	freeImageData();
	auto env = ctx.thisThreadJniEnv();
	if(!jBitmapFactory)
	{
		jBitmapFactory = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/BitmapFactory"));
		jDecodeFile = {env, jBitmapFactory, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;"};
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
	return {};
}

std::error_code BitmapFactoryImage::loadAsset(const char *name)
{
	freeImageData();
	logMsg("loading PNG asset: %s", name);
	auto env = ctx.thisThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	if(!jDecodeAsset) [[unlikely]]
	{
		jDecodeAsset = {env, baseActivity, "bitmapDecodeAsset", "(Ljava/lang/String;)Landroid/graphics/Bitmap;"};
	}
	auto nameJStr = env->NewStringUTF(name);
	bitmap = jDecodeAsset(env, baseActivity, nameJStr);
	env->DeleteLocalRef(nameJStr);
	if(!bitmap)
	{
		logErr("couldn't decode file: %s", name);
		return {EINVAL, std::system_category()};
	}
	AndroidBitmap_getInfo(env, bitmap, &info);
	//logMsg("%d %d %d", info.width, info.height, info.stride);
	return {};
}

bool BitmapFactoryImage::hasAlphaChannel()
{
	return info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 || info.format == ANDROID_BITMAP_FORMAT_RGBA_4444;
}

std::errc BitmapFactoryImage::readImage(IG::Pixmap dest)
{
	assumeExpr(dest.format() == pixelFormat());
	auto env = ctx.thisThreadJniEnv();
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
		auto env = ctx.thisThreadJniEnv();
		ctx.application().recycleBitmap(env, bitmap);
		env->DeleteLocalRef(std::exchange(bitmap, {}));
	}
}

BitmapFactoryImage::operator bool() const
{
	return (bool)bitmap;
}

PngFile::~PngFile()
{
	deinit();
}

std::errc PngFile::write(IG::Pixmap dest)
{
	return(png.readImage(dest));
}

IG::Pixmap PngFile::pixmapView()
{
	return {{{(int)png.width(), (int)png.height()}, png.pixelFormat()}, {}};
}

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
