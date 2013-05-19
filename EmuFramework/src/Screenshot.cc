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

#include <Screenshot.hh>
#include <data-type/image/libpng/reader.h>
#include <io/sys.hh>
#include <fs/sys.hh>

static void png_ioWriter(png_structp pngPtr, png_bytep data, png_size_t length)
{
	Io *stream = (Io*)png_get_io_ptr(pngPtr);

	if(stream->fwrite(data, length, 1) != 1)
	{
		logErr("error writing png file");
		//png_error(pngPtr, "Write Error");
	}
}

static void png_ioFlush(png_structp pngPtr)
{
	logMsg("called png_ioFlush");
}

bool writeScreenshot(const Pixmap &vidPix, const char *fname)
{
	Io *fp = IoSys::create(fname);
	if(!fp)
	{
		return 0;
	}

	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!pngPtr)
	{
		delete fp;
		FsSys::remove(fname);
		return 0;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr)
	{
		png_destroy_write_struct(&pngPtr, (png_infopp)NULL);
		delete fp;
		FsSys::remove(fname);
		return 0;
	}

	if(setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_write_struct(&pngPtr, &infoPtr);
		delete fp;
		FsSys::remove(fname);
		return 0;
	}

	uint imgwidth = vidPix.x;
	uint imgheight = vidPix.y;

	png_set_write_fn(pngPtr, fp, png_ioWriter, png_ioFlush);
	png_set_IHDR(pngPtr, infoPtr, imgwidth, imgheight, 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(pngPtr, infoPtr);

	//png_set_packing(pngPtr);

	png_byte *rowPtr= (png_byte*)mem_alloc(png_get_rowbytes(pngPtr, infoPtr));
	uint8 *screen = vidPix.data;
	for(uint y=0; y < vidPix.y; y++, screen+=vidPix.pitch)
	{
		png_byte *rowpix = rowPtr;
		for(uint x=0; x < vidPix.x; x++)
		{
			// assumes RGB565
			uint16 pixVal = *(uint16 *)(screen+2*x);
			uint32 r = pixVal >> 11, g = (pixVal >> 5) & 0x3f, b = pixVal & 0x1f;
			r *= 8; g *= 4; b *= 8;
			*(rowpix++) = r;
			*(rowpix++) = g;
			*(rowpix++) = b;
			if(imgwidth!=vidPix.x)
			{
				*(rowpix++) = r;
				*(rowpix++) = g;
				*(rowpix++) = b;
			}
		}
		png_write_row(pngPtr, rowPtr);
		if(imgheight!=vidPix.y)
			png_write_row(pngPtr, rowPtr);
	}

	mem_free(rowPtr);

	png_write_end(pngPtr, infoPtr);
	png_destroy_write_struct(&pngPtr, &infoPtr);

	delete fp;
	logMsg("%s saved.", fname);
	return 1;
}
