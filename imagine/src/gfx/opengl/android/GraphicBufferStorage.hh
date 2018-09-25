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

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <imagine/gfx/Texture.hh>
#include "../../../base/android/privateApi/GraphicBuffer.hh"

namespace Gfx
{

struct GraphicBufferStorage: public DirectTextureStorage
{
	Base::GraphicBuffer gBuff{};
	EGLImageKHR eglImg = EGL_NO_IMAGE_KHR;
	uint bpp = 0;
	uint pitch = 0;
	static bool testPassed;

	GraphicBufferStorage() {}
	~GraphicBufferStorage() final;
	Error setFormat(Renderer &r, IG::PixmapDesc desc, GLuint tex) final;
	Buffer lock(Renderer &r, IG::WindowRect *dirtyRect) final;
	void unlock(Renderer &r, GLuint tex) final;
	bool isSingleBuffered() const final;
	void resetImage(EGLDisplay dpy);
	void reset(EGLDisplay dpy);
	static bool isRendererWhitelisted(const char *rendererStr);
};

}
