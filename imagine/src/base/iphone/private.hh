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

#include <CoreGraphics/CGColorSpace.h>
#include <imagine/base/iphone/config.h>
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

namespace Base
{

class ApplicationContext;
extern CGColorSpaceRef grayColorSpace, rgbColorSpace;

}

namespace Input
{

void init(Base::ApplicationContext);

}

#ifdef __OBJC__
@interface ImagineUIViewController : UIViewController

@end
#endif
