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

#define thisModuleName "res:font"
#include <engine-globals.h>
#include <gfx/Gfx.hh>

#include "ResourceFont.h"

CallResult ResourceFont::initWithName(const char *name)
{
	/*if(Resource::initWithName(name) != OK)
	{
		return OUT_OF_MEMORY;
	}*/
	return OK;
}

//ResourceImageGlyph *ResourceFont::createRenderable(int c, ResourceFace *face, GlyphEntry *entry)
//{
//	return ResourceImageGlyph::createWithFace(face, entry);
//}

int ResourceFont::minUsablePixels() const
{
	return 16;
}
