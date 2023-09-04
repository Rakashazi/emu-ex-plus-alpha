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

class Wiimote final: public BluetoothInputDevice
{
public:
	static constexpr std::array<uint8_t, 3> btClass{0x04, 0x25, 0x00};
	static constexpr std::array<uint8_t, 3> btClassDevOnly{0x04, 0x05, 0x00};
	static constexpr std::array<uint8_t, 3> btClassRemotePlus{0x08, 0x05, 0x00};

	Wiimote(ApplicationContext, BluetoothAddr);
	~Wiimote();
	ErrorCode open(BluetoothAdapter &adapter) final;
	bool dataHandler(const char *data, size_t size);
	uint32_t statusHandler(BluetoothSocket &sock, uint32_t status);
	void requestStatus();
	void setLEDs(uint8_t player);
	void sendDataMode(uint8_t mode);
	void writeReg(uint8_t offset, uint8_t val);
	void readReg(uint32_t offset, uint8_t size);
	const char *keyName(Input::Key k) const final;
	std::span<Input::Axis> motionAxes() final;
	static bool isSupportedClass(std::array<uint8_t, 3> devClass);
	static std::pair<Input::Key, Input::Key> joystickKeys(Input::Map, Input::AxisId);

private:
	BluetoothSocketSys ctlSock, intSock;
	int extension = EXT_NONE;
	uint32_t function = FUNC_NONE;
	Input::Axis axis[4];
	uint8_t prevBtnData[2]{};
	uint8_t prevExtData[11]{};
	BluetoothAddr addr;
	bool identifiedType = false;

	struct ExtDevice : public Device
	{
		ExtDevice() {}
		ExtDevice(Input::Map map, Input::DeviceTypeFlags typeFlags, std::string name):
			Device{0, map, typeFlags, std::move(name)} {}
		const char *keyName(Input::Key k) const final;
	};
	Input::Device *extDevicePtr{};

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

	void initExtension();
	void initExtensionPart2();
	static uint8_t playerLEDs(int player);
	void sendDataModeByExtension();
	static void decodeCCSticks(const uint8_t *ccSticks, int &lX, int &lY, int &rX, int &rY);
	static void decodeProSticks(const uint8_t *proSticks, int &lX, int &lY, int &rX, int &rY);
	void processCoreButtons(const uint8_t *packet, SteadyClockTimePoint time);
	void processClassicButtons(const uint8_t *packet, SteadyClockTimePoint time);
	void processProButtons(const uint8_t *packet, SteadyClockTimePoint time);
	void processNunchukButtons(const uint8_t *packet, SteadyClockTimePoint time);
	void removeExtendedDevice();
};

}
