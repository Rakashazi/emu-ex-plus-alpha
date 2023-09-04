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

class PS3Controller : public BluetoothInputDevice
{
public:
	PS3Controller(ApplicationContext, BluetoothAddr);
	IG::ErrorCode open(BluetoothAdapter &adapter) final;
	IG::ErrorCode open1Ctl(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	IG::ErrorCode open2Int(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	void close();
	bool dataHandler(const char *data, size_t size);
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	void setLEDs(uint32_t player);
	const char *keyName(Input::Key k) const final;
	std::span<Input::Axis> motionAxes() final;
	static std::pair<Input::Key, Input::Key> joystickKeys(Input::AxisId);

private:
	static constexpr float axisScaler = 1./127.;
	uint8_t prevData[3]{};
	bool didSetLEDs = false;
	Input::Axis axis[4]
	{
		{*this, Input::AxisId::X,	axisScaler}, // Left X Axis
		{*this, Input::AxisId::Y, axisScaler}, // Left Y Axis
		{*this, Input::AxisId::Z, axisScaler}, // Right X Axis
		{*this, Input::AxisId::RZ, axisScaler} // Right Y Axis
	};
	BluetoothSocketSys ctlSock, intSock;
	BluetoothAddr addr;

	static uint8_t playerLEDs(uint32_t player);
	void sendFeatureReport();
};

}
