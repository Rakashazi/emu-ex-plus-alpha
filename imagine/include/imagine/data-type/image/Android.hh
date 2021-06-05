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
#include <imagine/base/ApplicationContext.hh>
#include <android/bitmap.h>
#include <imagine/util/jni.hh>
#include <system_error>

namespace IG
{
class PixelFormat;
}

namespace IG::Data
{

class BitmapFactoryImage
{
public:
	constexpr BitmapFactoryImage(Base::ApplicationContext ctx):
		ctx{ctx}
	{}
	~BitmapFactoryImage();
	std::errc readImage(IG::Pixmap dest);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint32_t width();
	uint32_t height();
	IG::PixelFormat pixelFormat() const;
	constexpr Base::ApplicationContext appContext() const { return ctx; }

protected:
	jobject bitmap{};
	Base::ApplicationContext ctx{};
	AndroidBitmapInfo info{};
};

using PixmapReaderImpl = BitmapFactoryImage;

class BitmapWriter
{
public:
	BitmapWriter(Base::ApplicationContext);

protected:
	Base::ApplicationContext ctx{};
	JNI::InstMethod<jobject(jint, jint, jint)> jMakeBitmap;
	JNI::InstMethod<jboolean(jobject, jobject)> jWritePNG;
};

using PixmapWriterImpl = BitmapWriter;

}
