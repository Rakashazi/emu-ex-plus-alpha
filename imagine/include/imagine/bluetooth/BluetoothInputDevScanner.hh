#pragma once

#include <imagine/input/Input.hh>
#include <imagine/bluetooth/sys.hh>

namespace Base
{
class ApplicationContext;
}

namespace Bluetooth
{

bool scanForDevices(Base::ApplicationContext, BluetoothAdapter &, BluetoothAdapter::OnStatusDelegate);
bool listenForDevices(Base::ApplicationContext, BluetoothAdapter &bta, const BluetoothAdapter::OnStatusDelegate &onScanStatus);
void closeDevices(BluetoothAdapter *bta);
void closeBT(BluetoothAdapter *&bta);
uint32_t devsConnected();
uint32_t pendingDevs();
void connectPendingDevs(BluetoothAdapter *bta);

}
