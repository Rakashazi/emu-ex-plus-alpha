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

#define LOGTAG "LibPNG"

#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/data-type/image/PixmapWriter.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FS.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/pixmap/MemPixmap.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/logger/logger.h>

#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

// this must be in the range 1 to 8
#define INITIAL_HEADER_READ_BYTES 8

#ifndef PNG_ERROR_TEXT_SUPPORTED

CLINK void PNGAPI png_error(png_const_structrp png_ptr, png_const_charp error_message) PNG_NORETURN;
CLINK void PNGAPI png_error(png_const_structrp png_ptr, png_const_charp error_message)
{
	// TODO: print out more verbose error
	bug_unreachable("fatal libpng error");
}

CLINK void PNGAPI png_chunk_error(png_const_structrp png_ptr, png_const_charp error_message) PNG_NORETURN;
CLINK void PNGAPI png_chunk_error(png_const_structrp png_ptr, png_const_charp error_message)
{
	png_error(png_ptr, error_message);
}

#endif

#ifndef PNG_WARNINGS_SUPPORTED

CLINK void PNGAPI png_warning(png_const_structrp png_ptr, png_const_charp warning_message)
{
	// TODO: print out more verbose warning
	logWarn("libpng warning");
}

CLINK void PNGAPI png_chunk_warning(png_const_structrp png_ptr, png_const_charp warning_message)
{
	png_warning(png_ptr, warning_message);
}

#endif

namespace IG::Data
{

bool PngImage::supportUncommonConv = 0;

static void png_ioReader(png_structp pngPtr, png_bytep data, png_size_t length)
{
	auto &io = *(IO*)png_get_io_ptr(pngPtr);
	if(io.read(data, length) != (ssize_t)length)
	{
		logErr("error reading png file");
		png_error(pngPtr, "Read Error");
	}
}

int PngImage::width()
{
	return png_get_image_width(png, info);
}

int PngImage::height()
{
	return png_get_image_height(png, info);
}

bool PngImage::isGrayscale()
{
	return !(png_get_color_type(png, info) & PNG_COLOR_MASK_COLOR);
}

PixelFormat PngImage::pixelFormat()
{
	bool grayscale = isGrayscale();
	bool alpha = hasAlphaChannel();
	if(grayscale)
		return alpha ? PixelFmtIA88 : PixelFmtI8;
	else
		return PixelFmtRGBA8888;
}

static png_voidp png_memAlloc(png_structp, png_size_t size)
{
	//log_mPrintf(LOG_MSG, "about to allocate %d bytes", size);
	return new uint8_t[size];
}

static void png_memFree(png_structp, png_voidp ptr)
{
	delete[] (uint8_t*)ptr;
}

PngImage::PngImage(IO io, PixmapReaderParams params):
	premultiplyAlpha{params.premultiplyAlpha}
{
	//logMsg("reading header from file handle @ %p",stream);
	
	//logMsg("initial reading to start at offset 0x%X", (int)stream->ftell);
	
	//log_mPrintf(LOG_MSG, "%d items %d size, %d", 10, 500, PNG_UINT_32_MAX/500);
	if(!io)
		return;
	std::array <uint8_t, INITIAL_HEADER_READ_BYTES> header;
	if(io.read(header).bytes != INITIAL_HEADER_READ_BYTES)
		return;

	int isPng = !png_sig_cmp(header.data(), 0, INITIAL_HEADER_READ_BYTES);
	if (!isPng)
	{
		logErr("error - not a png file");
		return;
	}

	png = png_create_read_struct_2 (PNG_LIBPNG_VER_STRING,
		/*(png_voidp)user_error_ptr*/NULL, /*user_error_fn*/NULL, /*user_warning_fn*/NULL,
		0, png_memAlloc, png_memFree);
	if (png == NULL)
	{
		logErr("error allocating png struct");
		return;
	}

	//log_mPrintf(LOG_MSG,"creating info struct");
	info = png_create_info_struct(png);
	if (info == NULL)
	{
		logErr("error allocating png info");
		png_structpp pngStructpAddr = &png;
		png_destroy_read_struct(pngStructpAddr, (png_infopp)NULL, (png_infopp)NULL);
		return;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		logErr("error occurred, jumped to setjmp");
		freeImageData();
		return;
	}
	
	//init custom libpng io
	png_set_read_fn(png, std::make_unique<IO>(std::move(io)).release(), png_ioReader);
	//log_mPrintf(LOG_MSG,"set custom png read function %p", png_ioReader);
	
	png_set_sig_bytes(png, INITIAL_HEADER_READ_BYTES);
	//log_mPrintf(LOG_MSG,"let libpng know we read %d bytes already", INITIAL_HEADER_READ_BYTES);
	
	png_read_info(png, info);
	//log_mPrintf(LOG_MSG,"finished reading header");
}

void PngImage::freeImageData()
{
	if(png)
	{
		logMsg("deallocating libpng data");
		delete (IO*)png_get_io_ptr(png);
		png_structpp pngStructpAddr = &png;
		png_infopp pngInfopAddr = &info;
		//void * pngEndInfopAddr = &data->end;
		png_destroy_read_struct(pngStructpAddr, pngInfopAddr, (png_infopp)NULL/*pngEndInfopAddr*/);
		assert(!png);
		assert(!info);
	}
}

bool PngImage::hasAlphaChannel()
{
	return ( (png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA) ||
				( png_get_valid(png, info, PNG_INFO_tRNS) ) ) ? 1 : 0;
}

void PngImage::setTransforms(PixelFormat outFormat)
{
	int addingAlphaChannel = 0;
	
	if(png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE)
	{
		//logMsg("converting paletted image to direct color");
		png_set_palette_to_rgb(png);
	}

	if(png_get_valid(png, info, PNG_INFO_tRNS))
	{
		//logMsg("coverting pallete transparency to alpha channel");
		png_set_tRNS_to_alpha(png);
		addingAlphaChannel = 1;
	}

	if(png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png, info) < 8)
	{
		logMsg("expanding gray-scale to 8-bits");
		png_set_expand_gray_1_2_4_to_8(png);
	}
	
	// convert 16-bits per channel to 8-bits per channel
	#ifndef PNG_NO_READ_16_TO_8
	if(png_get_bit_depth(png, info) == 16)
	{
		logMsg("converting 16-bits per channel to 8-bits per channel");
		png_set_strip_16(png);
	}
	#endif

	if((!hasAlphaChannel() && png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB)
			|| (!hasAlphaChannel() && outFormat.desc().aBits ))
	{
		logMsg("adding alpha channel");
			png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}

	if(premultiplyAlpha)
	{
		png_set_alpha_mode(png, PNG_ALPHA_STANDARD, PNG_GAMMA_LINEAR);
	}

	if(supportUncommonConv)
	{
		if((png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY || png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY_ALPHA) &&
				!(outFormat.isGrayscale()))
		{
			logMsg("converting gray-scale to RGB");
			png_set_gray_to_rgb(png);
		}

		if(!outFormat.desc().aBits)
		{
			if(png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA || addingAlphaChannel)
			{
				logMsg("removing alpha channel");
				png_set_strip_alpha(png);
			}
		}

		if(outFormat.desc().aBits && outFormat.desc().aShift != 0)
		{
			logMsg("swapping RGBA to ARGB");
			png_set_swap_alpha(png);
		}
	}
	
	if(outFormat.isBGROrder())
	{
		//logMsg("swaping colors to BGR order");
		png_set_bgr(png);
	}
	
	png_read_update_info(png, info);
}

std::errc PngImage::readImage(MutablePixmapView dest)
{
	int height = this->height();
	int width = this->width();

	png_infop transInfo = png_create_info_struct(png);
	if (!transInfo)
	{
		logErr("error allocating png transform info");
		return std::errc::not_enough_memory;
	}
	scopeGuard(
		[&]()
		{
			png_infopp pngInfopAddr = &transInfo;
			png_destroy_info_struct(png, pngInfopAddr);
		});
	setTransforms(dest.format());
	
	//log_mPrintf(LOG_MSG,"after transforms, rowbytes = %u", (uint32_t)png_get_rowbytes(data->png, data->info));

	assumeExpr( (uint32_t)width*dest.format().bytesPerPixel() == png_get_rowbytes(png, info) );
	
	if(png_get_interlace_type(png, info) == PNG_INTERLACE_NONE)
	{
		//logMsg("reading row at a time");
		if (setjmp(png_jmpbuf((png_structp)png)))
		{
			logErr("error reading image, jumped to setjmp");
			return std::errc::io_error;
		}

		for (int i = 0; i < height; i++)
		{
			png_read_row(png, (png_bytep)&dest[0, i], nullptr);
		}
	}
	else // read the whole image in 1 call with interlace handling, but needs array of row pointers allocated
	{
		logMsg("is interlaced");
		png_bytep rowPtr[height];
		for (int i = 0; i < height; i++)
		{
			//logr_mPrintf(LOG_MSG,row relative offset = %d", offset);
			rowPtr[i] = (png_bytep)&dest[0, i];
			//log_mPrintf(LOG_MSG, "set row pointer %d to %p", i, row_pointers[i]);
		}

		if (setjmp(png_jmpbuf((png_structp)png)))
		{
			logErr("error reading image, jumped to setjmp");
			return std::errc::io_error;
		}

		png_read_image(png, rowPtr);
	}
	
	//log_mPrintf(LOG_MSG,"reading complete");
	png_read_end(png, nullptr);
	return {};
}

PixmapImage::operator bool() const
{
	return info;
}

PngImage::PngImage(PngImage &&o) noexcept
{
	*this = std::move(o);
}

PngImage &PngImage::operator=(PngImage &&o) noexcept
{
	freeImageData();
	png = std::exchange(o.png, {});
	info = std::exchange(o.info, {});
	return *this;
}

PngImage::~PngImage()
{
	freeImageData();
}

void PixmapImage::write(MutablePixmapView dest)
{
	readImage(dest);
}

PixmapView PixmapImage::pixmapView()
{
	return PixmapView{{{width(), height()}, pixelFormat()}};
}

PixmapImage::operator PixmapSource()
{
	return {[this](MutablePixmapView dest){ return write(dest); }, pixmapView()};
}

bool PixmapImage::isPremultipled() const { return premultiplyAlpha; }

PixmapImage PixmapReader::load(IO io, PixmapReaderParams params) const
{
	return PixmapImage{std::move(io), params};
}

PixmapImage PixmapReader::load(const char *name, PixmapReaderParams params) const
{
	if(!std::string_view{name}.ends_with(".png"))
	{
		logErr("suffix doesn't match PNG image");
		return {};
	}
	return load(FileIO{name, {.test = true, .accessHint = IOAccessHint::All}}, params);
}

PixmapImage PixmapReader::loadAsset(const char *name, PixmapReaderParams params, const char *appName) const
{
	return load(appContext().openAsset(name, {.accessHint = IOAccessHint::All}, appName), params);
}

bool PixmapWriter::writeToFile(PixmapView pix, const char *path) const
{
	FileIO fp{path, OpenFlags::testNewFile()};
	if(!fp)
	{
		return false;
	}
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!pngPtr)
	{
		FS::remove(path);
		return false;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr)
	{
		png_destroy_write_struct(&pngPtr, (png_infopp)NULL);
		FS::remove(path);
		return false;
	}
	if(setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_write_struct(&pngPtr, &infoPtr);
		FS::remove(path);
		return false;
	}
	png_set_write_fn(pngPtr, &fp,
		[](png_structp pngPtr, png_bytep data, png_size_t length)
		{
			auto &io = *(FileIO*)png_get_io_ptr(pngPtr);
			if(io.write(data, length) != (ssize_t)length)
			{
				logErr("error writing png file");
				//png_error(pngPtr, "Write Error");
			}
		},
		[](png_structp)
		{
			logMsg("called png_ioFlush");
		});
	png_set_IHDR(pngPtr, infoPtr, pix.w(), pix.h(), 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(pngPtr, infoPtr);
	{
		MemPixmap tempMemPix{{pix.size(), PixelFmtRGB888}};
		auto tempPix = tempMemPix.view();
		tempPix.writeConverted(pix);
		int rowBytes = png_get_rowbytes(pngPtr, infoPtr);
		assert(rowBytes == tempPix.pitchBytes());
		auto rowData = (png_const_bytep)tempPix.data();
		for([[maybe_unused]] auto i : iotaCount(tempPix.h()))
		{
			png_write_row(pngPtr, rowData);
			rowData += tempPix.pitchBytes();
		}
	}
	png_write_end(pngPtr, infoPtr);
	png_destroy_write_struct(&pngPtr, &infoPtr);
	return true;
}

}
