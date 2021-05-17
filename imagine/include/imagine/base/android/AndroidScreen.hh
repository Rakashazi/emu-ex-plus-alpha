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
#include <imagine/time/Time.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/base/android/Choreographer.hh>
#include <imagine/util/jni.hh>
#include <utility>
#include <compare>
#include <memory>

namespace Base
{

class ApplicationContext;

enum SurfaceRotation : uint8_t;

using FrameTimerVariant = std::variant<NativeChoreographerFrameTimer, JavaChoreographerFrameTimer, SimpleFrameTimer>;

class FrameTimer : public FrameTimerVariantWrapper<FrameTimerVariant>
{
public:
	using FrameTimerVariantWrapper::FrameTimerVariantWrapper;
};

class AndroidScreen
{
public:
	struct InitParams
	{
		JNIEnv *env;
		jobject aDisplay;
		jobject metrics;
		int id;
		float refreshRate;
		SurfaceRotation rotation;
	};

	AndroidScreen(ApplicationContext, InitParams);
	std::pair<float, float> dpi() const;
	float densityDPI() const;
	jobject displayObject() const;
	int id() const;
	void updateRefreshRate(float refreshRate);
	bool operator ==(AndroidScreen const &rhs) const;
	explicit operator bool() const;

	constexpr bool operator ==(ScreenId id) const
	{
		return id_ == id;
	}

protected:
	JNI::UniqueGlobalRef aDisplay{};
	FrameTimer frameTimer;
	IG::FloatSeconds frameTime_{};
	float xDPI{}, yDPI{};
	float densityDPI_{};
	float refreshRate_{};
	int width_{}, height_{};
	int id_{};
	bool reliableRefreshRate = true;
};

using ScreenImpl = AndroidScreen;

}
