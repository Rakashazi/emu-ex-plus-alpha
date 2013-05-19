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

struct Zeemote : public BluetoothInputDevice
{
public:
	Zeemote(BluetoothAddr addr): addr(addr) { }
	CallResult open(BluetoothAdapter &adapter) override;

	void close();

	void removeFromSystem() override;

	uint statusHandler(BluetoothSocket &sock, uint status);
	bool dataHandler(const uchar *packet, size_t size);

	static const uchar btClass[3];

	static bool isSupportedClass(const uchar devClass[3])
	{
		return mem_equal(devClass, btClass, 3);
	}

	static StaticDLList<Zeemote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys sock;
	Input::Device *device = nullptr;
	uchar inputBuffer[46] = {0};
	uint inputBufferPos = 0;
	uint packetSize = 0;
	bool prevBtnPush[4] = {0}, stickBtn[4] = {0};
	uint player;
	BluetoothAddr addr;

	static const uint RID_VERSION = 0x03, RID_BTN_METADATA = 0x04, RID_CONFIG_DATA = 0x05,
		RID_BTN_REPORT = 0x07, RID_8BA_2A_JS_REPORT = 0x08, RID_BATTERY_REPORT = 0x11;

	static uint findFreeDevId();
	static const char *reportIDToStr(uint id);
	void processBtnReport(const uchar *btnData, uint player);
	void processStickDataForButtonEmulation(const schar *pos, int player);
};
