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

class Wiimote : public BluetoothInputDevice, public Input::Device
{
public:
	static const uchar btClass[3], btClassDevOnly[3], btClassRemotePlus[3];
	static StaticArrayList<Wiimote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;

	Wiimote(BluetoothAddr addr):
		Device{0, Input::Event::MAP_WIIMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Wiimote"},
		addr{addr}
	{}
	CallResult open(BluetoothAdapter &adapter) override;
	void close();
	void removeFromSystem() override;
	uint joystickAxisBits() override;
	uint joystickAxisAsDpadBitsDefault() override;
	void setJoystickAxisAsDpadBits(uint axisMask) override;
	uint joystickAxisAsDpadBits() override { return joystickAxisAsDpadBits_; }
	bool dataHandler(const char *data, size_t size);
	uint statusHandler(BluetoothSocket &sock, uint status);
	void requestStatus();
	void setLEDs(uint player);
	void sendDataMode(uchar mode);
	void writeReg(uchar offset, uchar val);
	void readReg(uint offset, uchar size);
	const char *keyName(Input::Key k) const override;

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3)
			|| mem_equal(devClass, btClassDevOnly, 3)
			|| mem_equal(devClass, btClassRemotePlus, 3);
	}

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
		using Device::Device;
		const char *keyName(Input::Key k) const override;
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
	void processCoreButtons(const uchar *packet, uint player);
	void processClassicButtons(const uchar *packet, uint player);
	void processProButtons(const uchar *packet, uint player);
	void processNunchukButtons(const uchar *packet, uint player);
};
