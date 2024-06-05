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

struct xcb_input_xi_device_info_t;

namespace IG
{
struct XIDeviceInfo;
}

namespace IG::Input
{

class XInputDevice : public BaseDevice
{
public:
	XInputDevice() = default;
	XInputDevice(InputDeviceTypeFlags, std::string name);
	XInputDevice(XIDeviceInfo, bool isPointingDevice, bool isPowerButton);
	XInputDevice(xcb_input_xi_device_info_t&, bool isPointingDevice, bool isPowerButton);
	void setICadeMode(bool on) { iCadeMode_ = on; }
	bool iCadeMode() const { return iCadeMode_; }

protected:
	bool iCadeMode_{};
};

}
