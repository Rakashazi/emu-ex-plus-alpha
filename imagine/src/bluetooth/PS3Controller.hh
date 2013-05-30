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

#include <bluetooth/sys.hh>
#include <input/Input.hh>
#include <input/AxisKeyEmu.hh>
#include <util/collection/DLList.hh>

class PS3Controller : public BluetoothInputDevice
{
public:
	PS3Controller(BluetoothAddr addr): addr(addr) { }
	CallResult open(BluetoothAdapter &adapter) override;
	CallResult open1Ctl(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	CallResult open2Int(BluetoothAdapter &adapter, BluetoothPendingSocket &pending);
	void close();
	void removeFromSystem() override;
	bool dataHandler(const uchar *data, size_t size);
	uint statusHandler(BluetoothSocket &sock, uint status);
	void setLEDs(uint player);

	static StaticDLList<PS3Controller*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	static uint findFreeDevId();
	static uchar playerLEDs(int player);
	void sendFeatureReport();
	uchar prevData[3] {0};
	Input::AxisKeyEmu<int> axisKey[4]
	{
		{64, 192, Input::PS3::LSTICK_LEFT, Input::PS3::LSTICK_RIGHT}, // Left X Axis
		{64, 192, Input::PS3::LSTICK_UP, Input::PS3::LSTICK_DOWN},  // Left Y Axis
		{64, 192, Input::PS3::RSTICK_LEFT, Input::PS3::RSTICK_RIGHT}, // Right X Axis
		{64, 192, Input::PS3::RSTICK_UP, Input::PS3::RSTICK_DOWN}   // Right Y Axis
	};
	BluetoothSocketSys ctlSock, intSock;
	Input::Device *device = nullptr;
	uint player = 0;
	BluetoothAddr addr;
	bool didSetLEDs = false;
};
