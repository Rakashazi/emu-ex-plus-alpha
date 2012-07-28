#include "BluetoothAdapter.hh"
#include "sys.hh"

#if defined CONFIG_BLUEZ && defined CONFIG_ANDROIDBT
bool useBluezBT = 1;
#endif

bool BluetoothAdapter::useScanCache = 1;

BluetoothAdapter *BluetoothAdapter::defaultAdapter()
{
	#if defined CONFIG_BLUEZ && defined CONFIG_ANDROIDBT
		if(useBluezBT)
		{
			auto adapter = BluezBluetoothAdapter::defaultAdapter();
			if(adapter)
				return adapter;
		}
		logMsg("No Bluez, using Android BT");
		return AndroidBluetoothAdapter::defaultAdapter();
	#elif defined CONFIG_BTSTACK
		return BtstackBluetoothAdapter::defaultAdapter();
	#elif defined CONFIG_BLUEZ
		return BluezBluetoothAdapter::defaultAdapter();
	#else
		return AndroidBluetoothAdapter::defaultAdapter();
	#endif
}
