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
#include <android/native_window_jni.h>

namespace Gfx
{

struct SurfaceTextureStorage: public DirectTextureStorage
{
	jobject surfaceTex{}, surface{};
	ANativeWindow* nativeWin{};
	uint bpp = 0;

	constexpr SurfaceTextureStorage() {}
	~SurfaceTextureStorage() override;
	CallResult init(GLuint tex);
	CallResult setFormat(IG::PixmapDesc desc, GLuint tex) override;
	Buffer lock() override;
	void unlock(GLuint tex) override;
};

}
