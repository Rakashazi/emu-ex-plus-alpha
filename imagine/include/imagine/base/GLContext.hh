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

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XGL.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidGL.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSGL.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaGL.hh>
#endif

#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/base/Error.hh>
#include <imagine/base/glDefs.hh>
#include <optional>
#include <compare>

namespace Base
{

class Window;
class GLDisplay;
class ApplicationContext;

class GLBufferConfigAttributes
{
public:
	constexpr GLBufferConfigAttributes(IG::PixelFormat pixelFormat = {}):pixelFormat_{pixelFormat} {}

	constexpr void setPixelFormat(IG::PixelFormat pixelFormat_)
	{
		this->pixelFormat_ = pixelFormat_;
	}

	constexpr IG::PixelFormat pixelFormat() const
	{
		return pixelFormat_;
	}

	constexpr void setUseDepth(bool useDepth_)
	{
		this->useDepth_ = useDepth_;
	}

	constexpr bool useDepth() const
	{
		return useDepth_;
	}

	constexpr void setUseStencil(bool useStencil_)
	{
		this->useStencil_ = useStencil_;
	}

	constexpr bool useStencil() const
	{
		return useStencil_;
	}

private:
	IG::PixelFormat pixelFormat_{};
	bool useDepth_{};
	bool useStencil_{};
};

class GLContextAttributes
{
public:
	constexpr GLContextAttributes() {}

	constexpr GLContextAttributes(uint32_t majorVer, uint32_t minorVer, GL::API api)
	{
		setMajorVersion(majorVer);
		setMinorVersion(minorVer);
		setOpenGLESAPI(api == GL::API::OPENGL_ES);
	}

	constexpr void setMajorVersion(uint32_t majorVer)
	{
		if(!majorVer)
			majorVer = 1;
		this->majorVer = majorVer;
	}

	constexpr uint32_t majorVersion() const
	{
		return majorVer;
	}

	constexpr void setMinorVersion(uint32_t minorVer)
	{
		this->minorVer = minorVer;
	}

	constexpr uint32_t minorVersion() const
	{
		return minorVer;
	}

	constexpr void setOpenGLESAPI(bool glesAPI)
	{
		this->glesAPI = glesAPI;
	}

	constexpr bool openGLESAPI() const
	{
		return glesAPI;
	}

	constexpr void setDebug(bool debug)
	{
		debug_ = debug;
	}

	constexpr bool debug() const
	{
		return debug_;
	}

	constexpr void setNoError(bool noError)
	{
		noError_ = noError;
	}

	constexpr bool noError() const
	{
		return noError_;
	}

private:
	uint32_t majorVer{1};
	uint32_t minorVer{};
	bool glesAPI{};
	bool debug_{};
	bool noError_{};
};

enum class GLColorSpace : uint8_t
{
	LINEAR,
	SRGB
};

class GLDrawableAttributes
{
public:
	constexpr GLDrawableAttributes() {}
	constexpr GLDrawableAttributes(GLBufferConfig config):bufferConfig_{config} {}

	constexpr void setColorSpace(GLColorSpace c)
	{
		colorSpace_ = c;
	}

	constexpr GLColorSpace colorSpace() const
	{
		return colorSpace_;
	}

	constexpr GLBufferConfig bufferConfig() const
	{
		return bufferConfig_;
	}

private:
	GLBufferConfig bufferConfig_{};
	GLColorSpace colorSpace_{};
};

class GLDisplay : public GLDisplayImpl
{
public:
	using GLDisplayImpl::GLDisplayImpl;

	constexpr GLDisplay() {}
	constexpr bool operator ==(GLDisplay const&) const = default;
	void resetCurrentContext() const;
};

class GLDrawable : public GLDrawableImpl
{
public:
	using GLDrawableImpl::GLDrawableImpl;

	constexpr GLDrawable() {}
	bool operator ==(GLDrawable const&) const = default;
	operator NativeGLDrawable() const { return GLDrawableImpl::operator NativeGLDrawable(); }
	GLDisplay display() const;
};

class GLContext : public GLContextImpl
{
public:
	using GLContextImpl::GLContextImpl;

	constexpr GLContext() {}
	bool operator ==(GLContext const&) const = default;
	operator NativeGLContext() const { return GLContextImpl::operator NativeGLContext(); }
	GLDisplay display() const;
	void setCurrentContext(NativeGLDrawable) const;
	void setCurrentDrawable(NativeGLDrawable) const;
	void present(NativeGLDrawable) const;
};

class GLManager : public GLManagerImpl
{
public:
	using GLManagerImpl::GLManagerImpl;

	GLManager(NativeDisplayConnection);
	GLManager(NativeDisplayConnection, GL::API);
	GLDisplay display() const;
	GLDisplay getDefaultDisplay(NativeDisplayConnection) const;
	std::optional<GLBufferConfig> makeBufferConfig(Base::ApplicationContext, GLBufferConfigAttributes, GL::API, unsigned majorVersion = 0) const;
	Base::NativeWindowFormat nativeWindowFormat(Base::ApplicationContext, GLBufferConfig) const;
	GLContext makeContext(GLContextAttributes, GLBufferConfig, NativeGLContext shareContext, IG::ErrorCode &);
	GLContext makeContext(GLContextAttributes, GLBufferConfig, IG::ErrorCode &);
	static NativeGLContext currentContext();
	void resetCurrentContext() const;
	GLDrawable makeDrawable(Window &, GLDrawableAttributes, IG::ErrorCode &) const;
	static bool hasCurrentDrawable(NativeGLDrawable);
	static bool hasCurrentDrawable();
	static void *procAddress(const char *funcName);
	static bool bindAPI(GL::API api);
	bool hasBufferConfig(GLBufferConfigAttributes) const;
	bool hasDrawableConfig(GLBufferConfigAttributes, GLColorSpace) const;
	bool hasNoErrorContextAttribute() const;
	bool hasNoConfigContext() const;
	bool hasSrgbColorSpace() const;
	void logInfo() const;

	template<class T>
	static bool loadSymbol(T &symPtr, const char *name)
	{
		static_assert(std::is_pointer_v<T>, "called loadSymbol() without pointer type");
		symPtr = (T)procAddress(name);
		return symPtr;
	}
};

}
