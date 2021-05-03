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

#define LOGTAG "AndroidFont"
#include <imagine/font/Font.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/logger.h>
#include "../base/android/android.hh"
#include <android/bitmap.h>

namespace IG
{

static JNI::InstMethod<jobject(jint, jobject, jlong)> jBitmap{};
static JNI::InstMethod<void(jint, jobject, jlong)> jMetrics{};
static JNI::InstMethod<jobject(jint, jboolean)> jMakePaint{};
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
	auto jFontRendererCls = env->GetObjectClass(renderer);
	jBitmap = {env, jFontRendererCls, "bitmap", "(ILandroid/graphics/Paint;J)Landroid/graphics/Bitmap;"};
	jMetrics = {env, jFontRendererCls, "metrics", "(ILandroid/graphics/Paint;J)V"};
	jMakePaint = {env, jFontRendererCls, "makePaint", "(IZ)Landroid/graphics/Paint;"};
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
				/*logDMsg("char metrics: size %dx%d offset %dx%d advance %d", metrics.xSize, metrics.ySize,
					metrics.xOffset, metrics.yOffset, metrics.xAdvance);*/
			})
		},
	};
	env->RegisterNatives(jFontRendererCls, method, std::size(method));
}

Font Font::makeSystem(Base::ApplicationContext ctx)
{
	auto env = ctx.mainThreadJniEnv();
	Font font{ctx};
	if(!renderer) [[unlikely]]
	{
		logMsg("making renderer");
		renderer = ctx.application().makeFontRenderer(env, ctx.baseActivityObject());
		setupResourceFontAndroidJni(env, renderer);
		if(env->ExceptionCheck())
		{
			logErr("exception");
			env->ExceptionClear();
			return {ctx};
		}
		renderer = env->NewGlobalRef(renderer);
	}
	return font;
}

Font Font::makeBoldSystem(Base::ApplicationContext ctx)
{
	Font font = makeSystem(ctx);
	font.isBold = true;
	return font;
}

Font::operator bool() const
{
	return true;
}

Font::Glyph Font::glyph(int idx, FontSize &size, std::errc &ec)
{
	auto env = ctx.thisThreadJniEnv();
	GlyphMetrics metrics{};
	auto lockedBitmap = jBitmap(env, renderer, idx, size.paint(), (jlong)&metrics);
	if(!lockedBitmap)
	{
		ec = std::errc::invalid_argument;
		return {};
	}
	ec = {};
	//logMsg("got bitmap @ %p", lockedBitmap);
	//logMsg("size %dx%d, pitch %d", info.width, info.height, info.stride);
	void *data{};
	auto res = AndroidBitmap_lockPixels(env, lockedBitmap, &data);
	//logMsg("AndroidBitmap_lockPixels returned %s", androidBitmapResultToStr(res));
	assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
	return {{ctx, Base::makePixmapView(env, lockedBitmap, data, PIXEL_A8), lockedBitmap}, metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size, std::errc &ec)
{
	auto env = ctx.thisThreadJniEnv();
	GlyphMetrics metrics{};
	jMetrics(env, renderer, idx, size.paint(), (jlong)&metrics);
	if(!metrics.xSize)
	{
		ec = std::errc::invalid_argument;
		return {};
	}
	ec = {};
	return metrics;
}

FontSize Font::makeSize(FontSettings settings, std::errc &ec)
{
	auto env = ctx.thisThreadJniEnv();
	auto paint = jMakePaint(env, renderer, settings.pixelHeight(), isBold);
	if(!paint)
	{
		ec = std::errc::invalid_argument;
		return {};
	}
	logMsg("allocated new size %dpx @ 0x%p", settings.pixelHeight(), paint);
	ec = {};
	return {{env, paint}};
}

AndroidFontSize::AndroidFontSize(JNI::UniqueGlobalRef paint):
	paint_{std::move(paint)}
{}

AndroidGlyphImage::AndroidGlyphImage(Base::ApplicationContext ctx, IG::Pixmap pixmap, jobject bitmap):
	pixmap_{pixmap},
	aBitmap{bitmap},
	ctx{ctx}
{}

AndroidGlyphImage::AndroidGlyphImage(AndroidGlyphImage &&o)
{
	*this = std::move(o);
}

AndroidGlyphImage &AndroidGlyphImage::operator=(AndroidGlyphImage &&o)
{
	static_cast<GlyphImage*>(this)->unlock();
	pixmap_ = o.pixmap_;
	aBitmap = std::exchange(o.aBitmap, {});
	ctx = o.ctx;
	return *this;
}

AndroidGlyphImage::~AndroidGlyphImage()
{
	static_cast<GlyphImage*>(this)->unlock();
}

void GlyphImage::unlock()
{
	if(!aBitmap)
		return;
	auto env = ctx.thisThreadJniEnv();
	AndroidBitmap_unlockPixels(env, aBitmap);
	ctx.application().recycleBitmap(env, aBitmap);
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
