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

#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/bluetooth/sys.hh>

namespace IG
{

#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
bool BluetoothAdapter::useScanCache = 1;
#endif
#ifdef CONFIG_BLUETOOTH_SCAN_SECS
uint32_t BluetoothAdapter::scanSecs = 4;
#endif

BluetoothAdapter *BluetoothAdapter::defaultAdapter(ApplicationContext ctx)
{
	#if defined CONFIG_BLUETOOTH_ANDROID
	return AndroidBluetoothAdapter::defaultAdapter(ctx);
	#elif defined CONFIG_BLUETOOTH_BTSTACK
	return BtstackBluetoothAdapter::defaultAdapter(ctx);
	#elif defined CONFIG_BLUETOOTH_BLUEZ
	return BluezBluetoothAdapter::defaultAdapter(ctx);
	#else
	#error "no Bluetooth back-ends are selected"
	#endif
}

ApplicationContext BluetoothAdapter::appContext() const
{
	return ctx;
}

void BluetoothAdapter::setAppContext(ApplicationContext ctx_)
{
	ctx = ctx_;
}

BluetoothInputDevice::BluetoothInputDevice(ApplicationContext ctx, Input::Map map,
	Input::DeviceTypeFlags typeFlags, const char *name):
	Device{0, map, typeFlags, name},
	ctx{ctx}
{}

}
