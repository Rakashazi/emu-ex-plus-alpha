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

#include <imagine/gfx/Texture.hh>
#include <jni.h>

struct ANativeWindow;

namespace IG::Gfx
{

class Renderer;

class SurfaceTextureStorage final: public Texture
{
public:
	SurfaceTextureStorage(RendererTask &, TextureConfig config, bool singleBuffered);
	SurfaceTextureStorage(SurfaceTextureStorage &&o) noexcept;
	SurfaceTextureStorage &operator=(SurfaceTextureStorage &&o) noexcept;
	~SurfaceTextureStorage();
	bool setFormat(PixmapDesc desc, ColorSpace, TextureSamplerConfig);
	LockedTextureBuffer lock(TextureBufferFlags bufferFlags);
	void unlock(LockedTextureBuffer lockBuff, TextureWriteFlags writeFlags);

protected:
	jobject surfaceTex{};
	jobject surface{};
	ANativeWindow *nativeWin{};
	uint8_t bpp = 0;
	bool singleBuffered = false;

	void deinit();
};

}
