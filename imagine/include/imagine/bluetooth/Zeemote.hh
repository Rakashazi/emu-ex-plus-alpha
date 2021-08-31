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
#include <imagine/base/Error.hh>

struct Zeemote : public BluetoothInputDevice
{
public:
	static const uint8_t btClass[3];

	Zeemote(Base::ApplicationContext ctx, BluetoothAddr addr);
	IG::ErrorCode open(BluetoothAdapter &adapter) final;
	void close();
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	bool dataHandler(const char *packet, size_t size);
	const char *keyName(Input::Key k) const final;
	static bool isSupportedClass(const uint8_t devClass[3]);

private:
	BluetoothSocketSys sock;
	uint8_t inputBuffer[46]{};
	bool prevBtnPush[4]{};
	uint32_t inputBufferPos = 0;
	uint32_t packetSize = 0;
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
	BluetoothAddr addr;

	static const uint32_t RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static const char *reportIDToStr(uint32_t id);
	void processBtnReport(const uint8_t *btnData, Input::Time time);
};
