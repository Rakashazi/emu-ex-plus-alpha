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

struct Zeemote : public BluetoothInputDevice
{
public:
	static constexpr std::array<uint8_t, 3> btClass{0x84, 0x05, 0x00};

	Zeemote(ApplicationContext ctx, BluetoothAddr addr);
	ErrorCode open(BluetoothAdapter &adapter) final;
	void close();
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	bool dataHandler(const char *packet, size_t size);
	const char *keyName(Input::Key k) const final;
	std::span<Input::Axis> motionAxes() final;
	static bool isSupportedClass(std::array<uint8_t, 3> devClass);
	static std::pair<Input::Key, Input::Key> joystickKeys(Input::AxisId);

private:
	static constexpr float axisScaler = 1./127.;
	BluetoothSocketSys sock;
	uint8_t inputBuffer[46]{};
	bool prevBtnPush[4]{};
	uint32_t inputBufferPos = 0;
	uint32_t packetSize = 0;
	Input::Axis axis[2]
	{
		{*this, Input::AxisId::X, axisScaler},
		{*this, Input::AxisId::Y, axisScaler}
	};
	BluetoothAddr addr;

	static const uint32_t RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static const char *reportIDToStr(uint32_t id);
	void processBtnReport(const uint8_t *btnData, SteadyClockTimePoint time);
};

}
