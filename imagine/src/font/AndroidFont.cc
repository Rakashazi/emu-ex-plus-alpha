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

constexpr auto androidBitmapResultToStr(int result)
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

AndroidFontManager::AndroidFontManager(ApplicationContext ctx):
	appPtr{&ctx.application()},
	jRecycleBitmap{ctx.application().recycleBitmapMethod()}
{
	auto env = ctx.mainThreadJniEnv();
	auto fontRenderer = app().makeFontRenderer(env, ctx.baseActivityObject());
	auto jFontRendererCls = env->GetObjectClass(fontRenderer);
	renderer = {env, fontRenderer};
	jBitmap = {env, jFontRendererCls, "bitmap", "(ILandroid/graphics/Paint;J)Landroid/graphics/Bitmap;"};
	jMetrics = {env, jFontRendererCls, "metrics", "(ILandroid/graphics/Paint;J)V"};
	jMakePaint = {env, jFontRendererCls, "makePaint", "(IZ)Landroid/graphics/Paint;"};
	JNINativeMethod method[]
	{
		{
			"charMetricsCallback", "(JIIIII)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jint, jint, jint, jint, jint))
			([](JNIEnv*, jobject, jlong metricsAddr,
					jint xSize, jint ySize, jint xOff, jint yOff, jint xAdv)
			{
				auto &metrics = *((GlyphMetrics*)metricsAddr);
				metrics.size = {int16_t(xSize), int16_t(ySize)};
				metrics.offset = {int16_t(xOff), int16_t(yOff)};
				metrics.xAdvance = xAdv;
				/*logDMsg("char metrics: size %dx%d offset %dx%d advance %d", metrics.xSize, metrics.ySize,
					metrics.xOffset, metrics.yOffset, metrics.xAdvance);*/
			})
		},
	};
	env->RegisterNatives(jFontRendererCls, method, std::size(method));
}

Font FontManager::makeSystem() const
{
	return {*this};
}

Font FontManager::makeBoldSystem() const
{
	return {*this, FontWeight::BOLD};
}

std::pair<jobject, GlyphMetrics> AndroidFontManager::makeBitmap(JNIEnv *env, int idx, AndroidFontSize &size) const
{
	GlyphMetrics metrics{};
	auto bitmap = jBitmap(env, renderer, idx, size.paint(), (jlong)&metrics);
	if(!bitmap)
	{
		return {};
	}
	return {bitmap, metrics};
}

GlyphMetrics AndroidFontManager::makeMetrics(JNIEnv *env, int idx, AndroidFontSize &size) const
{
	GlyphMetrics metrics{};
	jMetrics(env, renderer, idx, size.paint(), (jlong)&metrics);
	return metrics;
}

jobject AndroidFontManager::makePaint(JNIEnv *env, int pixelHeight, FontWeight weight) const
{
	return jMakePaint(env, renderer, pixelHeight, weight == FontWeight::BOLD);
}

Font::operator bool() const
{
	return true;
}

Font::Glyph Font::glyph(int idx, FontSize &size)
{
	auto &mgr = manager();
	auto env = mgr.app().thisThreadJniEnv();
	auto [bitmap, metrics] = mgr.makeBitmap(env, idx, size);
	if(!bitmap)
		return {};
	void *data{};
	[[maybe_unused]] auto res = AndroidBitmap_lockPixels(env, bitmap, &data);
	auto pix = makePixmapView(env, bitmap, data, PixelFmtA8);
	//logMsg("AndroidBitmap_lockPixels returned %s", androidBitmapResultToStr(res));
	assert(res == ANDROID_BITMAP_RESULT_SUCCESS);
	return {{{env, bitmap, mgr.recycleBitmapMethod()}, pix}, metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size)
{
	auto &mgr = manager();
	auto env = mgr.app().thisThreadJniEnv();
	auto metrics = mgr.makeMetrics(env, idx, size);
	if(!metrics)
		return {};
	return metrics;
}

FontSize Font::makeSize(FontSettings settings)
{
	auto &mgr = manager();
	auto env = mgr.app().thisThreadJniEnv();
	auto paint = mgr.makePaint(env, settings.pixelHeight(), weight);
	if(!paint)
		return {};
	logMsg("allocated new size %dpx @ %p", settings.pixelHeight(), paint);
	return {{env, paint}};
}

AndroidFontSize::AndroidFontSize(JNI::UniqueGlobalRef paint):
	paint_{std::move(paint)}
{}

AndroidGlyphImage::AndroidGlyphImage(JNI::LockedLocalBitmap lockedBitmap, PixmapView pixmap):
	lockedBitmap{std::move(lockedBitmap)},	pixmap_{pixmap} {}

PixmapView GlyphImage::pixmap()
{
	return pixmap_;
}

GlyphImage::operator bool() const
{
	return (bool)lockedBitmap;
}

}
