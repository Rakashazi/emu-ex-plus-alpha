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

#define thisModuleName "res:font:freetype"
#include <engine-globals.h>
#include <gfx/Gfx.hh>
#include <util/strings.h>

#include "ResourceFontFreetype.h"

ResourceFont *ResourceFontFreetype::loadWithIoWithName (Io* io, const char *name)
{
	if(!io)
		return 0;
	//logMsg("loadWithIoWithName");
	ResourceFontFreetype *inst = new ResourceFontFreetype;
	if(inst == NULL)
	{
		logErr("out of memory");
		return NULL;
	}

	inst->io = io;

	//logMsg("fontData_open");
	if(inst->f.open(inst->io) != OK)
	{
		logErr("error reading font");
		inst->free();
		return NULL;
	}

	//logMsg("setting io addr %p", &i_io);

	if(inst->initWithName(name) != OK)
	{
		inst->free();
		return NULL;
	}

	return inst;
}

ResourceFont *ResourceFontFreetype::load(Io* io)
{
	//logMsg("loadWithIo");
	return loadWithIoWithName(io, NULL);
}

ResourceFont *ResourceFontFreetype::load(const char *name)
{
	/*ResourceFont *r = (ResourceFont*)Resource::findExisting(name);
	if(r != NULL)
		return r;*/

	if(string_hasDotExtension(name, "ttf"))
	{
		logMsg("suffix matches TT Font");
		Io* io = IoSys::open(name, 0);
		if(io == NULL)
		{
			logMsg("unable to open file");
			return NULL;
		}
		return ResourceFontFreetype::loadWithIoWithName(io, name);
	}

	return NULL;
}

void ResourceFontFreetype::free () //+
{
	f.close();
	io->close();
	delete this;
}

int ResourceFontFreetype::currentCharYOffset () const //+
{
	return f.getCurrentCharBitmapTop();
}

int ResourceFontFreetype::currentCharXOffset () const //+
{
	return f.getCurrentCharBitmapLeft();
}

int ResourceFontFreetype::currentCharXAdvance () const //+
{
	return f.getCurrentCharBitmapXAdvance();
}

int ResourceFontFreetype::minUsablePixels() const
{
	return 16;
}

void ResourceFontFreetype::charBitmap (uchar** bitmap, int* x, int* y, int* pitch) const //+
{
	f.accessCharBitmap(bitmap, x, y, pitch);
}

CallResult ResourceFontFreetype::activeChar (int idx) //+
{
	//logMsg("active char: %c", idx);
	return f.setActiveChar(idx);
}

int ResourceFontFreetype::currentCharXSize () const //+
{ return f.charBitmapWidth(); }
int ResourceFontFreetype::currentCharYSize () const //+
{ return f.charBitmapHeight(); }
int ResourceFontFreetype::currentFaceDescender () const //+
{ return f.maxDescender(); }
int ResourceFontFreetype::currentFaceAscender () const //+
{ return f.maxAscender(); }

ResourceImageGlyph *ResourceFontFreetype::createRenderable (int c, ResourceFace *face, GlyphEntry *entry) //+
{
	return ResourceImageGlyph::createWithFace(face, entry);
}

CallResult ResourceFontFreetype::newSize (FontSettings* settings, FaceSizeData* sizeDataAddr) //+
{
	return f.newSize(settings->pixelWidth, settings->pixelHeight, sizeDataAddr);
}
CallResult ResourceFontFreetype::applySize (FaceSizeData sizeData) //+
{
	return f.applySize(sizeData);
}
void ResourceFontFreetype::freeSize (FaceSizeData sizeData) //+
{
	f.freeSize(sizeData);
}

#undef thisModuleName
