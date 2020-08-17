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

#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/thread/Semaphore.hh>
#include <android/native_window_jni.h>

namespace Gfx
{

class Renderer;

class SurfaceTextureStorage: public DirectTextureStorage
{
public:
	SurfaceTextureStorage(Renderer &r, GLuint tex, bool singleBuffered, Error &err);
	SurfaceTextureStorage(SurfaceTextureStorage &&o);
	SurfaceTextureStorage &operator=(SurfaceTextureStorage &&o);
	~SurfaceTextureStorage() final;
	Error setFormat(Renderer &r, IG::PixmapDesc desc, GLuint tex) final;
	Buffer lock(Renderer &r) final;
	void unlock(Renderer &r) final;

protected:
	void deinit();

	jobject surfaceTex{}, surface{};
	ANativeWindow *nativeWin{};
	uint8_t bpp = 0;
	bool singleBuffered = false;
};

}
