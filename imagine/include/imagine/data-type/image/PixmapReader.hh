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

#ifdef __ANDROID__
#include <imagine/data-type/image/Android.hh>
#elif defined __linux__
#include <imagine/data-type/image/LibPNG.hh>
#elif defined __APPLE__
#include <imagine/data-type/image/Quartz2d.hh>
#endif

#include <imagine/base/ApplicationContext.hh>
#include <system_error>

class GenericIO;

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

class PixmapSource;

class PixmapImage: public PixmapImageImpl
{
public:
	using PixmapImageImpl::PixmapImageImpl;
	std::errc write(IG::Pixmap dest);
	IG::Pixmap pixmapView();
	explicit operator bool() const;
	operator PixmapSource();
};

class PixmapReader final: public PixmapReaderImpl
{
public:
	using PixmapReaderImpl::PixmapReaderImpl;
	PixmapImage load(GenericIO io) const;
	PixmapImage load(const char *name) const;
	PixmapImage loadAsset(const char *name, const char *appName = Base::ApplicationContext::applicationName) const;
};

}
