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

#include <imagine/bluetooth/sys.hh>
#include <imagine/input/inputDefs.hh>

namespace IG
{

class ErrorCode;

struct IControlPad : public BluetoothInputDevice
{
public:
	static constexpr std::array<uint8_t, 3> btClass{0x00, 0x1F, 0x00};

	IControlPad(ApplicationContext, BluetoothAddr);
	ErrorCode open(BluetoothAdapter &adapter) final;
	void close();
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	bool dataHandler(const char *packet, size_t size);
	const char *keyName(Input::Key k) const final;
	std::span<Input::Axis> motionAxes() final;
	static bool isSupportedClass(std::array<uint8_t, 3> devClass);
	static std::pair<Input::Key, Input::Key> joystickKeys(Input::AxisId);

private:
	enum
	{
		FUNC_NONE,
		FUNC_SET_LED_MODE,
		FUNC_GP_REPORTS,
	};
	static constexpr float axisScaler = 1./127.;
	BluetoothSocketSys sock;
	char inputBuffer[6]{};
	uint32_t inputBufferPos = 0;
	int function = 0;
	char prevBtnData[2]{};
	Input::Axis axis[4]
	{
		{*this, Input::AxisId::X, axisScaler}, // Left X Axis
		{*this, Input::AxisId::Y, axisScaler}, // Left Y Axis
		{*this, Input::AxisId::Z, axisScaler}, // Right X Axis
		{*this, Input::AxisId::RZ, axisScaler} // Right Y Axis
	};
	BluetoothAddr addr;

	void processBtnReport(const char *btnData, SteadyClockTimePoint time);
};

}
