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

struct IControlPad : public BluetoothInputDevice
{
public:
	IControlPad(BluetoothAddr addr): addr(addr) { }
	enum
	{
		FUNC_NONE,
		FUNC_SET_LED_MODE,
		FUNC_GP_REPORTS,
	};

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

	static StaticDLList<IControlPad*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> devList;
private:
	BluetoothSocketSys sock;
	Input::Device *device = nullptr;
	uchar inputBuffer[6] = {0};
	uint inputBufferPos = 0;
	uint player = 0;
	int function = 0;
	uchar prevBtnData[2] = {0};
	bool nubBtn[8] = {0};
	static const int nubDeadzone = 64;
	BluetoothAddr addr;

	static uint findFreeDevId();
	void processBtnReport(const uchar *btnData, uint player);
	void processNubDataForButtonEmulation(const schar *nubData, uint player);
};
