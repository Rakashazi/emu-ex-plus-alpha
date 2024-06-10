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
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <optional>
#include <memory>
#include <type_traits>
#include <span>

namespace IG::GL
{
enum class API;
struct Version;
}

namespace IG
{

class GLDisplay;
class GLDrawable;
struct GLContextAttributes;
struct GLBufferConfigAttributes;

using NativeGLDrawable = EGLSurface;
using NativeGLContext = EGLContext;

struct EGLBufferConfig
{
	EGLConfig glConfig{};

	constexpr EGLBufferConfig() = default;
	constexpr EGLBufferConfig(EGLConfig eglConfig):
		glConfig{eglConfig} {}

	constexpr operator EGLConfig() const { return glConfig; }
	constexpr bool operator ==(EGLBufferConfig const&) const = default;
	EGLint renderableTypeBits(GLDisplay display) const;
	bool maySupportGLES(GLDisplay display, int majorVersion) const;
};

class EGLDisplayConnection
{
public:
	constexpr EGLDisplayConnection() = default;
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
	constexpr EGLDrawable() = default;
	EGLDrawable(EGLDisplay, Window &, EGLConfig, const EGLint *surfaceAttr);
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
	constexpr EGLContextBase() = default;
	EGLContextBase(EGLDisplay, GLContextAttributes, EGLConfig, EGLContext shareContext, bool savePBuffConfig);
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
	static constexpr bool hasSwapInterval{true};

	constexpr EGLManager() = default;
	static const char *errorString(EGLint error);
	static int makeRenderableType(GL::API, GL::Version version);
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
	using PresentationTimeFunc = EGLBoolean (EGLAPIENTRY *)(EGLDisplay dpy, EGLSurface surface, EGLnsecsANDROID time);

	UniqueEGLDisplay dpy{};
	bool supportsSurfaceless{};
	bool supportsNoConfig{};
	bool supportsNoError{};
	bool supportsSrgbColorSpace{};
	ConditionalMember<Config::envIsLinux, bool> supportsTripleBufferSurfaces{};
	ConditionalMember<Config::envIsAndroid, PresentationTimeFunc> presentationTime{};

	bool initDisplay(EGLDisplay);
	static std::optional<EGLConfig> chooseConfig(GLDisplay, int renderableType, GLBufferConfigAttributes, bool allowFallback = true);
	static int chooseConfigs(GLDisplay, int renderableType, GLBufferConfigAttributes, std::span<EGLConfig>);
	void logFeatures() const;
	static void terminateEGL(EGLDisplay);
};

}
