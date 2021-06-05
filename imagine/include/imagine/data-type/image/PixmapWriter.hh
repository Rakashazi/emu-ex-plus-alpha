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

#include <imagine/config/defs.hh>

#ifdef CONFIG_DATA_TYPE_IMAGE_LIBPNG
#include <imagine/data-type/image/LibPNG.hh>
#endif

#ifdef CONFIG_DATA_TYPE_IMAGE_QUARTZ2D
#include <imagine/data-type/image/Quartz2d.hh>
#endif

#ifdef CONFIG_DATA_TYPE_IMAGE_ANDROID
#include <imagine/data-type/image/Android.hh>
#endif

namespace Base
{
class ApplicationContext;
}

namespace IG
{
class Pixmap;
}

namespace IG::Data
{

class PixmapWriter final: public PixmapWriterImpl
{
public:
	using PixmapWriterImpl::PixmapWriterImpl;
	bool writeToFile(IG::Pixmap, const char *path);
};

}
