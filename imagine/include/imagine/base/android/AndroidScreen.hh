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

#include <imagine/engine-globals.h>
#include <imagine/util/operators.hh>
#include <jni.h>

namespace Base
{

enum SurfaceRotation : int;

class AndroidScreen : public NotEquals<AndroidScreen>
{
public:
	jobject aDisplay{};
	float xDPI{}, yDPI{};
	float densityDPI{};
	float refreshRate_{};
	int width_{}, height_{};
	FrameTimeBase currFrameTime{}; // only used if Choreographer class isn't present
	#ifdef CONFIG_BASE_MULTI_SCREEN
	int id{};
	#else
	static constexpr int id = 0;
	#endif

	constexpr AndroidScreen() {}

	void init(JNIEnv *env, jobject aDisplay, jobject metrics, bool isMain);
	SurfaceRotation rotation(JNIEnv *env);

	bool operator ==(AndroidScreen const &rhs) const
	{
		return id == rhs.id;
	}

	explicit operator bool() const
	{
		return aDisplay;
	}
};

using ScreenImpl = AndroidScreen;

}
