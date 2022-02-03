#pragma once

#include <imagine/bluetooth/sys.hh>

namespace IG
{
class ApplicationContext;
}

namespace IG::Bluetooth
{

bool scanForDevices(ApplicationContext, BluetoothAdapter &, BluetoothAdapter::OnStatusDelegate);
bool listenForDevices(ApplicationContext, BluetoothAdapter &bta, const BluetoothAdapter::OnStatusDelegate &onScanStatus);
void closeDevices(BluetoothAdapter *bta);
void closeBT(BluetoothAdapter *bta);
uint32_t devsConnected(ApplicationContext);
uint32_t pendingDevs();
void connectPendingDevs(BluetoothAdapter *bta);

}
