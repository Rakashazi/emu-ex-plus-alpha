#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/font/FontSettings.hh>
#include <imagine/util/jni.hh>
#include <utility>

namespace IG
{

class ApplicationContext;
class Application;
class FontManager;
struct GlyphMetrics;

class AndroidGlyphImage
{
public:
	constexpr AndroidGlyphImage() = default;
	AndroidGlyphImage(JNI::LockedLocalBitmap, PixmapView);

protected:
	JNI::LockedLocalBitmap lockedBitmap{};
	PixmapView pixmap_{};
};

class AndroidFont
{
public:
	constexpr AndroidFont() = default;
	constexpr AndroidFont(const FontManager &manager, FontWeight weight = {}):
		managerPtr{&manager},
		weight{weight}
	{}

protected:
	const FontManager *managerPtr{};
	FontWeight weight{};

	constexpr const FontManager &manager() const { return *managerPtr; }
};

class AndroidFontSize
{
public:
	constexpr AndroidFontSize() = default;
	AndroidFontSize(JNI::UniqueGlobalRef paint);
	jobject paint() const { return paint_; }

protected:
	JNI::UniqueGlobalRef paint_{};
};

class AndroidFontManager
{
public:
	AndroidFontManager(ApplicationContext);
	std::pair<jobject, GlyphMetrics> makeBitmap(JNIEnv*, int idx, AndroidFontSize &) const;
	GlyphMetrics makeMetrics(JNIEnv*, int idx, AndroidFontSize &) const;
	jobject makePaint(JNIEnv*, int pixelHeight, FontWeight) const;
	constexpr Application &app() const { return *appPtr; }
	constexpr JNI::InstMethod<void()> recycleBitmapMethod() const { return jRecycleBitmap; }

protected:
	Application *appPtr{};
	JNI::UniqueGlobalRef renderer{};
	JNI::InstMethod<jobject(jint, jobject, jlong)> jBitmap{};
	JNI::InstMethod<void(jint, jobject, jlong)> jMetrics{};
	JNI::InstMethod<jobject(jint, jboolean)> jMakePaint{};
	JNI::InstMethod<void()> jRecycleBitmap{};
};

using GlyphImageImpl = AndroidGlyphImage;
using FontImpl = AndroidFont;
using FontSize = AndroidFontSize;
using FontManagerImpl = AndroidFontManager;

}
