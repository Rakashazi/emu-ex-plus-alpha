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
#include <imagine/gfx/PixmapBufferTexture.hh>
#include "../../../base/android/privateApi/GraphicBuffer.hh"

namespace Gfx
{

class GraphicBufferStorage: public DirectTextureStorage
{
public:
	GraphicBufferStorage();
	GraphicBufferStorage(GraphicBufferStorage &&o);
	GraphicBufferStorage &operator=(GraphicBufferStorage &&o);
	Error setFormat(Renderer &r, IG::PixmapDesc desc, GLuint tex) final;
	Buffer lock(Renderer &r) final;
	void unlock(Renderer &r) final;
	static bool canSupport(const char *rendererStr);
	static bool testSupport();
	static bool isSupported();

protected:
	Base::GraphicBuffer gBuff{};
	uint32_t pitch = 0;
	uint8_t bpp = 0;
	static bool testPassed_;
};

}
