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

#include <imagine/bluetooth/defs.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/base/ApplicationContext.hh>

namespace IG
{

class ApplicationContext;

class BluetoothInputDevice : public Input::BaseDevice
{
public:
	BluetoothInputDevice(ApplicationContext, Input::Map, Input::DeviceTypeFlags, const char* name);
	virtual bool open(BluetoothAdapter&, Input::Device&) = 0;

protected:
	ApplicationContext ctx;
};

}

namespace IG::Bluetooth
{

bool scanForDevices(ApplicationContext, BluetoothAdapter&, BTOnStatusDelegate);
bool listenForDevices(ApplicationContext, BluetoothAdapter&, const BTOnStatusDelegate&);
void closeDevices(BluetoothAdapter&);
void closeBT(BluetoothAdapter&);
size_t devsConnected(ApplicationContext);
size_t pendingDevs();
void connectPendingDevs(BluetoothAdapter&);

}
