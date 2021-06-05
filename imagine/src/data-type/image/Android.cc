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

#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/data-type/image/PixmapWriter.hh>
#include "../../base/android/android.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>

namespace IG::Data
{

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

std::error_code PixmapReader::load(const char *name)
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

std::error_code PixmapReader::loadAsset(const char *name, const char *)
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

PixmapReader::operator bool() const
{
	return (bool)bitmap;
}

BitmapFactoryImage::~BitmapFactoryImage()
{
	freeImageData();
}

std::errc PixmapReader::write(IG::Pixmap dest)
{
	return(readImage(dest));
}

IG::Pixmap PixmapReader::pixmapView()
{
	return {{{(int)width(), (int)height()}, pixelFormat()}, {}};
}

void PixmapReader::reset()
{
	freeImageData();
}

BitmapWriter::BitmapWriter(Base::ApplicationContext ctx):
	ctx{ctx}
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	auto baseActivityCls = env->GetObjectClass(baseActivity);
	jMakeBitmap = {env, baseActivityCls, "makeBitmap", "(III)Landroid/graphics/Bitmap;"};
	jWritePNG = {env, baseActivityCls, "writePNG", "(Landroid/graphics/Bitmap;Ljava/lang/String;)Z"};
}

bool PixmapWriter::writeToFile(IG::Pixmap pix, const char *path)
{
	using namespace Base;
	auto env = ctx.thisThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	auto aFormat = pix.format().id() == PIXEL_RGB565 ? ANDROID_BITMAP_FORMAT_RGB_565 : ANDROID_BITMAP_FORMAT_RGBA_8888;
	auto bitmap = jMakeBitmap(env, baseActivity, pix.w(), pix.h(), aFormat);
	if(!bitmap)
	{
		logErr("error allocating bitmap");
		return false;
	}
	void *buffer;
	AndroidBitmap_lockPixels(env, bitmap, &buffer);
	Base::makePixmapView(env, bitmap, buffer, pix.format()).writeConverted(pix, {});
	AndroidBitmap_unlockPixels(env, bitmap);
	auto pathJStr = env->NewStringUTF(path);
	auto writeOK = jWritePNG(env, baseActivity, bitmap, pathJStr);
	if(!writeOK)
	{
		logErr("error writing PNG");
		return false;
	}
	return true;
}

}
