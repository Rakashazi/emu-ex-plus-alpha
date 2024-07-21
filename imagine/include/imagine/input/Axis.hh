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
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/time/Time.hh>

namespace IG
{
class Window;
}

namespace IG::Input
{

using AxisPair = std::pair<AxisId, AxisId>;
std::string_view toString(AxisId);

constexpr AxisPair toAxisIds(AxisSetId id)
{
	using enum AxisSetId;
	switch(id)
	{
		case stick1: return {AxisId::X, AxisId::Y};
		case stick2: return {AxisId::Z, AxisId::RZ};
		case hat: return {AxisId::HAT0X, AxisId::HAT0Y};
		case triggers: return {AxisId::LTRIGGER, AxisId::RTRIGGER};
		case pedals: return {AxisId::BRAKE, AxisId::GAS};
	}
	std::unreachable();
}

class Axis
{
public:
	constexpr Axis() = default;
	Axis(AxisId, float scaler = 1.f);
	void setEmulatesKeys(Map, bool);
	bool emulatesKeys() const;
	constexpr AxisId id() const { return id_; }
	AxisFlags idBit() const;
	bool isTrigger() const;
	float scale() const { return scaler; }
	std::string_view name() const;
	bool update(float pos, Map map, SteadyClockTimePoint time, const Device &, Window &, bool normalized = false);
	bool dispatchInputEvent(float pos, Map, SteadyClockTimePoint, const Device &, Window &);

protected:
	float scaler{};
	AxisKeyEmu keyEmu;
	AxisId id_{};
};

}
