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
	
	StorageManagerHelper() {}

	public static void enumVolumes(Activity act, long userData)
	{
		final StorageManager sm = (StorageManager)act.getSystemService(Context.STORAGE_SERVICE);
		final List<StorageVolume> volumes = sm.getStorageVolumes();
		for(final StorageVolume vol: volumes)
		{
			Parcel p = Parcel.obtain();
			vol.writeToParcel(p, 0);
			p.setDataPosition(0);
			String path;
			if(android.os.Build.VERSION.SDK_INT <= 27)
			{
				String id = p.readString();
				int storageID = p.readInt();
				path = p.readString();
			}
			else
			{
				String id = p.readString();
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
