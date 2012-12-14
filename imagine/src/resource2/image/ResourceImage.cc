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

#define thisModuleName "res:img"
#include <engine-globals.h>

#ifdef CONFIG_GFX
#include <gfx/Gfx.hh>
#endif

#ifdef CONFIG_RESOURCE_IMAGE_PNG
	#include <resource2/image/png/ResourceImagePng.h>
#endif
#ifdef CONFIG_RESOURCE_IMAGE_JPEG
	#include <resource2/image/jpeg/ResourceImageJpeg.hh>
#endif

#include "ResourceImage.h"

void ResourceImage::init ()
{
	ref();
	//Resource::init();
}

#if 0
CallResult ResourceImage::init (const char *name)
{
	/*if(Resource::initWithName(name) != OK)
	{
		return OUT_OF_MEMORY;
	}*/
	return OK;
}
#endif

ResourceImage *ResourceImage::load (const char *name)
{
	ResourceImage *r = NULL;
	#ifdef CONFIG_RESOURCE_IMAGE_PNG
		if((r = ResourceImagePng::load(name)) != NULL)
				return r;
	#endif
	#ifdef CONFIG_RESOURCE_IMAGE_JPEG
		if((r = ResourceImageJpeg::load(name)) != NULL)
				return r;
	#endif
	logMsg("failed to load image %s", name);
	return NULL;
}
