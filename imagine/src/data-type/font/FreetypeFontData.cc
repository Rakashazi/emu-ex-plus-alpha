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

#define LOGTAG "Freetype2"
#include <imagine/data-type/font/FreetypeFontData.hh>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/util/algorithm.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_SIZES_H

FT_Library FreetypeFontData::library{};

static unsigned long freetypeIoFunc(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
	auto &io = *((IO*)stream->descriptor.pointer);
	//logMsg("using io addr %p", io);
	stream->pos = offset;
	//logMsg("offset %d", (int)offset);
	
	if(count == 0) return 0;
	
	// TODO: error checking
	size_t bytesRead = io.readAtPos(buffer, count, offset);
	//logMsg("read %d out of %d req bytes", (int)bytesRead, (int)count);
	return bytesRead;
}

//void (*FT_Stream_CloseFunc)( FT_Stream  stream );

CallResult FreetypeFontData::open(GenericIO file)
{
	if(!library)
	{
		auto error = FT_Init_FreeType(&library);
		if(error)
		{
			logErr("error in FT_Init_FreeType");
			return INVALID_PARAMETER;
		}
		#ifndef NDEBUG
		/*FT_Int major, minor, patch;
		FT_Library_Version(library, &major, &minor, &patch);
		logMsg("init freetype version %d.%d.%d", (int)major, (int)minor, (int)patch);*/
		#endif
	}
	
	//streamRec.base = nullptr;
	streamRec.size = file.size();
	auto fileOffset =	file.tell();
	streamRec.pos = fileOffset;
	streamRec.descriptor.pointer = file.release();
	streamRec.read = &freetypeIoFunc;
	//streamRec.close = nullptr;
	FT_Open_Args openS{};
	openS.flags = FT_OPEN_STREAM;
	openS.stream = &streamRec;

	auto error = FT_Open_Face(library, &openS, 0, &face);

	if(error == FT_Err_Unknown_File_Format)
	{
		logErr("unknown font format");
		//FT_Done_FreeType(library);
		return INVALID_PARAMETER;
	}
	else if(error)
	{
		logErr("error occurred opening the font");
		//FT_Done_FreeType(library);
		return IO_ERROR;
	}
	
	/*error = FT_Set_Char_Size( data->face, 0, 16*64, 72, 72 );
	if ( error )
	{
		logger_printf(LOGGER_ERROR, "FreeType 2:reader: error occurred setting character size");
		return(INVALID_PARAMETER);
	}*/

	return OK;
}

bool FreetypeFontData::isOpen()
{
	return face;
}

CallResult FreetypeFontData::setSizes(int x, int y)
{
	auto error = FT_Set_Pixel_Sizes( face, x, y );
	if(error)
	{
		logErr("error occurred setting character pixel size");
		return INVALID_PARAMETER;
	}
	
	//logMsg("Face max bounds %dx%d,%dx%d, units per EM %d", face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax, face->units_per_EM);
	return OK;
}

CallResult FreetypeFontData::newSize(int x, int y, FT_Size *sizeRef)
{
	logMsg("creating new size object, %dx%d pixels", x, y);
	
	if(FT_New_Size(face, sizeRef) != 0)
	{
		logErr("error creating new size object");
		return INVALID_PARAMETER;
	}
	//logMsg("done");
	
	//logMsg("activating size");
	if(FT_Activate_Size(*sizeRef) != 0)
	{
		logErr("error activating size object");
		return INVALID_PARAMETER;
	}
	//logMsg("done");
	
	doOrReturn(setSizes(x, y));
	
	//logMsg("scaled ascender x descender %dx%d", (int)size->metrics.ascender >> 6, (int)size->metrics.descender >> 6);
	return OK;
}

CallResult FreetypeFontData::applySize(FT_Size sizeRef)
{
	//logMsg("applying size");
	if(FT_Activate_Size(sizeRef) != 0)
	{
		logErr("error activating size object");
		return INVALID_PARAMETER;
	}
	else return OK;
}

void FreetypeFontData::freeSize(FT_Size sizeRef)
{
	logDMsg("freeing size object");
	auto result = FT_Done_Size(sizeRef);
	assert(result == 0);
}

int FreetypeFontData::maxDescender() const
{
	return face->size->metrics.descender >> 6;
}

int FreetypeFontData::maxAscender() const
{
	return face->size->metrics.ascender >> 6;
}

void FreetypeFontData::close(bool closeIo)
{
	/*logMsg("freeing %p", library);
	assert(library != NULL);
	FT_Done_FreeType(library);*/
	if(convBitmap.buffer)
	{
		FT_Bitmap_Done(library, &convBitmap);
		convBitmap.buffer = nullptr;
	}
	if(face)
	{
		FT_Done_Face(face);
		face = nullptr;
	}
	if(closeIo && streamRec.descriptor.pointer)
	{
		delete (IO*)streamRec.descriptor.pointer;
		streamRec.descriptor.pointer = nullptr;
	}
}

CallResult FreetypeFontData::setActiveChar(int c)
{
	//logMsg("setting active char");
	if(convBitmap.buffer) // if there was a previous converted bitmap, free it
	{
		FT_Bitmap_Done(library, &convBitmap);
		convBitmap.buffer = nullptr;
	}
	auto idx = FT_Get_Char_Index(face, c);
	if(!idx)
		return NOT_FOUND;
	auto error = FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
	if(error)
	{
		logErr("error occurred loading/rendering character 0x%X", c);
		return INVALID_PARAMETER;
	}
	if(face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// rendered glyph is not in 8-bit gray-scale
		logMsg("converting mode %d bitmap", face->glyph->bitmap.pixel_mode);
		auto error = FT_Bitmap_Convert(library, &face->glyph->bitmap, &convBitmap, 1);
		if(error)
		{
			logErr("error occurred converting character 0x%X", c);
			return INVALID_PARAMETER;
		}
		assert(convBitmap.num_grays == 2); // only handle 2 gray levels for now
		//logMsg("new bitmap has %d gray levels", convBitmap.num_grays);
		// scale 1-bit values to 8-bit range
		iterateTimes(convBitmap.rows, y)
		{
			iterateTimes(convBitmap.width, x)
			{
				if(convBitmap.buffer[(y * convBitmap.pitch) + x] != 0)
					convBitmap.buffer[(y * convBitmap.pitch) + x] = 0xFF;
			}
		}
	}
	return OK;
}

int FreetypeFontData::charBitmapWidth() const
{
	return face->glyph->bitmap.width;
}

int FreetypeFontData::charBitmapHeight() const
{
	return face->glyph->bitmap.rows;
}

IG::Pixmap FreetypeFontData::accessCharBitmap() const
{
	auto slot = face->glyph;
	auto &bitmap = convBitmap.buffer ? convBitmap : slot->bitmap;
	//logMsg("character is %dx%d pixels, left-top offsets %dx%d", bitmap.width, bitmap.rows, slot->bitmap_left, slot->bitmap_top);
	IG::Pixmap pix{{{(int)bitmap.width, (int)bitmap.rows}, IG::PIXEL_FMT_A8},
		bitmap.buffer, {(uint)bitmap.pitch, IG::Pixmap::BYTE_UNITS}};
	return pix;
}

int FreetypeFontData::getCurrentCharBitmapXAdvance() const
{
	auto slot = face->glyph;
	return slot->advance.x >> 6;
}

int FreetypeFontData::getCurrentCharBitmapTop() const
{
	auto slot = face->glyph;
	//return(slot->bitmap_top - ((slot->metrics.height - slot->metrics.horiBearingY) / 64) );
	return slot->bitmap_top;
}

int FreetypeFontData::getCurrentCharBitmapLeft() const
{
	auto slot = face->glyph;
	return slot->bitmap_left;
}
