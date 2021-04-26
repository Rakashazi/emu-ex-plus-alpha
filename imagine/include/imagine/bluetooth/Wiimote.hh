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

class Wiimote : public BluetoothInputDevice, public Input::Device
{
public:
	static const uint8_t btClass[3], btClassDevOnly[3], btClassRemotePlus[3];
	static std::vector<Wiimote*> devList;

	Wiimote(Base::ApplicationContext ctx, BluetoothAddr addr): BluetoothInputDevice{ctx},
		Device{0, Input::Map::WIIMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Wiimote"},
		ctlSock{ctx}, intSock{ctx},
		addr{addr}
	{}
	IG::ErrorCode open(BluetoothAdapter &adapter) final;
	void close();
	void removeFromSystem() final;
	uint32_t joystickAxisBits() final;
	uint32_t joystickAxisAsDpadBitsDefault() final;
	void setJoystickAxisAsDpadBits(uint32_t axisMask) final;
	uint32_t joystickAxisAsDpadBits() final { return joystickAxisAsDpadBits_; }
	bool dataHandler(const char *data, size_t size);
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	void requestStatus();
	void setLEDs(uint32_t player);
	void sendDataMode(uint8_t mode);
	void writeReg(uint8_t offset, uint8_t val);
	void readReg(uint32_t offset, uint8_t size);
	const char *keyName(Input::Key k) const final;
	static bool isSupportedClass(const uint8_t devClass[3]);

private:
	BluetoothSocketSys ctlSock, intSock;
	int extension = EXT_NONE;
	uint32_t player = 0;
	uint32_t function = FUNC_NONE;
	uint32_t joystickAxisAsDpadBits_;
	Input::AxisKeyEmu<int> axisKey[4];
	uint8_t prevBtnData[2]{};
	uint8_t prevExtData[11]{};
	BluetoothAddr addr;
	bool identifiedType = false;

	struct ExtDevice : public Device
	{
		ExtDevice() {}
		ExtDevice(uint32_t devId, Input::Map map, uint32_t type, const char *name):
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

	static uint32_t findFreeDevId();
	void initExtension();
	void initExtensionPart2();
	static uint8_t playerLEDs(int player);
	void sendDataModeByExtension();
	static void decodeCCSticks(const uint8_t *ccSticks, int &lX, int &lY, int &rX, int &rY);
	static void decodeProSticks(const uint8_t *proSticks, int &lX, int &lY, int &rX, int &rY);
	void processCoreButtons(const uint8_t *packet, Input::Time time, uint32_t player);
	void processClassicButtons(const uint8_t *packet, Input::Time time, uint32_t player);
	void processProButtons(const uint8_t *packet, Input::Time time, uint32_t player);
	void processNunchukButtons(const uint8_t *packet, Input::Time time, uint32_t player);
};
