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

#ifndef EGL_NO_X11
#define EGL_NO_X11
#endif

#include <imagine/config/defs.hh>
#include <imagine/base/WindowConfig.hh>
#include <imagine/base/Error.hh>
#include <imagine/base/glDefs.hh>
#include <EGL/egl.h>
#include <optional>
#include <memory>
#include <type_traits>
#include <compare>

namespace Base
{

class GLDisplay;
class GLDrawable;
class GLContextAttributes;
class GLBufferConfigAttributes;

using NativeGLDrawable = EGLSurface;
using NativeGLContext = EGLContext;

struct EGLBufferConfig
{
	EGLConfig glConfig{};

	constexpr EGLBufferConfig() {}
	constexpr EGLBufferConfig(EGLConfig eglConfig):
		glConfig{eglConfig} {}

	constexpr operator EGLConfig() const { return glConfig; }
	constexpr bool operator ==(EGLBufferConfig const&) const = default;
	EGLint renderableTypeBits(GLDisplay display) const;
	bool maySupportGLES(GLDisplay display, unsigned majorVersion) const;
};

class EGLDisplayConnection
{
public:
	constexpr EGLDisplayConnection() {}
	constexpr EGLDisplayConnection(EGLDisplay display): display{display} {}
	constexpr operator EGLDisplay() const { return display; }
	constexpr bool operator ==(EGLDisplayConnection const&) const = default;
	explicit constexpr operator bool() const { return display != EGL_NO_DISPLAY; }
	const char *queryExtensions();

protected:
	EGLDisplay display{EGL_NO_DISPLAY};
};

class EGLDrawable
{
public:
	constexpr EGLDrawable() {}
	EGLDrawable(EGLDisplay, Window &, EGLConfig, const EGLint *surfaceAttr, IG::ErrorCode &);
	operator EGLSurface() const { return surface.get(); }
	bool operator ==(EGLDrawable const&) const = default;
	explicit operator bool() const { return (bool)surface; }

protected:
	struct EGLSurfaceDeleter
	{
		EGLDisplay dpy;

		void operator()(EGLSurface ptr) const
		{
			destroySurface(dpy, ptr);
		}
		constexpr bool operator ==(EGLSurfaceDeleter const&) const = default;
	};
	using UniqueEGLSurface = std::unique_ptr<std::remove_pointer_t<EGLSurface>, EGLSurfaceDeleter>;

	UniqueEGLSurface surface{};

	static void destroySurface(EGLDisplay, EGLSurface);
};

class EGLContextBase
{
public:
	constexpr EGLContextBase() {}
	EGLContextBase(EGLDisplay, GLContextAttributes, EGLConfig, EGLContext shareContext, bool savePBuffConfig, IG::ErrorCode &);
	operator EGLContext() const { return context.get(); }
	bool operator ==(EGLContextBase const&) const = default;
	explicit operator bool() const { return (bool)context; }

protected:
	struct EGLContextDeleter
	{
		EGLDisplay dpy;

		void operator()(EGLContext ptr) const
		{
			destroyContext(dpy, ptr);
		}
		constexpr bool operator ==(EGLContextDeleter const&) const = default;
	};
	using UniqueEGLContext = std::unique_ptr<std::remove_pointer_t<EGLContext>, EGLContextDeleter>;

	UniqueEGLContext context{};
	std::optional<EGLConfig> dummyPbuffConfig{};

	static void destroyContext(EGLDisplay, EGLContext);
};

class EGLManager
{
public:
	constexpr EGLManager() {}
	static const char *errorString(EGLint error);
	static int makeRenderableType(GL::API, unsigned majorVersion);
	explicit operator bool() const { return (bool)dpy; }

protected:
	struct EGLDisplayDeleter
	{
		void operator()(EGLDisplay ptr) const
		{
			terminateEGL(ptr);
		}
	};
	using UniqueEGLDisplay = std::unique_ptr<std::remove_pointer_t<EGLDisplay>, EGLDisplayDeleter>;

	UniqueEGLDisplay dpy{};
	bool supportsSurfaceless{};
	bool supportsNoConfig{};
	bool supportsNoError{};
	bool supportsSrgbColorSpace{};
	IG_enableMemberIf(Config::envIsLinux, bool, supportsTripleBufferSurfaces){};

	IG::ErrorCode initDisplay(EGLDisplay);
	static std::optional<EGLConfig> chooseConfig(GLDisplay, int renderableType, GLBufferConfigAttributes, bool allowFallback = true);
	void logFeatures() const;
	static void terminateEGL(EGLDisplay);
};

}
