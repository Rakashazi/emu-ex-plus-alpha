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

#if defined CONFIG_PACKAGE_X11
#include <imagine/base/x11/XGL.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidGL.hh>
#elif defined CONFIG_OS_IOS
#include <imagine/base/iphone/IOSGL.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaGL.hh>
#endif

#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/concepts.hh>
#include <optional>
#include <type_traits>

namespace IG::GL
{
enum class API {OPENGL, OPENGL_ES};
}

namespace IG
{

class Window;
class GLDisplay;
class ApplicationContext;

constexpr bool useEGLPlatformAPI = Config::envIsLinux && !Config::MACHINE_IS_PANDORA;

class GLBufferConfigAttributes
{
public:
	PixelFormat pixelFormat{};
	bool useAlpha{};
	bool useDepth{};
	bool useStencil{};
	bool translucentWindow{};
};

class GLContextAttributes
{
public:
	int majorVersion{1};
	int minorVersion{};
	bool glesApi{};
	bool debug{};
	bool noError{};

	constexpr GLContextAttributes() = default;

	constexpr GLContextAttributes(int majorVer, int minorVer, GL::API api):
		majorVersion{majorVer},
		minorVersion{minorVer},
		glesApi{api == GL::API::OPENGL_ES} {}
};

enum class GLColorSpace : uint8_t
{
	LINEAR,
	SRGB
};

class GLDrawableAttributes
{
public:
	GLBufferConfig bufferConfig{};
	GLColorSpace colorSpace{};
	int wantedRenderBuffers{};

	constexpr GLDrawableAttributes() = default;
	constexpr GLDrawableAttributes(GLBufferConfig config): bufferConfig{config} {}
};

class GLDisplay : public GLDisplayImpl
{
public:
	using GLDisplayImpl::GLDisplayImpl;

	constexpr GLDisplay() = default;
	constexpr bool operator ==(GLDisplay const&) const = default;
	void resetCurrentContext() const;
};

class GLDrawable : public GLDrawableImpl
{
public:
	using GLDrawableImpl::GLDrawableImpl;

	constexpr GLDrawable() = default;
	bool operator ==(GLDrawable const&) const = default;
	operator NativeGLDrawable() const { return GLDrawableImpl::operator NativeGLDrawable(); }
	GLDisplay display() const;
};

class GLContext : public GLContextImpl
{
public:
	using GLContextImpl::GLContextImpl;

	constexpr GLContext() = default;
	bool operator ==(GLContext const&) const = default;
	operator NativeGLContext() const { return GLContextImpl::operator NativeGLContext(); }
	GLDisplay display() const;
	void setCurrentContext(NativeGLDrawable) const;
	void setCurrentDrawable(NativeGLDrawable) const;
	void present(NativeGLDrawable) const;
	void setSwapInterval(int);
};

class GLManager : public GLManagerImpl
{
public:
	using GLManagerImpl::GLManagerImpl;
	static constexpr bool hasSwapInterval = GLManagerImpl::hasSwapInterval;

	GLManager(NativeDisplayConnection);
	GLManager(NativeDisplayConnection, GL::API);
	GLDisplay display() const;
	GLDisplay getDefaultDisplay(NativeDisplayConnection) const;
	std::optional<GLBufferConfig> makeBufferConfig(ApplicationContext, GLBufferConfigAttributes, GL::API, int majorVersion = 0) const;
	NativeWindowFormat nativeWindowFormat(ApplicationContext, GLBufferConfig) const;
	GLContext makeContext(GLContextAttributes, GLBufferConfig, NativeGLContext shareContext);
	GLContext makeContext(GLContextAttributes, GLBufferConfig);
	static NativeGLContext currentContext();
	void resetCurrentContext() const;
	GLDrawable makeDrawable(Window &, GLDrawableAttributes) const;
	static bool hasCurrentDrawable(NativeGLDrawable);
	static bool hasCurrentDrawable();
	static void *procAddress(const char *funcName);
	static bool bindAPI(GL::API api);
	bool hasBufferConfig(GLBufferConfigAttributes) const;
	bool hasDrawableConfig(GLBufferConfigAttributes, GLColorSpace) const;
	bool hasNoErrorContextAttribute() const;
	bool hasNoConfigContext() const;
	bool hasSrgbColorSpace() const;
	bool hasPresentationTime() const;
	void setPresentationTime(NativeGLDrawable, SteadyClockTimePoint) const;
	void logInfo() const;

	static bool loadSymbol(Pointer auto &symPtr, const char *name)
	{
		symPtr = reinterpret_cast<std::remove_reference_t<decltype(symPtr)>>(procAddress(name));
		return symPtr;
	}
};

}
