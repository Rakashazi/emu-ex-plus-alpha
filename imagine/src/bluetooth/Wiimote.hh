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
#include <util/collection/DLList.hh>

class Wiimote : public BluetoothInputDevice
{
public:
	Wiimote(BluetoothAddr addr): addr(addr) { }
	CallResult open(BluetoothAdapter &adapter) override;
	void close();
	void removeFromSystem() override;
	bool dataHandler(const uchar *data, size_t size);
	uint statusHandler(BluetoothSocket &sock, uint status);
	void requestStatus();
	void setLEDs(uint player);
	void sendDataMode(uchar mode);
	void writeReg(uchar offset, uchar val);
	void readReg(uint offset, uchar size);

	static const uchar btClass[3], btClassDevOnly[3], btClassRemotePlus[3];

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3)
			|| mem_equal(devClass, btClassDevOnly, 3)
			|| mem_equal(devClass, btClassRemotePlus, 3);
	}

	static StaticDLList<Wiimote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys ctlSock, intSock;
	Input::Device *device = nullptr, *subDevice = nullptr;
	int extension = EXT_NONE;
	uint player = 0;
	uint function = FUNC_NONE;
	bool stickBtn[8] = {0};
	uchar prevBtnData[2] = {0};
	uchar prevExtData[10] = {0};
	BluetoothAddr addr;
	bool identifiedType = 0;

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
	void processStickDataForButtonEmulation(int player, const uchar *data);
	void processProStickDataForButtonEmulation(int player, const uchar *data);
	void processCoreButtons(const uchar *packet, uint player);
	void processClassicButtons(const uchar *packet, uint player);
	void processProButtons(const uchar *packet, uint player);
	void processNunchukStickDataForButtonEmulation(int player, const uchar *data);
	void processNunchukButtons(const uchar *packet, uint player);
};

static const Input::PackedInputAccess wiimoteDataAccess[] =
{
	{ 0, BIT(0), Input::Wiimote::DOWN }, // map to sideways control
	{ 0, BIT(1), Input::Wiimote::UP },
	{ 0, BIT(2), Input::Wiimote::RIGHT },
	{ 0, BIT(3), Input::Wiimote::LEFT },
	{ 0, BIT(4), Input::Wiimote::PLUS },

	{ 1, BIT(0), Input::Wiimote::_2 },
	{ 1, BIT(1), Input::Wiimote::_1 },
	{ 1, BIT(2), Input::Wiimote::B },
	{ 1, BIT(3), Input::Wiimote::A },
	{ 1, BIT(4), Input::Wiimote::MINUS },
	{ 1, BIT(7), Input::Wiimote::HOME },
};

static const Input::PackedInputAccess wiimoteCCDataAccess[] =
{
	{ 4, BIT(7), Input::WiiCC::RIGHT },
	{ 4, BIT(6), Input::WiiCC::DOWN },
	{ 4, BIT(5), Input::WiiCC::L },
	{ 4, BIT(4), Input::WiiCC::MINUS },
	{ 4, BIT(3), Input::WiiCC::HOME },
	{ 4, BIT(2), Input::WiiCC::PLUS },
	{ 4, BIT(1), Input::WiiCC::R },

	{ 5, BIT(7), Input::WiiCC::ZL },
	{ 5, BIT(6), Input::WiiCC::B },
	{ 5, BIT(5), Input::WiiCC::Y },
	{ 5, BIT(4), Input::WiiCC::A },
	{ 5, BIT(3), Input::WiiCC::X },
	{ 5, BIT(2), Input::WiiCC::ZR },
	{ 5, BIT(1), Input::WiiCC::LEFT },
	{ 5, BIT(0), Input::WiiCC::UP },
};

static const Input::PackedInputAccess wiimoteProDataAccess[] =
{
	{ 8, BIT(7), Input::WiiCC::RIGHT },
	{ 8, BIT(6), Input::WiiCC::DOWN },
	{ 8, BIT(5), Input::WiiCC::L },
	{ 8, BIT(4), Input::WiiCC::MINUS },
	{ 8, BIT(3), Input::WiiCC::HOME },
	{ 8, BIT(2), Input::WiiCC::PLUS },
	{ 8, BIT(1), Input::WiiCC::R },

	{ 9, BIT(7), Input::WiiCC::ZL },
	{ 9, BIT(6), Input::WiiCC::B },
	{ 9, BIT(5), Input::WiiCC::Y },
	{ 9, BIT(4), Input::WiiCC::A },
	{ 9, BIT(3), Input::WiiCC::X },
	{ 9, BIT(2), Input::WiiCC::ZR },
	{ 9, BIT(1), Input::WiiCC::LEFT },
	{ 9, BIT(0), Input::WiiCC::UP },
};

static const Input::PackedInputAccess wiimoteNunchukDataAccess[] =
{
	{ 5, BIT(1), Input::Wiimote::NUN_C },
	{ 5, BIT(0), Input::Wiimote::NUN_Z },
};
