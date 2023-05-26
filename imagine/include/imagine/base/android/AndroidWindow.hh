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
#include <imagine/base/BaseWindow.hh>
#include <imagine/base/Animation.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/jni.hh>

struct ANativeWindow;
struct ANativeActivity;

namespace IG
{

class ApplicationContext;

using NativeWindowFormat = int32_t;
using NativeWindow = ANativeWindow*;

class AndroidWindow : public BaseWindow
{
public:
	static constexpr bool shouldRunOnInitAfterAddingWindow = false;

	using BaseWindow::BaseWindow;
	~AndroidWindow();
	explicit operator bool() const;
	void setNativeWindow(ApplicationContext, ANativeWindow *);
	int nativePixelFormat();
	void setContentRect(WRect bounds, WSize winSize);
	void systemRequestsRedraw(bool sync = true);

protected:
	enum class Type: uint8_t
	{
		NONE, MAIN, PRESENTATION
	};

	ANativeWindow *nWin{};
	JNI::UniqueGlobalRef jWin{};
	InitDelegate onInit{};
	int32_t nPixelFormat{};
	FrameAnimation<WindowRect, Window, InterpolatorType::EASEINOUTQUAD> contentRect{};
	Type type{};
};

using WindowImpl = AndroidWindow;

}
