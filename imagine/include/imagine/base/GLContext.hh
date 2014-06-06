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

class Window;

class GLConfigAttributes
{
private:
	uint majorVer = 0;
	uint minorVer = 0;
	uint colorBits = 0;
	bool useAlpha_ = false;
	bool useDepth_ = false;
	bool useStencil_ = false;
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

	void setOpenGLESAPI(bool glesAPI)
	{
		var_selfs(glesAPI);
	}

	bool openGLESAPI() const
	{
		return glesAPI;
	}

	static uint defaultColorBits();
};

class GLContext : public GLContextImpl
{
public:
	constexpr GLContext() {}
	CallResult init(const GLConfigAttributes &attr);
	void *procAddress(const char *funcName);
	GLConfig bufferConfig();
	static bool setCurrent(GLContext *context, Window *win);
	static bool setDrawable(Window *win);
	static GLContext *current();
	static Window *drawable();
	static bool validateCurrent();
	void present(Window &win);
	operator bool() const;
	void deinit();
};

}
