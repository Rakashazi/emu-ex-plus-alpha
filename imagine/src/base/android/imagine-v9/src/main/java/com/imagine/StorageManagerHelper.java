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
import android.util.*;
import android.content.*;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.Parcel;
import java.util.List;

final class StorageManagerHelper
{
	private static final String logTag = "StorageManagerHelper";
	private static native void volumeEnumerated(long userData, String path, String name);
	private static int pathPosition = -1;
	
	StorageManagerHelper() {}

	public static void enumVolumes(Activity act, long userData)
	{
		if(pathPosition == -2)
		{
			// failed to find path postion in previous attempt
			return;
		}
		final StorageManager sm = (StorageManager)act.getSystemService(Context.STORAGE_SERVICE);
		final List<StorageVolume> volumes = sm.getStorageVolumes();
		for(final StorageVolume vol: volumes)
		{
			if(vol.isPrimary())
				continue;
			Parcel p = Parcel.obtain();
			vol.writeToParcel(p, 0);
			String path = null;
			if(pathPosition < 0)
			{
				// search for the mount path string in the parcel
				p.setDataPosition(0);
				while(p.dataPosition() < p.dataSize())
				{
					int pos = p.dataPosition();
					final String s = p.readString();
					if(s != null && s.startsWith("/storage"))
					{
						//Log.v(logTag, "found path string:" + s + " @ position:" + pos + ")");
						pathPosition = pos;
						path = s;
						break;
					}
				}
				if(pathPosition < 0)
				{
					//Log.v(logTag, "unable to find path string in parcel, volume enumeration not possible");
					pathPosition = -2;
					p.recycle();
					return;
				}
			}
			else
			{
				p.setDataPosition(pathPosition);
				path = p.readString();
			}
			p.recycle();
			if(path == null || path.indexOf('/') != 0)
				continue;
			//Log.v(logTag, vol.getDescription(act) + ":" + path);
			volumeEnumerated(userData, vol.getDescription(act), path);
		}
	}
}
