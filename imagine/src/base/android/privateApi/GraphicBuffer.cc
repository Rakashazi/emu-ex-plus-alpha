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

#define LOGTAG "GraphicBuff"
#include "../android.hh"
#include <imagine/base/android/GraphicBuffer.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/pixmap/PixmapDesc.hh>
#include <imagine/logger/logger.h>

namespace IG
{

static gralloc_module_t const *grallocMod{};
static alloc_device_t *allocDev{};
static bool testPassed_ = false;

static void initAllocDev()
{
	if(allocDev)
		return;
	if(!libhardware_dl())
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
	if(!allocDev->alloc || !allocDev->free)
	{
		logErr("Missing alloc/free functions");
		if(allocDev->common.close)
			gralloc_close(allocDev);
		else
			logWarn("Missing device close function");
		allocDev = {};
		return;
	}
	logMsg("alloc device:%p", allocDev);
}

GraphicBuffer::GraphicBuffer()
{
	initAllocDev();
	common.incRef =
		[](struct android_native_base_t*)
		{
			//logMsg("called incRef:%p", ptr);
		};
	common.decRef =
		[](struct android_native_base_t*)
		{
			//logMsg("called decRef:%p", ptr);
		};
}

GraphicBuffer::GraphicBuffer(PixmapDesc desc, uint32_t usage):
	GraphicBuffer(desc.w(), desc.h(), toAHardwareBufferFormat(desc.format), usage)
{}

GraphicBuffer::GraphicBuffer(uint32_t w, uint32_t h, uint32_t f, uint32_t reqUsage):
	GraphicBuffer()
{
	if(!hasBufferMapper()) [[unlikely]]
		return;
	if(auto err = allocDev->alloc(allocDev, w, h, f, reqUsage, &handle, &stride);
		err)
	{
		logErr("alloc buffer failed: %s", strerror(-err));
		return;
	}
	width = w;
	height = h;
	format = f;
	usage = reqUsage;
}

GraphicBuffer::GraphicBuffer(GraphicBuffer &&o) noexcept
{
	*this = std::move(o);
}

GraphicBuffer &GraphicBuffer::operator=(GraphicBuffer &&o) noexcept
{
	deinit();
	android_native_buffer_t::operator=(o);
	handle = std::exchange(o.handle, {});
	return *this;
}

GraphicBuffer::~GraphicBuffer()
{
	deinit();
}

void GraphicBuffer::deinit()
{
	if(handle)
	{
		//logMsg("free handle:%p", handle);
		allocDev->free(allocDev, handle);
		handle = {};
	}
}

bool GraphicBuffer::lock(uint32_t usage, void **vaddr)
{
	return lock(usage, {{}, {width, height}}, vaddr);
}

bool GraphicBuffer::lock(uint32_t usage, IG::WindowRect rect, void **vaddr)
{
	if(Config::DEBUG_BUILD)
	{
		if((rect.x < 0 || rect.x2 > (int)width ||
			rect.y < 0 || rect.y2 > (int)height)) [[unlikely]]
		{
			bug_unreachable("locking pixels:[%d:%d:%d:%d] outside of buffer:%d,%d",
				rect.x, rect.y, rect.x2, rect.y2, width, height);
		}
	}
	auto err = grallocMod->lock(grallocMod, handle, usage, rect.x, rect.y, rect.xSize(), rect.ySize(), vaddr);
	return !err;
}

void GraphicBuffer::unlock()
{
	grallocMod->unlock(grallocMod, handle);
}

GraphicBuffer::operator bool() const
{
	return (bool)handle;
}

uint32_t GraphicBuffer::pitch()
{
	return stride;
}

android_native_buffer_t *GraphicBuffer::nativeObject()
{
	return static_cast<android_native_buffer_t*>(this);
}

EGLClientBuffer GraphicBuffer::eglClientBuffer()
{
	return (EGLClientBuffer)nativeObject();
}

bool GraphicBuffer::hasBufferMapper()
{
	return allocDev;
}

bool GraphicBuffer::canSupport(ApplicationContext ctx, std::string_view rendererStr)
{
	auto androidSDK = ctx.androidSDK();
	if(androidSDK >= 24)
	{
		// non-NDK library loading is blocked by the OS
		return false;
	}
	else if(androidSDK >= 11)
	{
		// known tested devices with Android 3.0+
		auto buildDevice = ctx.androidBuildDevice();
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(androidSDK >= 20 && buildDevice == "mako")
			{
				// only Adreno 320 drivers on the Nexus 4 (mako) are confirmed to work,
				// other devices like the HTC One M7 will crash using GraphicBuffers
				return true;
			}
			if(androidSDK >= 19 && buildDevice == "ha3g")
			{
				// works on Galaxy Note 3 (SM-N900) with Mali-T628
				// but not on all devices with this GPU
				return true;
			}
		}
		else if(Config::MACHINE_IS_GENERIC_X86)
		{
			if(androidSDK >= 19 && buildDevice == "ducati2fhd")
			{
				// Works on Acer Iconia Tab 8 (A1-840FHD)
				return true;
			}
		}
	}
	else
	{
		// general rules for Android 2.3 devices
		if(Config::MACHINE_IS_GENERIC_ARMV7)
		{
			if(rendererStr == "PowerVR SGX 530")
				return true;
			if(rendererStr == "PowerVR SGX 540")
				return true;
			if(rendererStr == "Mali-400 MP")
				return true;
		}
	}
	return false;
}

bool GraphicBuffer::testSupport()
{
	// test API functions work

	static constexpr uint32_t allocateUsage = GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE;
	GraphicBuffer gb{256, 256, HAL_PIXEL_FORMAT_RGB_565, allocateUsage};
	if(!gb.hasBufferMapper())
	{
		logErr("failed GraphicBuffer mapper initialization");
		return false;
	}
	if(!gb)
	{
		logErr("failed GraphicBuffer allocation test");
		return false;
	}
	static constexpr uint32_t lockUsage = GRALLOC_USAGE_SW_WRITE_OFTEN;
	void *addr;
	if(!gb.lock(lockUsage, &addr))
	{
		logErr("failed GraphicBuffer lock test");
		return false;
	}
	gb.unlock();
	testPassed_ = true;
	logMsg("Android GraphicBuffer test passed");
	return true;
}

bool GraphicBuffer::isSupported()
{
	return testPassed_;
}

}
