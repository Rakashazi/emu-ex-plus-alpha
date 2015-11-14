#pragma once

#include <imagine/config/defs.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/operators.hh>
#include <imagine/pixmap/PixelFormat.hh>

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XGLContext.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidGLContext.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSGLContext.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaGLContext.hh>
#endif

namespace Base
{

class GLBufferConfigAttributes
{
private:
	PixelFormat pixelFormat_;
	bool useDepth_ = false;
	bool useStencil_ = false;

public:
	void setPixelFormat(PixelFormat pixelFormat_)
	{
		this->pixelFormat_ = pixelFormat_;
	}

	uint pixelFormat() const
	{
		if(!pixelFormat_)
			return Window::defaultPixelFormat();
		return pixelFormat_;
	}

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
};

class GLContextAttributes
{
private:
	uint majorVer = 1;
	uint minorVer = 0;
	bool glesAPI = false;
	bool debug_ = false;

public:
	void setMajorVersion(uint majorVer)
	{
		if(!majorVer)
			majorVer = 1;
		this->majorVer = majorVer;
	}

	uint majorVersion() const
	{
		return majorVer;
	}

	void setMinorVersion(uint minorVer)
	{
		this->minorVer = minorVer;
	}

	uint minorVersion() const
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
};

class GLContext : public GLContextImpl, public NotEquals<GLContext>
{
public:
	enum API {OPENGL_API, OPENGL_ES_API};

	constexpr GLContext() {}
	CallResult init(GLContextAttributes attr, GLBufferConfig config);
	explicit operator bool() const;
	bool operator ==(GLContext const &rhs) const;
	void deinit();
	static GLBufferConfig makeBufferConfig(GLContextAttributes ctxAttr, GLBufferConfigAttributes attr);
	static GLContext current();
	static void *procAddress(const char *funcName);
	static void setCurrent(GLContext context, Window *win);
	static void setDrawable(Window *win);
	static void setDrawable(Window *win, GLContext cachedCurrentContext);
	static void present(Window &win);
	static void present(Window &win, GLContext cachedCurrentContext);
	static bool bindAPI(API api);
};

}
