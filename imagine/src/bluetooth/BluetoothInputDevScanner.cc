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

#define thisModuleName "btInput"
#include "BluetoothAdapter.hh"
#include "Wiimote.hh"
#include "Zeemote.hh"
#include "IControlPad.hh"
#include <util/collection/DLList.hh>

StaticDLList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 3> btInputDevList;

namespace Bluetooth
{
	uint scanSecs = 4;
	uint maxGamepadsPerType = 5;
	static BluetoothAdapter *bta = 0;

	static bool testSupportedBTDevClasses(const uchar devClass[3])
	{
		return Wiimote::isSupportedClass(devClass) ||
				IControlPad::isSupportedClass(devClass) ||
				Zeemote::isSupportedClass(devClass);
	}

	fbool onClass(const uchar devClass[3])
	{
		logMsg("class: %X:%X:%X", devClass[0], devClass[1], devClass[2]);
		return testSupportedBTDevClasses(devClass);
	}

	void onName(const char *name, BluetoothAddr addr)
	{
		logMsg("name: %s", name);
		if(strstr(name, "Nintendo RVL-CNT-01"))
		{
			Wiimote *dev = new Wiimote;
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			if(dev->open(name, addr, *bta) != OK)
			{
				delete dev;
				return;
			}
		}
		else if(strstr(name, "iControlPad-"))
		{
			IControlPad *dev = new IControlPad;
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			if(dev->open(addr, *bta) != OK)
			{
				delete dev;
				return;
			}
		}
		else if(strstr(name, "Zeemote JS1"))
		{
			Zeemote *dev = new Zeemote;
			if(!dev)
			{
				logErr("out of memory");
				return;
			}
			if(dev->open(addr, *bta) != OK)
			{
				delete dev;
				return;
			}
		}
	}

	bool startBT()
	{
		assert(bta);
		return bta->startScan();
	}

	CallResult initBT()
	{
		if(bta)
			return OK; // already init

		bta = BluetoothAdapter::defaultAdapter();
		if(!bta)
			return UNSUPPORTED_OPERATION;
		btInputDevList.init();
		Wiimote::devList.init();
		IControlPad::devList.init();
		Zeemote::devList.init();
		bta->scanDeviceClassDelegate().bind<&onClass>();
		bta->scanDeviceNameDelegate().bind<&onName>();
		return OK;
	}

	void closeDevs()
	{
		if(!bta)
			return; // Bluetooth was never used
		logMsg("closing all BT input devs");
		while(btInputDevList.size)
		{
			(*btInputDevList.first())->removeFromSystem();
		}
	}

	void closeBT()
	{
		if(!bta)
			return; // Bluetooth was never used
		closeDevs();
		bta->close();
		bta = 0;
	}

	uint devsConnected()
	{
		return btInputDevList.size;
	}
}
