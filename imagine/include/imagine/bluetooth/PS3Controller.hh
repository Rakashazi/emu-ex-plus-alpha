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
#include <imagine/input/Device.hh>
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/base/Error.hh>
#include <vector>

class PS3Controller : public BluetoothInputDevice, public Input::Device
{
public:
	static std::vector<PS3Controller*> devList;

	PS3Controller(Base::ApplicationContext ctx, BluetoothAddr addr): BluetoothInputDevice{ctx},
		Device{0, Input::Map::PS3PAD, Input::Device::TYPE_BIT_GAMEPAD, "PS3 Controller"},
		ctlSock{ctx}, intSock{ctx},
		addr{addr}
	{}
	IG::ErrorCode open(BluetoothAdapter &adapter) final;
	IG::ErrorCode open1Ctl(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	IG::ErrorCode open2Int(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	void close();
	void removeFromSystem() final;
	bool dataHandler(const char *data, size_t size);
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	void setLEDs(uint32_t player);
	uint32_t joystickAxisBits() final;
	uint32_t joystickAxisAsDpadBitsDefault() final;
	void setJoystickAxisAsDpadBits(uint32_t axisMask) final;
	uint32_t joystickAxisAsDpadBits() final { return joystickAxisAsDpadBits_; }
	const char *keyName(Input::Key k) const final;

private:
	uint8_t prevData[3]{};
	bool didSetLEDs = false;
	Input::AxisKeyEmu<int> axisKey[4]
	{
		{
			64, 192,
			Input::PS3::LSTICK_LEFT, Input::PS3::LSTICK_RIGHT,
			Input::Keycode::JS1_XAXIS_NEG, Input::Keycode::JS1_XAXIS_POS
		}, // Left X Axis
		{
			64, 192,
			Input::PS3::LSTICK_UP, Input::PS3::LSTICK_DOWN,
			Input::Keycode::JS1_YAXIS_NEG, Input::Keycode::JS1_YAXIS_POS
		},  // Left Y Axis
		{
			64, 192,
			Input::PS3::RSTICK_LEFT, Input::PS3::RSTICK_RIGHT,
			Input::Keycode::JS2_XAXIS_NEG, Input::Keycode::JS2_XAXIS_POS
		}, // Right X Axis
		{
			64, 192,
			Input::PS3::RSTICK_UP, Input::PS3::RSTICK_DOWN,
			Input::Keycode::JS2_YAXIS_NEG, Input::Keycode::JS2_YAXIS_POS
		}   // Right Y Axis
	};
	BluetoothSocketSys ctlSock, intSock;
	uint32_t player = 0;
	uint32_t joystickAxisAsDpadBits_;
	BluetoothAddr addr;

	static uint32_t findFreeDevId();
	static uint8_t playerLEDs(uint32_t player);
	void sendFeatureReport();
};
