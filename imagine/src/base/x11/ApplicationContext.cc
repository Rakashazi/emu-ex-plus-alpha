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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/pixmap/PixelFormat.hh>

namespace IG
{

void ApplicationContext::flushSystemInputEvents()
{
	application().runX11Events();
}

NativeDisplayConnection ApplicationContext::nativeDisplayConnection() const
{
	return {.conn = &application().xConnection(), .screen = &application().xScreen()};
}

PixelFormat ApplicationContext::defaultWindowPixelFormat() const
{
	return Config::MACHINE_IS_PANDORA ? PixelFmtRGB565 : PixelFmtRGBA8888;
}

}
