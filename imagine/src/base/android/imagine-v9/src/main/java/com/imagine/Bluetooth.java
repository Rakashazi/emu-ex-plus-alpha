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

package com.imagine;

import android.app.*;
import android.content.*;
import android.os.*;
import android.bluetooth.*;
import android.util.*;
import java.util.*;
import java.lang.reflect.*;

final class Bluetooth
{
	private static final String logTag = "ImagineBluetooth";
	private static final int TYPE_L2CAP = 3;
	private static ArrayList<BluetoothDevice> devs = null;
	private static final Constructor<?> l2capInsecureSocketConstructor = Util.getConstructor(BluetoothSocket.class, new Class[] {int.class, int.class, boolean.class, boolean.class, BluetoothDevice.class, int.class, ParcelUuid.class});
	private static final Method createInsecureRfcommSocket = Util.getMethod(BluetoothDevice.class, "createInsecureRfcommSocket", new Class[] { int.class });
	static long nativeBta = 0;
	
	static BluetoothAdapter defaultAdapter(long nativeObj)
	{
		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		if(adapter == null)
		{
			//Log.i(logTag, "no bluetooth adapter found");
			return null;
		}
		nativeBta = nativeObj;
		return adapter;
	}
	
	static boolean startScan(Activity act, BluetoothAdapter adapter)
	{
		if(devs == null)
			devs = new ArrayList<BluetoothDevice>();
		devs.clear();
		try
		{
			if(adapter.isDiscovering())
				adapter.cancelDiscovery();
			act.registerReceiver(onDiscoveryFinished, new IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED));
			act.registerReceiver(onDeviceFound, new IntentFilter(BluetoothDevice.ACTION_FOUND));
	        return adapter.startDiscovery();
		}
		catch(Exception e)
		{
			return false;
		}
	}
	
	static void cancelScan(Activity act, BluetoothAdapter adapter)
	{
		act.unregisterReceiver(onDiscoveryFinished);
		act.unregisterReceiver(onDeviceFound);
		adapter.cancelDiscovery();
		devs.clear();
	}
	
	static BluetoothSocket openSocket(BluetoothAdapter adapter, String address, int ch, boolean l2cap)
	{
		BluetoothDevice dev = adapter.getRemoteDevice(address);
		if(dev == null)
			return null;
		BluetoothSocket socket = null;
		try
		{
			if(l2cap)
			{
				//Log.i(logTag, "creating l2cap");
				if(l2capInsecureSocketConstructor == null)
				{
					//Log.i(logTag, "missing l2cap methods");
					return null;
				}
				socket = (BluetoothSocket)l2capInsecureSocketConstructor.newInstance(TYPE_L2CAP, -1, false, false, dev, ch, null);
			}
			else
			{
				//Log.i(logTag, "creating rfcomm");
				if(createInsecureRfcommSocket == null)
				{
					//Log.i(logTag, "missing rfcomm methods");
					return null;
				}
				socket = (BluetoothSocket)createInsecureRfcommSocket.invoke(dev, ch);
				// doesn't appear to work, need to use private APIs
				//socket = dev.createRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));
			}
			//Log.i(logTag, "connecting to socket");
			socket.connect();
			//Log.i(logTag, "complete");
		}
		catch(java.lang.IllegalAccessException e)
		{
			//Log.i(logTag, "IllegalAccessException creating socket");
		}
		catch(java.lang.reflect.InvocationTargetException e)
		{
			//Log.i(logTag, "InvocationTargetException creating socket");
		}
		catch(java.lang.InstantiationException e)
		{
			//Log.i(logTag, "InstantiationException creating socket");
		}
		catch(java.io.IOException e)
		{
			//Log.i(logTag, "IOException creating socket");
			if(socket != null)
			{
				try { socket.close(); }
				catch(java.io.IOException closeE) { }
			}
			socket = null;
		}
		return socket;
	}

	private static BroadcastReceiver onDiscoveryFinished = new BroadcastReceiver()
	{
		@Override public void onReceive(Context context, Intent intent)
		{
			//Log.i(logTag, "discovery finished with " + devs.size() + " devs");
			context.unregisterReceiver(onDiscoveryFinished);
			context.unregisterReceiver(onDeviceFound);
			devs.clear();
			BaseActivity.onBTScanStatus(nativeBta, 1);
		}
	};
	
	private static BroadcastReceiver onDeviceFound = new BroadcastReceiver()
	{
		@Override public void onReceive(Context context, Intent intent)
		{
			if (BluetoothDevice.ACTION_FOUND.equals(intent.getAction()))
			{
				BluetoothDevice found = (BluetoothDevice)intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
				//Log.i(logTag, "found device " + found.getBluetoothClass().getDeviceClass() + " " + found.getName());
				if(found.getName() == null)
				{
					return;
				}
				for(BluetoothDevice d : devs)
				{
					if (d.getAddress().equals(found.getAddress()))
					{
						//Log.i(logTag, "device is duplicate");
						return;
					}
				}
				devs.add(found);
				if(BaseActivity.onScanDeviceClass(nativeBta, found.getBluetoothClass().getDeviceClass()))
				{
					BaseActivity.onScanDeviceName(nativeBta, found.getName(), found.getAddress());
				}
			}
		}
	};	
}