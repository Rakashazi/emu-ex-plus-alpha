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

#define thisModuleName "res:img:glyph"
#include <engine-globals.h>
#include <gfx/Gfx.hh>
#include <resource2/face/ResourceFace.hh>

#include "ResourceImageGlyph.h"

ResourceImageGlyph *ResourceImageGlyph::createWithFace(ResourceFace *face, GlyphEntry *idx)
{
	//logMsg("creating glyph");
	ResourceImageGlyph *inst = new ResourceImageGlyph;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}

	inst->face = face;
	inst->idx = idx;

	inst->gfxD.init(*inst, Gfx::BufferImage::linear, Gfx::BufferImage::HINT_NO_MINIFY);
	inst->ResourceImage::init();

	return inst;
}

void ResourceImageGlyph::free()
{
	//logMsg("freeing");
	gfxD.removeBacker();
	gfxD.deinit();
	delete this;
}

CallResult ResourceImageGlyph::getImage(Pixmap* dest)
{
	return face->writeCurrentChar(dest);
}

int ResourceImageGlyph::alphaChannelType()
{
	return IMAGE_ALPHA_CHANNEL_IS_INTENSITY;
}

uint ResourceImageGlyph::width()
{
	return idx->metrics.xSize;
}

uint ResourceImageGlyph::height()
{
	return idx->metrics.ySize;
}

const PixelFormatDesc *ResourceImageGlyph::pixelFormat() const
{
	return &PixelFormatI8;
}

float ResourceImageGlyph::aspectRatio()
{
	return (float)idx->metrics.xSize / (float)idx->metrics.ySize;
}
