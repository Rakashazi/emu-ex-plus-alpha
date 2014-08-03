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

#include <imagine/gfx/GfxBufferImage.hh>
#include "../../../base/android/android.hh"

namespace Gfx
{

struct DirectTextureBufferImage: public TextureBufferImage
{
	IG::Pixmap eglPixmap {PixelFormatRGB565};
	android_native_buffer_t eglBuf;
	EGLImageKHR eglImg = EGL_NO_IMAGE_KHR;

	constexpr DirectTextureBufferImage() {}
	static bool testSupport(const char **errorStr);
	bool init(IG::Pixmap &pix, uint texRef, uint usedX, uint usedY, const char **errorStr = nullptr);
	void write(IG::Pixmap &p, uint hints) override;
	void write(IG::Pixmap &p, uint hints, uint alignment) override;
	IG::Pixmap *lock(uint x, uint y, uint xlen, uint ylen, IG::Pixmap *fallback = nullptr) override;
	void unlock(IG::Pixmap *p = nullptr, uint hints = 0) override;
	void deinit() override;

private:
	bool initTexture(IG::Pixmap &pix, uint usedX, uint usedY, bool testLock, const char **errorStr = nullptr);
};

}
