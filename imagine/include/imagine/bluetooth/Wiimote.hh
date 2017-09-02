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
#include <vector>

class Wiimote : public BluetoothInputDevice, public Input::Device
{
public:
	static const uchar btClass[3], btClassDevOnly[3], btClassRemotePlus[3];
	static std::vector<Wiimote*> devList;

	Wiimote(BluetoothAddr addr):
		Device{0, Input::Event::MAP_WIIMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Wiimote"},
		addr{addr}
	{}
	CallResult open(BluetoothAdapter &adapter) final;
	void close();
	void removeFromSystem() final;
	uint joystickAxisBits() final;
	uint joystickAxisAsDpadBitsDefault() final;
	void setJoystickAxisAsDpadBits(uint axisMask) final;
	uint joystickAxisAsDpadBits() final { return joystickAxisAsDpadBits_; }
	bool dataHandler(const char *data, size_t size);
	uint statusHandler(BluetoothSocket &sock, uint status);
	void requestStatus();
	void setLEDs(uint player);
	void sendDataMode(uchar mode);
	void writeReg(uchar offset, uchar val);
	void readReg(uint offset, uchar size);
	const char *keyName(Input::Key k) const final;
	static bool isSupportedClass(const uchar devClass[3]);

private:
	BluetoothSocketSys ctlSock, intSock;
	int extension = EXT_NONE;
	uint player = 0;
	uint function = FUNC_NONE;
	uint joystickAxisAsDpadBits_;
	Input::AxisKeyEmu<int> axisKey[4];
	uchar prevBtnData[2]{};
	uchar prevExtData[11]{};
	BluetoothAddr addr;
	bool identifiedType = false;

	struct ExtDevice : public Device
	{
		ExtDevice() {}
		ExtDevice(uint devId, uint map, uint type, const char *name):
			Device{devId, map, type, name} {}
		const char *keyName(Input::Key k) const final;
	};
	ExtDevice extDevice;

	enum
	{
		FUNC_NONE,
		FUNC_INIT_EXT, // extension init part 1
		FUNC_INIT_EXT_DONE, // extension init part 2
		FUNC_GET_EXT_TYPE, // return the extension type register
	};

	enum
	{
		EXT_NONE, EXT_CC, EXT_NUNCHUK, EXT_WIIU_PRO, EXT_UNKNOWN
	};

	static uint findFreeDevId();
	void initExtension();
	void initExtensionPart2();
	static uchar playerLEDs(int player);
	void sendDataModeByExtension();
	static void decodeCCSticks(const uchar *ccSticks, int &lX, int &lY, int &rX, int &rY);
	static void decodeProSticks(const uchar *proSticks, int &lX, int &lY, int &rX, int &rY);
	void processCoreButtons(const uchar *packet, Input::Time time, uint player);
	void processClassicButtons(const uchar *packet, Input::Time time, uint player);
	void processProButtons(const uchar *packet, Input::Time time, uint player);
	void processNunchukButtons(const uchar *packet, Input::Time time, uint player);
};
