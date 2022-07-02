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
#include <imagine/util/jni.hh>

namespace IG
{
class ApplicationContext;
class Application;
class PixelFormat;
}

namespace IG::Data
{

class BitmapFactoryImage
{
public:
	constexpr BitmapFactoryImage() = default;
	BitmapFactoryImage(JNI::LockedLocalBitmap, PixmapView);

protected:
	JNI::LockedLocalBitmap lockedBitmap{};
	PixmapView pixmap_{};
};

using PixmapImageImpl = BitmapFactoryImage;

class BitmapFactoryReader
{
public:
	BitmapFactoryReader(ApplicationContext ctx);

protected:
	Application *appPtr{};
	jobject baseActivity{};
	JNI::UniqueGlobalRef jBitmapFactory{};
	JNI::ClassMethod<jobject(jstring)> jDecodeFile{};
	JNI::InstMethod<jobject(jstring)> jDecodeAsset{};
	JNI::InstMethod<void()> jRecycleBitmap{};

	constexpr Application &app() const { return *appPtr; }
};

using PixmapReaderImpl = BitmapFactoryReader;

class BitmapWriter
{
public:
	BitmapWriter(ApplicationContext);

protected:
	Application *appPtr{};
	jobject baseActivity{};
	JNI::InstMethod<jobject(jint, jint, jint)> jMakeBitmap;
	JNI::InstMethod<jboolean(jobject, jobject)> jWritePNG;

	constexpr Application &app() const { return *appPtr; }
};

using PixmapWriterImpl = BitmapWriter;

}
