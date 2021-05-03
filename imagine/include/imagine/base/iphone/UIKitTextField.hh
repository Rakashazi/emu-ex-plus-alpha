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

#include <imagine/input/inputDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/NonCopyable.hh>

#ifdef __OBJC__
@class IGAppTextField;
#endif

namespace Input
{

class UIKitTextField : private NonCopyable
{
public:
	UIKitTextField(Base::ApplicationContext, TextFieldDelegate, const char *initialText, const char *promptText, int fontSizePixels);
	~UIKitTextField();

protected:
	Base::ApplicationContext ctx;
	void *textField_{};
	IG::WindowRect textRect{{8, 200}, {8+304, 200+48}};

	#ifdef __OBJC__
	IGAppTextField *textField() const { return (__bridge IGAppTextField*)textField_; }
	#endif
};

using TextFieldImpl = UIKitTextField;

}
