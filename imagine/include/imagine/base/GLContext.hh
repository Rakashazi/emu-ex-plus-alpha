#pragma once

#include <imagine/engine-globals.h>
#include <imagine/base/Window.hh>

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
	uint colorBits = 0;
	bool useAlpha_ = false;
	bool useDepth_ = false;
	bool useStencil_ = false;

public:
	void setPreferredColorBits(uint colorBits)
	{
		var_selfs(colorBits);
	}

	uint preferredColorBits() const
	{
		return colorBits;
	}

	void setUseAlpha(bool useAlpha_)
	{
		var_selfs(useAlpha_);
	}

	bool useAlpha() const
	{
		return useAlpha_;
	}

	void setUseDepth(bool useDepth_)
	{
		var_selfs(useDepth_);
	}

	bool useDepth() const
	{
		return useDepth_;
	}

	void setUseStencil(bool useStencil_)
	{
		var_selfs(useStencil_);
	}

	bool useStencil() const
	{
		return useStencil_;
	}

	static uint defaultColorBits();
};

class GLContextAttributes
{
private:
	uint majorVer{};
	uint minorVer{};
	bool glesAPI = false;

public:
	void setMajorVersion(uint majorVer)
	{
		var_selfs(majorVer);
	}

	uint majorVersion() const
	{
		return majorVer;
	}

	void setMinorVersion(uint minorVer)
	{
		var_selfs(minorVer);
	}

	uint minorVersion() const
	{
		return minorVer;
	}

	void setOpenGLESAPI(bool glesAPI)
	{
		var_selfs(glesAPI);
	}

	bool openGLESAPI() const
	{
		return glesAPI;
	}
};

class GLContext : public GLContextImpl
{
public:
	constexpr GLContext() {}
	CallResult init(const GLContextAttributes &attr, const GLBufferConfig &config);
	operator bool() const;
	void deinit();
	static GLBufferConfig makeBufferConfig(const GLContextAttributes &ctxAttr, const GLBufferConfigAttributes &attr);
	static void *procAddress(const char *funcName);
	static void setCurrent(GLContext context, Window *win);
	static void setDrawable(Window *win);
	static GLContext current();
	void present(Window &win);
};

}
