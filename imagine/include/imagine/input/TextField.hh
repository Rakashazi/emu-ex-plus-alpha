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
#if defined __ANDROID__
#include <imagine/base/android/AndroidTextField.hh>
#elif (defined __APPLE__ && TARGET_OS_IPHONE)
#include <imagine/base/iphone/UIKitTextField.hh>
#else
#include <imagine/base/ApplicationContext.hh>
#include <imagine/input/inputDefs.hh>
namespace IG::Input
{
class TextFieldImpl
{
public:
	constexpr TextFieldImpl(ApplicationContext, TextFieldDelegate, const char*, const char*, int) {}
};
}
#endif

#include <imagine/config/defs.hh>
#include <imagine/util/rectangle2.h>

namespace IG::Input
{

class TextField : public TextFieldImpl
{
public:
	using TextFieldImpl::TextFieldImpl;
	void cancel();
	void finish();
	void place(WRect rect);
	WRect windowRect() const;
};

}
