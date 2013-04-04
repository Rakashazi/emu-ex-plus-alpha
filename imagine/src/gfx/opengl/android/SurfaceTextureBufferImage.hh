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

#include <gfx/GfxBufferImage.hh>
#include <android/native_window_jni.h>

namespace Gfx
{

struct SurfaceTextureBufferImage: public BufferImageInterface
{
	constexpr SurfaceTextureBufferImage() { }
	jobject surfaceTex = nullptr, surface = nullptr;
	ANativeWindow* nativeWin = nullptr;
	Pixmap pix {PixelFormatRGB565}, *backingPix = nullptr;

	void init(int tid, Pixmap &pixmap);
	void write(Pixmap &p, uint hints) override;
	void write(Pixmap &p, uint hints, uint alignment) override;
	void replace(Pixmap &pixmap, uint hints) override;
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback = nullptr) override;
	void unlock(Pixmap *pix = nullptr, uint hints = 0) override;
	void deinit() override;
};

}
