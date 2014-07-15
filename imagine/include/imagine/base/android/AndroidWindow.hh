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

#include <imagine/engine-globals.h>
#include <imagine/util/operators.hh>
#include <EGL/egl.h>

struct ANativeWindow;
struct ANativeActivity;

namespace Base
{

class AndroidWindow : public NotEquals<AndroidWindow>
{
public:
	ANativeWindow *nWin = nullptr;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLConfig eglConfig{};
	int pixelFormat = 0;
	IG::WindowRect contentRect; // active window content
	#ifdef CONFIG_BASE_MULTI_WINDOW
	jobject jDialog = nullptr;
	#endif
	bool initialInit = false;
	bool presented = false;

	constexpr AndroidWindow() {}

	bool operator ==(AndroidWindow const &rhs) const
	{
		return nWin == rhs.nWin;
	}

	operator bool() const
	{
		return initialInit;
	}

	void initEGLSurface(EGLDisplay display);
	void destroyEGLSurface(EGLDisplay display);
	void deinit();
};

using WindowImpl = AndroidWindow;

using GLConfig = EGLConfig;

}
