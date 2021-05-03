/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Screenshot.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/logger/logger.h>
#include <imagine/data-type/image/sys.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/string.h>

#ifdef CONFIG_DATA_TYPE_IMAGE_QUARTZ2D

bool writeScreenshot(Base::ApplicationContext, IG::Pixmap vidPix, const char *fname)
{
	auto screen = vidPix.data();
	IG::MemPixmap tempMemPix{{vidPix.size(), IG::PIXEL_FMT_RGB888}};
	auto tempPix = tempMemPix.view();
	tempPix.writeConverted(vidPix);
	Quartz2dImage::writeImage(tempPix, fname);
	return true;
}

#elif defined CONFIG_DATA_TYPE_IMAGE_ANDROID

#include <imagine/util/jni.hh>

// TODO: make png writer module in imagine
bool writeScreenshot(Base::ApplicationContext ctx, IG::Pixmap vidPix, const char *fname)
{
	static JNI::InstMethod<jobject(jint, jint, jint)> jMakeBitmap;
	static JNI::InstMethod<jboolean(jobject, jobject)> jWritePNG;
	using namespace Base;
	auto env = ctx.thisThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	if(!jMakeBitmap)
	{
		auto baseActivityCls = env->GetObjectClass(baseActivity);
		jMakeBitmap = {env, baseActivityCls, "makeBitmap", "(III)Landroid/graphics/Bitmap;"};
		jWritePNG = {env, baseActivityCls, "writePNG", "(Landroid/graphics/Bitmap;Ljava/lang/String;)Z"};
	}
	auto aFormat = vidPix.format().id() == PIXEL_RGB565 ? ANDROID_BITMAP_FORMAT_RGB_565 : ANDROID_BITMAP_FORMAT_RGBA_8888;
	auto bitmap = jMakeBitmap(env, baseActivity, vidPix.w(), vidPix.h(), aFormat);
	if(!bitmap)
	{
		logErr("error allocating bitmap");
		return false;
	}
	void *buffer;
	AndroidBitmap_lockPixels(env, bitmap, &buffer);
	Base::makePixmapView(env, bitmap, buffer, vidPix.format()).write(vidPix, {});
	AndroidBitmap_unlockPixels(env, bitmap);
	auto nameJStr = env->NewStringUTF(fname);
	auto writeOK = jWritePNG(env, baseActivity, bitmap, nameJStr);
	env->DeleteLocalRef(nameJStr);
	env->DeleteLocalRef(bitmap);
	if(!writeOK)
	{
		logErr("error writing PNG");
		return false;
	}
	return true;
}

#else

#if PNG_LIBPNG_VER < 10500
using png_const_bytep = png_bytep;
#endif

bool writeScreenshot(Base::ApplicationContext, IG::Pixmap vidPix, const char *fname)
{
	FileIO fp;
	fp.create(fname);
	if(!fp)
	{
		return false;
	}
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!pngPtr)
	{
		FS::remove(fname);
		return false;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr)
	{
		png_destroy_write_struct(&pngPtr, (png_infopp)NULL);
		FS::remove(fname);
		return false;
	}
	if(setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_write_struct(&pngPtr, &infoPtr);
		FS::remove(fname);
		return false;
	}
	png_set_write_fn(pngPtr, &fp,
		[](png_structp pngPtr, png_bytep data, png_size_t length)
		{
			auto &io = *(IO*)png_get_io_ptr(pngPtr);
			if(io.write(data, length) != (ssize_t)length)
			{
				logErr("error writing png file");
				//png_error(pngPtr, "Write Error");
			}
		},
		[](png_structp pngPtr)
		{
			logMsg("called png_ioFlush");
		});
	png_set_IHDR(pngPtr, infoPtr, vidPix.w(), vidPix.h(), 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(pngPtr, infoPtr);
	{
		IG::MemPixmap tempMemPix{{vidPix.size(), IG::PIXEL_FMT_RGB888}};
		auto tempPix = tempMemPix.view();
		tempPix.writeConverted(vidPix);
		uint32_t rowBytes = png_get_rowbytes(pngPtr, infoPtr);
		assert(rowBytes == tempPix.pitchBytes());
		auto rowData = (png_const_bytep)tempPix.data();
		iterateTimes(tempPix.h(), i)
		{
			png_write_row(pngPtr, rowData);
			rowData += tempPix.pitchBytes();
		}
	}
	png_write_end(pngPtr, infoPtr);
	png_destroy_write_struct(&pngPtr, &infoPtr);
	return true;
}

#endif

int sprintScreenshotFilename(FS::PathString &str)
{
	const unsigned maxNum = 999;
	int num = -1;
	iterateTimes(maxNum, i)
	{
		string_printf(str, "%s/%s.%.3d.png", EmuSystem::savePath(), EmuSystem::gameName().data(), i);
		if(!FS::exists(str))
		{
			num = i;
			break;
		}
	}
	if(num == -1)
	{
		logMsg("no screenshot filenames left");
		return -1;
	}
	logMsg("screenshot %d", num);
	return num;
}
