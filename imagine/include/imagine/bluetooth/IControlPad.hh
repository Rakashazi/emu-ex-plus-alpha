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

struct IControlPad : public BluetoothInputDevice
{
public:
	static constexpr std::array<uint8_t, 3> btClass{0x00, 0x1F, 0x00};

	IControlPad(ApplicationContext, BluetoothAddr);
	bool open(BluetoothAdapter &, Input::Device &) final;
	void close();
	uint32_t statusHandler(Input::Device &, BluetoothSocket &, BluetoothSocketState status);
	bool dataHandler(Input::Device &, const char *packet, size_t size);
	const char *keyName(Input::Key k) const;
	std::span<Input::Axis> motionAxes() { return axis; }
	static bool isSupportedClass(std::array<uint8_t, 3> devClass);

private:
	enum
	{
		FUNC_NONE,
		FUNC_SET_LED_MODE,
		FUNC_GP_REPORTS,
	};
	static constexpr float axisScaler = 1./127.;
	BluetoothSocket sock;
	char inputBuffer[6]{};
	uint32_t inputBufferPos = 0;
	int function = 0;
	char prevBtnData[2]{};
	Input::Axis axis[4]
	{
		{Input::AxisId::X, axisScaler}, // Left X Axis
		{Input::AxisId::Y, axisScaler}, // Left Y Axis
		{Input::AxisId::Z, axisScaler}, // Right X Axis
		{Input::AxisId::RZ, axisScaler} // Right Y Axis
	};
	BluetoothAddr addr;

	void processBtnReport(Input::Device &, const char *btnData, SteadyClockTimePoint time);
};

}
