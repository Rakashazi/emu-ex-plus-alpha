#define thisModuleName "freetype2"

#include "FreetypeFontData.hh"
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <logger/interface.h>
#include <base/Base.hh>
#include <mem/interface.h>
#include <util/pixel.h>

#include FT_SIZES_H

FT_Library FreetypeFontData::library = nullptr;

static unsigned long freetypeIoFunc(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
	auto *io = (Io*)stream->descriptor.pointer;
	//logMsg("using io addr %p", io);
	io->seekAbs(offset);
	stream->pos = offset;
	//logMsg("offset %d", (int)offset);
	
	if(count == 0) return 0;
	
	size_t bytesRead = io->readUpTo(buffer, count);
	//logMsg("read %d out of %d req bytes", (int)bytesRead, (int)count);
	return bytesRead;
}

//void (*FT_Stream_CloseFunc)( FT_Stream  stream );

CallResult FreetypeFontData::open(Io *file)
{
	if(!library)
	{
		auto error = FT_Init_FreeType(&library);
		if(error)
		{
			logErr("error in FT_Init_FreeType");
			return INVALID_PARAMETER;
		}
	}
	
	//streamRec.base = nullptr;
	streamRec.size = file->size();
	ulong fileOffset;
	file->tell(&fileOffset);
	streamRec.pos = fileOffset;
	streamRec.descriptor.pointer = file;
	streamRec.read = &freetypeIoFunc;
	//streamRec.close = nullptr;
	FT_Open_Args openS {0};
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
	return face != nullptr;
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
	if(face)
	{
		FT_Done_Face(face);
		face = nullptr;
	}
	if(closeIo && streamRec.descriptor.pointer)
	{
		delete ((Io*)streamRec.descriptor.pointer);
		streamRec.descriptor.pointer = nullptr;
	}
}

CallResult FreetypeFontData::setActiveChar(int c)
{
	//logMsg("setting active char");
	auto idx = FT_Get_Char_Index(face, c);
	if(!idx)
		return NOT_FOUND;
	auto error = FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
	if(error)
	{
		logErr("error occurred loading/rendering character 0x%X", c);
		return INVALID_PARAMETER;
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

void FreetypeFontData::accessCharBitmap(void *&bitmap, int &x, int &y, int &pitch) const
{
	auto slot = face->glyph;
	//logMsg("character is %dx%d pixels, left-top offsets %dx%d", slot->bitmap.width, slot->bitmap.rows, slot->bitmap_left, slot->bitmap_top);
	bitmap = slot->bitmap.buffer;
	x = slot->bitmap.width;
	y = slot->bitmap.rows;
	pitch = slot->bitmap.pitch;
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
