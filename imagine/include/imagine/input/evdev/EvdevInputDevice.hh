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

#include <imagine/input/Axis.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <array>
#include <span>

namespace IG
{
class LinuxApplication;
}

namespace IG::Input
{

class EvdevInputDevice : public BaseDevice
{
public:
	EvdevInputDevice(int id, UniqueFileDescriptor fd, DeviceTypeFlags, std::string name, uint32_t vendorProductId);
	std::span<Axis> motionAxes() { return axis; };
	int fd() const { return fdSrc.fd(); }
	static void addPollEvent(Device&, LinuxApplication&);

protected:
	static constexpr unsigned AXIS_SIZE = 24;
	StaticArrayList<Axis, AXIS_SIZE> axis;
	std::array<int, AXIS_SIZE> axisRangeOffset{};
	FDEventSource fdSrc;

	static void processInputEvents(Device&, LinuxApplication&, std::span<const input_event>);
	bool setupJoystickBits();
};

}
