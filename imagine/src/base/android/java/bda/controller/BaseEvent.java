package com.bda.controller;

import android.os.Parcel;
import android.os.Parcelable;
import android.os.Parcelable.Creator;

class BaseEvent
  implements Parcelable
{
  final long mEventTime;
  final int mControllerId;
  public static final Parcelable.Creator<BaseEvent> CREATOR = new ParcelableCreator();

  public BaseEvent(long eventTime, int deviceId)
  {
    this.mEventTime = eventTime;
    this.mControllerId = deviceId;
  }

  BaseEvent(Parcel parcel)
  {
    this.mEventTime = parcel.readLong();
    this.mControllerId = parcel.readInt();
  }

  public int describeContents()
  {
    return 0;
  }

  public final int getControllerId()
  {
    return this.mControllerId;
  }

  public final long getEventTime()
  {
    return this.mEventTime;
  }

  public void writeToParcel(Parcel parcel, int flags)
  {
    parcel.writeLong(this.mEventTime);
    parcel.writeInt(this.mControllerId);
  }

  static class ParcelableCreator
    implements Parcelable.Creator<BaseEvent>
  {
    public BaseEvent createFromParcel(Parcel source)
    {
      return new BaseEvent(source);
    }

    public BaseEvent[] newArray(int size)
    {
      return new BaseEvent[size];
    }
  }
}