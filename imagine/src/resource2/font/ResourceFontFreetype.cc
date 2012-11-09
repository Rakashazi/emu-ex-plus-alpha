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

#include "ResourceFontFreetype.hh"

ResourceFontFreetype *ResourceFontFreetype::loadWithIoWithName(Io* io, const char *name)
{
	if(!io)
		return nullptr;
	//logMsg("loadWithIoWithName");
	auto inst = new ResourceFontFreetype;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}

	//logMsg("fontData_open");
	if(inst->f[0].open(io) != OK)
	{
		logErr("error reading font");
		inst->free();
		return nullptr;
	}

	//logMsg("setting io addr %p", &i_io);

	if(inst->initWithName(name) != OK)
	{
		inst->free();
		return nullptr;
	}

	return inst;
}

ResourceFontFreetype *ResourceFontFreetype::load(Io* io)
{
	//logMsg("loadWithIo");
	return loadWithIoWithName(io, nullptr);
}

ResourceFontFreetype *ResourceFontFreetype::load(const char *name)
{
	/*ResourceFont *r = (ResourceFont*)Resource::findExisting(name);
	if(r != NULL)
		return r;*/

	//if(string_hasDotExtension(name, "ttf"))
	{
		//logMsg("suffix matches TT Font");
		auto io = IoSys::open(name, 0);
		if(!io)
		{
			logMsg("unable to open file");
			return nullptr;
		}
		return ResourceFontFreetype::loadWithIoWithName(io, name);
	}

	return nullptr;
}

CallResult ResourceFontFreetype::loadIntoSlot(Io *io, uint slot)
{
	if(f[slot].isOpen())
		f[slot].close(1);
	if(f[slot].open(io) != OK)
	{
		logErr("error reading font");
		return IO_ERROR;
	}
	return OK;
}

CallResult ResourceFontFreetype::loadIntoSlot(const char *name, uint slot)
{
	auto io = IoSys::open(name, 0);
	if(!io)
	{
		logMsg("unable to open file %s", name);
		return IO_ERROR;
	}
	auto res = loadIntoSlot(io, slot);
	if(res != OK)
	{
		io->close();
		return res;
	}
	return OK;
}

void ResourceFontFreetype::free()
{
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		f[i].close(1);
	}
	delete this;
}

void ResourceFontFreetype::charBitmap(void *&bitmap, int &x, int &y, int &pitch)
{
	f[currCharSlot].accessCharBitmap(bitmap, x, y, pitch);
}

CallResult ResourceFontFreetype::activeChar(int idx, GlyphMetrics &metrics)
{
	//logMsg("active char: %c", idx);
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		auto &font = f[i];
		if(!font.isOpen())
			continue;
		auto res = font.setActiveChar(idx);
		if(res != OK)
		{
			logMsg("glyph 0x%X not found in slot %d", idx, i);
			continue;
		}
		metrics.xSize = font.charBitmapWidth();
		metrics.ySize = font.charBitmapHeight();
		metrics.xOffset = font.getCurrentCharBitmapLeft();
		metrics.yOffset = font.getCurrentCharBitmapTop();
		metrics.xAdvance = font.getCurrentCharBitmapXAdvance();
		currCharSlot = i;
		return OK;
	}
	return NOT_FOUND;
}

/*int ResourceFontFreetype::currentFaceDescender () const //+
{ return f.maxDescender(); }
int ResourceFontFreetype::currentFaceAscender () const //+
{ return f.maxAscender(); }*/

CallResult ResourceFontFreetype::newSize(FontSettings* settings, FontSizeRef &sizeRef)
{
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		if(!f[i].isOpen())
		{
			sizeRef.size[i] = nullptr;
			continue;
		}
		auto res = f[i].newSize(settings->pixelWidth, settings->pixelHeight, &sizeRef.size[i]);
		if(res != OK)
		{
			// TODO: cleanup already allocated sizes
			return res;
		}
	}
	return OK;
}

CallResult ResourceFontFreetype::applySize(FontSizeRef &sizeRef)
{
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		if(!sizeRef.size[i])
			continue;
		auto res = f[i].applySize(sizeRef.size[i]);
		if(res != OK)
		{
			return res;
		}
	}
	return OK;
}

void ResourceFontFreetype::freeSize(FontSizeRef &sizeRef)
{
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		if(sizeRef.size[i])
			f[i].freeSize(sizeRef.size[i]);
	}
}
