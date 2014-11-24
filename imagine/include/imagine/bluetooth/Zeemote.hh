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
#include <imagine/input/AxisKeyEmu.hh>
#include <imagine/util/container/ArrayList.hh>

struct Zeemote : public BluetoothInputDevice, public Input::Device
{
public:
	static const uchar btClass[3];
	static StaticArrayList<Zeemote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;

	Zeemote(BluetoothAddr addr):
		Device{0, Input::Event::MAP_ZEEMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Zeemote"},
		addr{addr}
	{}
	CallResult open(BluetoothAdapter &adapter) override;
	void close();
	void removeFromSystem() override;
	uint statusHandler(BluetoothSocket &sock, uint status);
	bool dataHandler(const char *packet, size_t size);
	const char *keyName(Input::Key k) const override;

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3);
	}

private:
	BluetoothSocketSys sock;
	uchar inputBuffer[46]{};
	bool prevBtnPush[4]{};
	uint inputBufferPos = 0;
	uint packetSize = 0;
	Input::AxisKeyEmu<int> axisKey[2]
	{
		{
			-63, 63,
			Input::Zeemote::LEFT, Input::Zeemote::RIGHT,
			Input::Keycode::LEFT, Input::Keycode::RIGHT
		}, // X Axis
		{
			-63, 63,
			Input::Zeemote::UP, Input::Zeemote::DOWN,
			Input::Keycode::UP, Input::Keycode::DOWN
		},  // Y Axis
	};
	uint player;
	BluetoothAddr addr;

	static const uint RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static uint findFreeDevId();
	static const char *reportIDToStr(uint id);
	void processBtnReport(const uchar *btnData, uint player);
};
