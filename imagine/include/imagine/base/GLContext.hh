#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/util/operators.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <system_error>

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XGL.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidGL.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSGL.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaGL.hh>
#endif

namespace Base
{

class Window;

class GLBufferConfigAttributes
{
public:
	void setPixelFormat(IG::PixelFormat pixelFormat_)
	{
		this->pixelFormat_ = pixelFormat_;
	}

	IG::PixelFormat pixelFormat() const;

	void setUseDepth(bool useDepth_)
	{
		this->useDepth_ = useDepth_;
	}

	bool useDepth() const
	{
		return useDepth_;
	}

	void setUseStencil(bool useStencil_)
	{
		this->useStencil_ = useStencil_;
	}

	bool useStencil() const
	{
		return useStencil_;
	}

private:
	IG::PixelFormat pixelFormat_;
	bool useDepth_ = false;
	bool useStencil_ = false;
};

class GLContextAttributes
{
public:
	void setMajorVersion(uint32_t majorVer)
	{
		if(!majorVer)
			majorVer = 1;
		this->majorVer = majorVer;
	}

	uint32_t majorVersion() const
	{
		return majorVer;
	}

	void setMinorVersion(uint32_t minorVer)
	{
		this->minorVer = minorVer;
	}

	uint32_t minorVersion() const
	{
		return minorVer;
	}

	void setOpenGLESAPI(bool glesAPI)
	{
		this->glesAPI = glesAPI;
	}

	bool openGLESAPI() const
	{
		return glesAPI;
	}

	void setDebug(bool debug)
	{
		debug_ = debug;
	}

	bool debug() const
	{
		return debug_;
	}

private:
	uint32_t majorVer = 1;
	uint32_t minorVer = 0;
	bool glesAPI = false;
	bool debug_ = false;
};

class GLDrawable : public GLDrawableImpl, public NotEquals<GLDrawable>
{
public:
	using GLDrawableImpl::GLDrawableImpl;

	constexpr GLDrawable() {}
	void freeCaches();
	void restoreCaches();
	explicit operator bool() const;
	bool operator ==(GLDrawable const &rhs) const;
};

class GLDisplay : public GLDisplayImpl, public NotEquals<GLDisplay>
{
public:
	enum class API {OPENGL, OPENGL_ES};

	using GLDisplayImpl::GLDisplayImpl;

	constexpr GLDisplay() {}
	static std::pair<std::error_code, GLDisplay> makeDefault();
	static std::pair<std::error_code, GLDisplay> makeDefault(API api);
	static GLDisplay getDefault();
	static GLDisplay getDefault(API api);
	explicit operator bool() const;
	bool operator ==(GLDisplay const &rhs) const;
	bool deinit();
	std::pair<std::error_code, GLDrawable> makeDrawable(Window &win, GLBufferConfig config) const;
	bool deleteDrawable(GLDrawable &drawable) const;
	void logInfo() const;
	static bool bindAPI(API api);
};

class GLContext : public GLContextImpl, public NotEquals<GLContext>
{
public:
	using GLContextImpl::GLContextImpl;

	constexpr GLContext() {}
	GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, std::error_code &ec);
	GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, std::error_code &ec);
	explicit operator bool() const;
	bool operator ==(GLContext const &rhs) const;
	void deinit(GLDisplay display);
	static GLBufferConfig makeBufferConfig(GLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr);
	static GLContext current(GLDisplay display);
	static bool isCurrentDrawable(GLDisplay display, GLDrawable drawable);
	static void *procAddress(const char *funcName);
	static void setCurrent(GLDisplay display, GLContext context, GLDrawable drawable);
	static void setDrawable(GLDisplay display, GLDrawable drawable);
	static void setDrawable(GLDisplay display, GLDrawable drawable, GLContext cachedCurrentContext);
	static void present(GLDisplay display, GLDrawable drawable);
	static void present(GLDisplay display, GLDrawable drawable, GLContext cachedCurrentContext);
	static bool supportsNoConfig();
	NativeGLContext nativeObject();
};

}
