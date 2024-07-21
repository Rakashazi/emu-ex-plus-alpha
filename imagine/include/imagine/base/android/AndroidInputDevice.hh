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
#include <imagine/util/container/ArrayList.hh>

namespace IG::Input
{

class AndroidInputDevice : public BaseDevice
{
public:
	AndroidInputDevice(int osId, InputDeviceTypeFlags, std::string name);
	AndroidInputDevice(int osId, int src, std::string name,
		int kbType, AxisFlags, uint32_t vendorProductId, bool isPowerButton);
	bool operator ==(AndroidInputDevice const& rhs) const;
	void setTypeFlags(InputDeviceTypeFlags f) { typeFlags_ = f; }
	std::span<Axis> motionAxes() { return axis; }
	void setICadeMode(bool on) { iCadeMode_ = on; }
	bool iCadeMode() const { return iCadeMode_; }
	auto &jsAxes() { return axis; }
	void update(const AndroidInputDevice&);
	void setSubtype(Subtype t) { subtype_ = t; };
	const std::string &name() const { return name_; }
	int id() const { return id_; }

protected:
	static constexpr uint32_t MAX_AXES = 14;
	StaticArrayList<Axis, MAX_AXES> axis;
	bool iCadeMode_{};
};

bool hasGetAxisValue();

}
