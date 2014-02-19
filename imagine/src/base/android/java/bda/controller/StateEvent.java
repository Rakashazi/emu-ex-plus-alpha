package com.bda.controller;

import android.os.Parcel;
import android.os.Parcelable;
import android.os.Parcelable.Creator;

public class StateEvent extends BaseEvent
  implements Parcelable
{
  public static final int STATE_UNKNOWN = 0;
  public static final int STATE_CONNECTION = 1;
  public static final int STATE_POWER_LOW = 2;

  @Deprecated
  public static final int STATE_SUPPORTED_VERSION = 3;

  @Deprecated
  public static final int STATE_SELECTED_VERSION = 4;
  public static final int STATE_SUPPORTED_PRODUCT_VERSION = 3;
  public static final int STATE_CURRENT_PRODUCT_VERSION = 4;
  public static final int ACTION_VERSION_MOGA = 0;
  public static final int ACTION_VERSION_MOGAPRO = 1;
  public static final int ACTION_FALSE = 0;
  public static final int ACTION_TRUE = 1;
  public static final int ACTION_DISCONNECTED = 0;
  public static final int ACTION_CONNECTED = 1;
  public static final int ACTION_CONNECTING = 2;
  final int mState;
  final int mAction;
  public static final Parcelable.Creator<StateEvent> CREATOR = new ParcelableCreator();

  public StateEvent(long eventTime, int deviceId, int state, int action)
  {
    super(eventTime, deviceId);
    this.mState = state;
    this.mAction = action;
  }

  StateEvent(Parcel parcel)
  {
    super(parcel);
    this.mState = parcel.readInt();
    this.mAction = parcel.readInt();
  }

  public int describeContents()
  {
    return 0;
  }

  public final int getAction()
  {
    return this.mAction;
  }

  public final int getState()
  {
    return this.mState;
  }

  public void writeToParcel(Parcel parcel, int flags)
  {
    super.writeToParcel(parcel, flags);
    parcel.writeInt(this.mState);
    parcel.writeInt(this.mAction);
  }

  static class ParcelableCreator
    implements Parcelable.Creator<StateEvent>
  {
    public StateEvent createFromParcel(Parcel source)
    {
      return new StateEvent(source);
    }

    public StateEvent[] newArray(int size)
    {
      return new StateEvent[size];
    }
  }
}