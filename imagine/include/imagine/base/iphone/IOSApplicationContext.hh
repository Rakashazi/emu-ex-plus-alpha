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
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

namespace IG
{

class Application;
class Window;

using NativeDisplayConnection = void*;

class IOSApplicationContext
{
public:
	constexpr IOSApplicationContext() = default;
	#ifdef __OBJC__
	constexpr IOSApplicationContext(UIApplication *app):uiAppPtr{(__bridge void*)app} {}
	UIApplication *uiApp() const { return (__bridge UIApplication*)uiAppPtr; }
	#endif
	void setApplicationPtr(Application*);
	Application &application() const;
	bool deviceIsIPad() const;
	bool isSystemApp() const;
	Window *deviceWindow() const;

protected:
	void *uiAppPtr{};
};

using ApplicationContextImpl = IOSApplicationContext;

}
