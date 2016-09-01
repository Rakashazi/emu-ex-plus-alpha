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
#include <imagine/font/Font.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include "../base/android/android.hh"
#include <android/bitmap.h>

namespace IG
{

static JavaInstMethod<jobject(jint, jobject, jlong)> jBitmap{};
static JavaInstMethod<void(jint, jobject, jlong)> jMetrics{};
static JavaClassMethod<jobject(jint, jboolean)> jMakePaint{};
static jclass jFontRendererCls{};
static jobject renderer{};

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

static void setupResourceFontAndroidJni(JNIEnv *env, jobject renderer)
{
	if(jBitmap)
		return; // already setup
	//logMsg("setting up JNI methods");
	jFontRendererCls = env->GetObjectClass(renderer);
	jFontRendererCls = (jclass)env->NewGlobalRef(jFontRendererCls);
	jBitmap.setup(env, jFontRendererCls, "bitmap", "(ILandroid/graphics/Paint;J)Landroid/graphics/Bitmap;");
	jMetrics.setup(env, jFontRendererCls, "metrics", "(ILandroid/graphics/Paint;J)V");
	jMakePaint.setup(env, jFontRendererCls, "makePaint", "(IZ)Landroid/graphics/Paint;");
	JNINativeMethod method[]
	{
		{
			"charMetricsCallback", "(JIIIII)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint, jint, jint, jint))
			([](JNIEnv* env, jobject thiz, jlong metricsAddr,
					jint xSize, jint ySize, jint xOff, jint yOff, jint xAdv)
			{
				auto &metrics = *((GlyphMetrics*)metricsAddr);
				metrics.xSize = xSize;
				metrics.ySize = ySize;
				metrics.xOffset = xOff;
				metrics.yOffset = yOff;
				metrics.xAdvance = xAdv;
				logMsg("char metrics: size %dx%d offset %dx%d advance %d", metrics.xSize, metrics.ySize,
					metrics.xOffset, metrics.yOffset, metrics.xAdvance);
			})
		},
	};
	env->RegisterNatives(jFontRendererCls, method, IG::size(method));
}

Font::Font() {}

Font Font::makeSystem()
{
	auto env = Base::jEnv();
	Font font{};
	if(unlikely(!renderer))
	{
		renderer = Base::newFontRenderer(env);
		setupResourceFontAndroidJni(env, renderer);
		if(env->ExceptionCheck())
		{
			logErr("exception");
			env->ExceptionClear();
			return {};
		}
		renderer = env->NewGlobalRef(renderer);
	}
	return font;
}

Font Font::makeBoldSystem()
{
	Font font = makeSystem();
	font.isBold = true;
	return font;
}

Font::Font(Font &&o)
{
	swap(*this, o);
}

Font &Font::operator=(Font o)
{
	swap(*this, o);
	return *this;
}

void Font::swap(Font &a, Font &b)
{
	std::swap(a.isBold, b.isBold);
}

Font::~Font() {}

Font::operator bool() const
{
	return true;
}

Font::Glyph Font::glyph(int idx, FontSize &size, std::error_code &ec)
{
	auto env = Base::jEnv();
	GlyphMetrics metrics{};
	auto lockedBitmap = jBitmap(env, renderer, idx, size.paint(), (jlong)&metrics);
	if(!lockedBitmap)
	{
		ec = {EINVAL, std::system_category()};
		return {};
	}
	ec = {};
	//logMsg("got bitmap @ %p", lockedBitmap);
	AndroidBitmapInfo info;
	auto res = AndroidBitmap_getInfo(env, lockedBitmap, &info);
	//logMsg("AndroidBitmap_getInfo returned %s", androidBitmapResultToStr(res));
	assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
	//logMsg("size %dx%d, pitch %d", info.width, info.height, info.stride);
	void *data{};
	res = AndroidBitmap_lockPixels(env, lockedBitmap, &data);
	//logMsg("AndroidBitmap_lockPixels returned %s", androidBitmapResultToStr(res));
	assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
	IG::Pixmap pixmap{{{(int)info.width, (int)info.height}, PIXEL_A8}, data, {info.stride, IG::Pixmap::BYTE_UNITS}};
	return {{pixmap, lockedBitmap}, metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size, std::error_code &ec)
{
	auto env = Base::jEnv();
	GlyphMetrics metrics{};
	jMetrics(env, renderer, idx, size.paint(), (jlong)&metrics);
	if(!metrics.xSize)
	{
		ec = {EINVAL, std::system_category()};
		return {};
	}
	ec = {};
	return metrics;
}

FontSize Font::makeSize(FontSettings settings, std::error_code &ec)
{
	auto env = Base::jEnv();
	auto paint = jMakePaint(env, jFontRendererCls, settings.pixelHeight(), isBold);
	if(!paint)
	{
		ec = {EINVAL, std::system_category()};
		return {};
	}
	logMsg("allocated new size %dpx @ 0x%p", settings.pixelHeight(), paint);
	ec = {};
	return {env->NewGlobalRef(paint)};
}

AndroidFontSize::AndroidFontSize(AndroidFontSize &&o)
{
	swap(*this, o);
}

AndroidFontSize &AndroidFontSize::operator=(AndroidFontSize o)
{
	swap(*this, o);
	return *this;
}

AndroidFontSize::~AndroidFontSize()
{
	if(!paint_)
		return;
	Base::jEnv()->DeleteGlobalRef(paint_);
}

void AndroidFontSize::swap(AndroidFontSize &a, AndroidFontSize &b)
{
	std::swap(a.paint_, b.paint_);
}

GlyphImage::GlyphImage(GlyphImage &&o)
{
	swap(*this, o);
}

GlyphImage &GlyphImage::operator=(GlyphImage o)
{
	swap(*this, o);
	return *this;
}

void GlyphImage::swap(GlyphImage &a, GlyphImage &b)
{
	std::swap(a.pixmap_, b.pixmap_);
	std::swap(a.aBitmap, b.aBitmap);
}

void GlyphImage::unlock()
{
	if(!aBitmap)
		return;
	auto env = Base::jEnv();
	AndroidBitmap_unlockPixels(env, aBitmap);
	Base::recycleBitmap(env, aBitmap);
	env->DeleteLocalRef(aBitmap);
	aBitmap = {};
}

IG::Pixmap GlyphImage::pixmap()
{
	return pixmap_;
}

GlyphImage::operator bool() const
{
	return aBitmap;
}

}
