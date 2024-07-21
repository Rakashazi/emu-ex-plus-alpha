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

#include <imagine/bluetooth/BluetoothInputDevice.hh>
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/input/Axis.hh>

namespace IG
{

struct Zeemote : public BluetoothInputDevice
{
public:
	static constexpr std::array<uint8_t, 3> btClass{0x84, 0x05, 0x00};

	Zeemote(ApplicationContext ctx, BluetoothAddr addr);
	bool open(BluetoothAdapter &, Input::Device &) final;
	void close();
	uint32_t statusHandler(Input::Device&, BluetoothSocket&, BluetoothSocketState);
	bool dataHandler(Input::Device &, const char *packet, size_t size);
	const char *keyName(Input::Key k) const;
	std::span<Input::Axis> motionAxes() { return axis; };
	static bool isSupportedClass(std::array<uint8_t, 3> devClass);

private:
	static constexpr float axisScaler = 1./127.;
	BluetoothSocket sock;
	uint8_t inputBuffer[46]{};
	bool prevBtnPush[4]{};
	uint32_t inputBufferPos = 0;
	uint32_t packetSize = 0;
	Input::Axis axis[2]
	{
		{Input::AxisId::X, axisScaler},
		{Input::AxisId::Y, axisScaler}
	};
	BluetoothAddr addr;

	static const uint32_t RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static const char *reportIDToStr(uint32_t id);
	void processBtnReport(Input::Device &, const uint8_t *btnData, SteadyClockTimePoint time);
};

}
