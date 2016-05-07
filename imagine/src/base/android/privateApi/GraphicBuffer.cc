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

#define LOGTAG "GBuff"
#include "../android.hh"
#include <imagine/logger/logger.h>

namespace Base
{

static gralloc_module_t const *grallocMod{};
static alloc_device_t *allocDev{};

static void initAllocDev()
{
	if(allocDev)
		return;
	if(libhardware_dl() != OK)
	{
		logErr("Incompatible libhardware.so");
		return;
	}
	if(hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (hw_module_t const**)&grallocMod) != 0)
	{
		logErr("Can't load gralloc module");
		return;
	}
	gralloc_open((const hw_module_t*)grallocMod, &allocDev);
	if(!allocDev)
	{
		logErr("Can't load allocator device");
		return;
	}
	assert(allocDev->free);
	logMsg("alloc device:%p", allocDev);
}

GraphicBuffer::GraphicBuffer()
{
	initAllocDev();
	common.incRef = [](struct android_native_base_t *){ logMsg("called incRef"); };
	common.decRef = [](struct android_native_base_t *){ logMsg("called decRef"); };
}

GraphicBuffer::~GraphicBuffer()
{
	if(handle)
	{
		allocDev->free(allocDev, handle);
	}
}

bool GraphicBuffer::reallocate(uint w, uint h, uint f, uint reqUsage)
{
	if(handle && w == (uint)width && h == (uint)height && f == (uint)format && reqUsage == (uint)usage)
		return true;
	if(handle)
	{
		allocDev->free(allocDev, handle);
		handle = nullptr;
	}
	return initSize(w, h, f, reqUsage);
}

bool GraphicBuffer::initSize(uint w, uint h, uint f, uint reqUsage)
{
	auto err = allocDev->alloc(allocDev, w, h, f, reqUsage, &handle, &stride);
	if(!err)
	{
		width = w;
		height = h;
		format = f;
		usage = reqUsage;
		return true;
	}
	logErr("alloc buffer failed: %s", strerror(-err));
	return false;
}

bool GraphicBuffer::lock(uint usage, void **vaddr)
{
	return lock(usage, {0, 0, width, height}, vaddr);
}

bool GraphicBuffer::lock(uint usage, IG::WindowRect rect, void **vaddr)
{
	if(rect.x < 0 || rect.x2 > width ||
		rect.y < 0 || rect.y2 > height)
	{
		logErr("locking pixels:[%d:%d:%d:%d] outside of buffer:%d,%d",
			rect.x, rect.y, rect.x2, rect.y2, width, height);
		return false;
	}
	auto err = grallocMod->lock(grallocMod, handle, usage, rect.x, rect.y, rect.xSize(), rect.ySize(), vaddr);
	return !err;
}

void GraphicBuffer::unlock()
{
	grallocMod->unlock(grallocMod, handle);
}

uint GraphicBuffer::getWidth()
{
	return width;
}

uint GraphicBuffer::getHeight()
{
	return height;
}

uint GraphicBuffer::getStride()
{
	return stride;
}

android_native_buffer_t *GraphicBuffer::getNativeBuffer()
{
	return static_cast<android_native_buffer_t*>(this);
}

bool GraphicBuffer::hasBufferMapper()
{
	return grallocMod;
}

}
