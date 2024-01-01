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
#include <utility>

#ifdef __OBJC__
#import <GameController/GameController.h>
#endif

namespace IG
{
class ApplicationContext;
}

namespace IG::Input
{

void initAppleGameControllers(ApplicationContext);
std::pair<Input::Key, Input::Key> appleJoystickKeys(Input::AxisId);

class AppleGameDevice : public BaseDevice
{
public:
	ApplicationContext ctx;
	void *gcController_{};
	Input::Axis axis[4];
	bool pushState[Keycode::COUNT]{};

	#ifdef __OBJC__
	AppleGameDevice(ApplicationContext ctx, GCController *gcController);
	#endif
	~AppleGameDevice();

	std::span<Input::Axis> motionAxes()
	{
		return subtype_ == Subtype::APPLE_EXTENDED_GAMEPAD ? axis : std::span<Input::Axis>{};
	}

	const char *keyName(Key k) const;
	bool operator ==(AppleGameDevice const& rhs) const;
	void setKeys(Device &);
	#ifdef __OBJC__
	GCController *gcController() const { return (__bridge GCController*)gcController_; }
	#endif

private:
	#ifdef __OBJC__
	template <class T>
	void setGamepadBlocks(Device &, GCController *controller, T gamepad);
	void setExtendedGamepadBlocks(Device &, GCController *controller, GCExtendedGamepad *extGamepad);
	#endif
	void handleKey(Device &, Key key, bool pressed, bool repeatable = true);
};

}
