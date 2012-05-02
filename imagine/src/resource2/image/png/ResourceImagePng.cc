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

#define thisModuleName "res:img:png"
#include <engine-globals.h>
#include <gfx/Gfx.hh>
#include <util/strings.h>

#include "ResourceImagePng.h"

ResourceImage * ResourceImagePng::loadWithIoWithName (Io* io, const char *name)
{
	if(!io)
		return 0;
	Png png;
	if(png.readHeader(io) != OK)
	{
		logErr("error reading header");
		return NULL;
	}

	ResourceImagePng *inst = new ResourceImagePng;
	if(inst == NULL)
	{
		logErr("out of memory");
		png.freeImageData();
		return NULL;
	}

	inst->io = io;
	inst->png = png;
	inst->format = png.pixelFormat();

	/*if(inst->initWithName(name) != OK)
	{
		inst->free();
		return NULL;
	}*/

	inst->gfxD.init(*inst);
	inst->ResourceImage::init();

	return inst;
}

ResourceImage * ResourceImagePng::load (Io* io)
{
	return  ResourceImagePng::loadWithIoWithName(io, NULL);
}

ResourceImage * ResourceImagePng::load (const char *name)
{
	/*ResourceImage *r = (ResourceImage*)Resource::findExisting(name);
	if(r != NULL)
		return r;*/
	
	if(string_hasDotExtension(name, "png"))
	{
		//logMsg("suffix matches PNG image");
		Io *io = IoSys::open(name, 0);
		if(io == NULL)
		{
			logMsg("unable to open file");
			return NULL;
		}
		return  ResourceImagePng::loadWithIoWithName(io, name);
	}

	return NULL;
}

void  ResourceImagePng::free ()
{
	gfxD.removeBacker();
	gfxD.deinit();
	png.freeImageData();
	io->close();
	delete this;
}

CallResult  ResourceImagePng::getImage (Pixmap* dest)
{
	return(png.readImage(io, dest->data, dest->pitch, *dest->format));
}

int  ResourceImagePng::alphaChannelType ()
{
	return png.hasAlphaChannel() ? IMAGE_ALPHA_CHANNEL_USED : IMAGE_ALPHA_CHANNEL_NONE;
}

uint  ResourceImagePng::width ()
{
	return png.getWidth();
}

uint  ResourceImagePng::height ()
{
	return png.getHeight();
}

const PixelFormatDesc *ResourceImagePng::pixelFormat () const
{
	return format;
}

float  ResourceImagePng::aspectRatio ()
{
	return (float)png.getWidth() / (float)png.getHeight();
}

#undef thisModuleName
