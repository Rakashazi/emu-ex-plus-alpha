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
	jobject surfaceTex = nullptr, surface = nullptr;
	ANativeWindow* nativeWin = nullptr;
	IG::Pixmap pix {PixelFormatRGB565};
	TextureDesc desc;

	constexpr SurfaceTextureBufferImage() {}
	void init(int tid, IG::Pixmap &pixmap);
	void write(IG::Pixmap &p, uint hints) override;
	void write(IG::Pixmap &p, uint hints, uint alignment) override;
	void replace(IG::Pixmap &pixmap, uint hints) override;
	IG::Pixmap *lock(uint x, uint y, uint xlen, uint ylen, IG::Pixmap *fallback = nullptr) override;
	void unlock(IG::Pixmap *pix = nullptr, uint hints = 0) override;
	void deinit() override;
	const TextureDesc &textureDesc() const override { return desc; };
	TextureDesc &textureDesc() override { return desc; };
};

}
