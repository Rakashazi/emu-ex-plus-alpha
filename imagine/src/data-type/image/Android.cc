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
#include <imagine/data-type/image/PixmapSource.hh>
#include "../../base/android/android.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include <android/bitmap.h>

namespace IG::Data
{

BitmapFactoryReader::BitmapFactoryReader(ApplicationContext ctx):
	appPtr{&ctx.application()},
	baseActivity{ctx.baseActivityObject()},
	jRecycleBitmap{ctx.application().recycleBitmapMethod()}
{
	auto env = ctx.mainThreadJniEnv();
	jBitmapFactory = {env, env->FindClass("android/graphics/BitmapFactory")};
	jDecodeFile = {env, (jclass)jBitmapFactory, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;"};
	jDecodeAsset = {env, baseActivity, "bitmapDecodeAsset", "(Ljava/lang/String;)Landroid/graphics/Bitmap;"};
}

static PixmapImage makePixmapImage(JNIEnv *env, jobject bitmap, JNI::InstMethod<void()> recycle)
{
	void *buff;
	AndroidBitmap_lockPixels(env, bitmap, &buff);
	auto pix = makePixmapView(env, bitmap, buff, {});
	return PixmapImage{{env, bitmap, recycle}, pix};
}

PixmapImage PixmapReader::load(const char *name, PixmapReaderParams) const
{
	auto env = app().thisThreadJniEnv();
	auto nameJStr = env->NewStringUTF(name);
	auto bitmap = jDecodeFile(env, (jclass)jBitmapFactory, nameJStr);
	env->DeleteLocalRef(nameJStr);
	if(!bitmap)
	{
		logErr("couldn't decode file:%s", name);
		return {};
	}
	return makePixmapImage(env, bitmap, jRecycleBitmap);
}

PixmapImage PixmapReader::loadAsset(const char *name, PixmapReaderParams, const char *) const
{
	logMsg("loading asset: %s", name);
	auto env = app().thisThreadJniEnv();
	auto nameJStr = env->NewStringUTF(name);
	auto bitmap = jDecodeAsset(env, baseActivity, nameJStr);
	env->DeleteLocalRef(nameJStr);
	if(!bitmap)
	{
		logErr("couldn't decode asset:%s", name);
		return {};
	}
	return makePixmapImage(env, bitmap, jRecycleBitmap);
}

BitmapFactoryImage::BitmapFactoryImage(JNI::LockedLocalBitmap lockedBitmap, PixmapView pix):
	lockedBitmap{std::move(lockedBitmap)}, pixmap_{pix} {}

PixmapImage::operator bool() const
{
	return (bool)lockedBitmap;
}

void PixmapImage::write(MutablePixmapView dest)
{
	dest.write(pixmap_, {});
}

PixmapView PixmapImage::pixmapView()
{
	return pixmap_;
}

PixmapImage::operator PixmapSource()
{
	return {pixmapView()};
}

bool PixmapImage::isPremultipled() const { return true; }

BitmapWriter::BitmapWriter(ApplicationContext ctx):
	appPtr{&ctx.application()},
	baseActivity{ctx.baseActivityObject()}
{
	auto env = ctx.mainThreadJniEnv();
	auto baseActivityCls = env->GetObjectClass(baseActivity);
	jMakeBitmap = {env, baseActivityCls, "makeBitmap", "(III)Landroid/graphics/Bitmap;"};
	jWritePNG = {env, baseActivityCls, "writePNG", "(Landroid/graphics/Bitmap;Ljava/lang/String;)Z"};
}

bool PixmapWriter::writeToFile(PixmapView pix, const char *path) const
{
	auto env = app().thisThreadJniEnv();
	auto aFormat = pix.format() == PixelFmtRGB565 ? ANDROID_BITMAP_FORMAT_RGB_565 : ANDROID_BITMAP_FORMAT_RGBA_8888;
	auto bitmap = jMakeBitmap(env, baseActivity, pix.w(), pix.h(), aFormat);
	if(!bitmap)
	{
		logErr("error allocating bitmap");
		return false;
	}
	void *buffer;
	AndroidBitmap_lockPixels(env, bitmap, &buffer);
	makePixmapView(env, bitmap, buffer, pix.format()).writeConverted(pix, {});
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
