/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/pixmap/Pixmap.hh>
#include <mednafen/video/surface.h>

static Mednafen::MDFN_Surface pixmapToMDFNSurface(IG::MutablePixmapView pix)
{
	using namespace Mednafen;
	MDFN_PixelFormat fmt =
		[&]()
		{
			switch(pix.format().id())
			{
				case IG::PIXEL_BGRA8888: return MDFN_PixelFormat::ARGB32_8888;
				case IG::PIXEL_RGBA8888: return MDFN_PixelFormat::ABGR32_8888;
				case IG::PIXEL_RGB565: return MDFN_PixelFormat::RGB16_565;
				default:
					bug_unreachable("format id == %d", pix.format().id());
					return MDFN_PixelFormat::ABGR32_8888;
			};
		}();
	return {pix.data(), (uint32)pix.w(), (uint32)pix.h(), (uint32)pix.pitchPixels(), fmt};
}
