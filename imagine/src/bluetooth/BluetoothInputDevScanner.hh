#pragma once

#include <input/interface.h>
#include <bluetooth/sys.hh>

namespace Bluetooth
{

bool startBT();
CallResult initBT();
void closeDevs();
void closeBT();
uint devsConnected();
uint pendingDevs();
void connectPendingDevs();

static const uint maxGamepadsPerTypeStorage = 5;
extern uint maxGamepadsPerType;
extern uint scanSecs;

}
