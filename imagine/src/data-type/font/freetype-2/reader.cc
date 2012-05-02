#define thisModuleName "freetype2"

#include "reader.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <logger/interface.h>
#include <base/Base.hh>
#include <mem/interface.h>
#include <util/pixel.h>

#include FT_SIZES_H

static unsigned long freetype_IoFunc(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
	Io *io = (Io*)stream->descriptor.pointer;
	//logMsg("using io addr %p", io);
	io->seekAbs(offset);
	stream->pos = offset;
	//logMsg("offset %d", (int)offset);
	
	if(count == 0) return(0);
	
	size_t bytesRead = io->readUpTo(buffer, count);
	//logMsg("read %d out of %d req bytes", (int)bytesRead, (int)count);
	return bytesRead;
}

//void (*FT_Stream_CloseFunc)( FT_Stream  stream );

CallResult FontData::open(Io *file)
{
	library = NULL;

	int error = FT_Init_FreeType( &library );
	if ( error )
	{
		logErr("error in FT_Init_FreeType");
		return INVALID_PARAMETER;
	}
	
	streamRec.base = NULL;
	streamRec.size = file->size();
	ulong fileOffset;
	file->tell(&fileOffset);
	streamRec.pos = fileOffset;
	streamRec.descriptor.pointer = file;
	streamRec.read = &freetype_IoFunc;
	streamRec.close = NULL;
	
	openS.flags = FT_OPEN_STREAM;
	openS.stream = &streamRec;

	error = FT_Open_Face( library, &openS, 0, &face );

	if ( error == FT_Err_Unknown_File_Format )
	{
		logErr("cannot use font, the format is unknown");
		FT_Done_FreeType(library);
		return INVALID_PARAMETER;
	}
	else if ( error )
	{
		logErr("error occurred opening the font");
		FT_Done_FreeType(library);
		return IO_ERROR;
	}
	
	/*error = FT_Set_Char_Size( data->face, 0, 16*64, 72, 72 );
	if ( error )
	{
		logger_printf(LOGGER_ERROR, "FreeType 2:reader: error occurred setting character size");
		return(INVALID_PARAMETER);
	}*/

	return(OK);
}

CallResult FontData::setSizes(int x, int y)
{
	int error = FT_Set_Pixel_Sizes( face, x, y );
	if ( error )
	{
		logErr("error occurred setting character pixel size");
		return(INVALID_PARAMETER);
	}
	
	//logMsg("Face max bounds %dx%d,%dx%d, units per EM %d", face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax, face->units_per_EM);
	return OK;
}

CallResult FontData::newSize(int x, int y, FT_Size *sizeDataAddr)
{
	logMsg("creating new size object, %dx%d pixels", x, y);
	
	if(FT_New_Size(face, sizeDataAddr) != 0)
	{
		logErr("error creating new size object");
		return INVALID_PARAMETER;
	}
	//logMsg("done");
	FT_Size size = *sizeDataAddr;
	
	//logMsg("activating size");
	if(FT_Activate_Size(size) != 0)
	{
		logErr("error activating size object");
		return INVALID_PARAMETER;
	}
	//logMsg("done");
	
	doOrReturn(setSizes(x, y));
	
	//logMsg("scaled ascender x descender %dx%d", (int)size->metrics.ascender >> 6, (int)size->metrics.descender >> 6);
	return OK;
}

CallResult FontData::applySize(FT_Size sizeData)
{
	//logMsg("applying size");
	if(FT_Activate_Size(sizeData) != 0)
	{
		logErr("error activating size object");
		return INVALID_PARAMETER;
	}
	else return OK;
}

void FontData::freeSize(FT_Size sizeData)
{
	logDMsg("freeing size object");
	var_copy(result, FT_Done_Size(sizeData));
	assert(result == 0);
}

int FontData::maxDescender() const
{
	return face->size->metrics.descender >> 6;
}

int FontData::maxAscender() const
{
	return face->size->metrics.ascender >> 6;
}

void FontData::close()
{
	logMsg("freeing %p", library);
	assert(library != NULL);
	FT_Done_FreeType(library);
	//FT_Done_Face(data->face);
}

CallResult FontData::setActiveChar(uchar c)
{
	//logMsg("setting active char");
	int error = FT_Load_Char( face, c, FT_LOAD_RENDER );
	if( error )
	{
		logErr("error occurred loading/rendering character %c", c);
		return(INVALID_PARAMETER);
	}
	return OK;
}

int FontData::charBitmapWidth() const
{
	return face->glyph->bitmap.width;
}

int FontData::charBitmapHeight() const
{
	return face->glyph->bitmap.rows;
}

void FontData::accessCharBitmap(uchar **bitmap, int *x, int *y, int *pitch) const
{
	FT_GlyphSlot slot = face->glyph;

	//logMsg("character is %dx%d pixels, left-top offsets %dx%d", slot->bitmap.width, slot->bitmap.rows, slot->bitmap_left, slot->bitmap_top);
	*bitmap = slot->bitmap.buffer;
	*x = slot->bitmap.width;
	*y = slot->bitmap.rows;
	*pitch = slot->bitmap.pitch;


}

/*void FontData::accessCurrentCharBitmapOffsets(int *left, int *top)
{
	FT_GlyphSlot slot = face->glyph;
	*left = slot->bitmap_left;
	*top = slot->bitmap_top;
}*/

int FontData::getCurrentCharBitmapXAdvance() const
{
	FT_GlyphSlot slot = face->glyph;
	return(slot->advance.x >> 6);
}

int FontData::getCurrentCharBitmapTop() const
{
	FT_GlyphSlot slot = face->glyph;
	//return(slot->bitmap_top - ((slot->metrics.height - slot->metrics.horiBearingY) / 64) );
	return(slot->bitmap_top);
}

int FontData::getCurrentCharBitmapLeft() const
{
	FT_GlyphSlot slot = face->glyph;
	return(slot->bitmap_left);
}

#undef thisModuleName
