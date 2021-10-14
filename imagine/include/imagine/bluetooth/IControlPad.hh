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
#include <imagine/input/Input.hh>
#include <imagine/base/Error.hh>

struct IControlPad : public BluetoothInputDevice
{
public:
	static const uint8_t btClass[3];

	IControlPad(Base::ApplicationContext, BluetoothAddr);
	IG::ErrorCode open(BluetoothAdapter &adapter) final;
	void close();
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	bool dataHandler(const char *packet, size_t size);
	const char *keyName(Input::Key k) const final;
	std::span<Input::Axis> motionAxes() final;
	static bool isSupportedClass(const uint8_t devClass[3]);
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

	void processBtnReport(const char *btnData, Input::Time time);
};
