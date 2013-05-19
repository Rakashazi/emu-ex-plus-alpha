#include "BluetoothAdapter.hh"
#include <bluetooth/sys.hh>

#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
bool BluetoothAdapter::useScanCache = 1;
#endif
#ifdef CONFIG_BLUETOOTH_SCAN_SECS
uint BluetoothAdapter::scanSecs = 4;
#endif

BluetoothAdapter *BluetoothAdapter::defaultAdapter()
{
	#if defined CONFIG_BLUETOOTH_ANDROID
		return AndroidBluetoothAdapter::defaultAdapter();
	#elif defined CONFIG_BLUETOOTH_BTSTACK
		return BtstackBluetoothAdapter::defaultAdapter();
	#elif defined CONFIG_BLUETOOTH_BLUEZ && !defined CONFIG_BASE_ANDROID
		return BluezBluetoothAdapter::defaultAdapter();
	#else
		static_assert(0, "no Bluetooth back-ends are selected");
		return nullptr;
	#endif
}
