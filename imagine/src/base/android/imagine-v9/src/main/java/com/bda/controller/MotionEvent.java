package com.bda.controller;

import android.os.Parcel;
import android.os.Parcelable;
import android.os.Parcelable.Creator;
import android.util.SparseArray;

public final class MotionEvent extends BaseEvent
  implements Parcelable
{
  public static final int AXIS_X = 0;
  public static final int AXIS_Y = 1;
  public static final int AXIS_Z = 11;
  public static final int AXIS_RZ = 14;
  public static final int AXIS_LTRIGGER = 17;
  public static final int AXIS_RTRIGGER = 18;
  public static final Parcelable.Creator<MotionEvent> CREATOR = new ParcelableCreator();
  final SparseArray<Float> mAxis;
  final SparseArray<Float> mPrecision;

  public MotionEvent(long eventTime, int deviceId, float x, float y, float z, float rz, float xPrecision, float yPrecision)
  {
    super(eventTime, deviceId);

    int axis = 4;
    this.mAxis = new SparseArray(4);
    this.mAxis.put(0, Float.valueOf(x));
    this.mAxis.put(1, Float.valueOf(y));
    this.mAxis.put(11, Float.valueOf(z));
    this.mAxis.put(14, Float.valueOf(rz));

    int precision = 2;
    this.mPrecision = new SparseArray(2);
    this.mPrecision.put(0, Float.valueOf(xPrecision));
    this.mPrecision.put(1, Float.valueOf(yPrecision));
  }

  public MotionEvent(long eventTime, int deviceId, int[] axisKey, float[] axisValue, int[] precisionKey, float[] precisionValue)
  {
    super(eventTime, deviceId);

    int axis = axisKey.length;
    this.mAxis = new SparseArray(axis);
    for (int index = 0; index < axis; index++)
    {
      this.mAxis.put(axisKey[index], Float.valueOf(axisValue[index]));
    }

    int precision = precisionKey.length;
    this.mPrecision = new SparseArray(precision);
    for (int index = 0; index < precision; index++)
    {
      this.mPrecision.put(precisionKey[index], Float.valueOf(precisionValue[index]));
    }
  }

  MotionEvent(Parcel parcel)
  {
    super(parcel);

    int axis = parcel.readInt();
    this.mAxis = new SparseArray(axis);
    for (int index = 0; index < axis; index++)
    {
      int key = parcel.readInt();
      float value = parcel.readFloat();
      this.mAxis.put(key, Float.valueOf(value));
    }

    int precision = parcel.readInt();
    this.mPrecision = new SparseArray(precision);
    for (int index = 0; index < axis; index++)
    {
      int key = parcel.readInt();
      float value = parcel.readFloat();
      this.mPrecision.put(key, Float.valueOf(value));
    }
  }

  public int describeContents()
  {
    return 0;
  }

  public final int findPointerIndex(int pointerId)
  {
    return -1;
  }

  public final float getAxisValue(int axis)
  {
    return getAxisValue(axis, 0);
  }

  public final float getAxisValue(int axis, int pointerIndex)
  {
    if (pointerIndex == 0)
    {
      return ((Float)this.mAxis.get(axis, Float.valueOf(0.0F))).floatValue();
    }
    return 0.0F;
  }

  public final int getPointerCount()
  {
    return 1;
  }

  public final int getPointerId(int pointerIndex)
  {
    return 0;
  }

  public final float getRawX()
  {
    return getX();
  }

  public final float getRawY()
  {
    return getY();
  }

  public final float getX()
  {
    return getAxisValue(0, 0);
  }

  public final float getX(int pointerIndex)
  {
    return getAxisValue(0, pointerIndex);
  }

  public final float getXPrecision()
  {
    return ((Float)this.mPrecision.get(0, Float.valueOf(0.0F))).floatValue();
  }

  public final float getY()
  {
    return getAxisValue(1, 0);
  }

  public final float getY(int pointerIndex)
  {
    return getAxisValue(1, pointerIndex);
  }

  public final float getYPrecision()
  {
    return ((Float)this.mPrecision.get(1, Float.valueOf(0.0F))).floatValue();
  }

  public void writeToParcel(Parcel parcel, int flags)
  {
    super.writeToParcel(parcel, flags);

    int axis = this.mAxis.size();
    parcel.writeInt(axis);
    for (int index = 0; index < axis; index++)
    {
      parcel.writeInt(this.mAxis.keyAt(index));
      parcel.writeFloat(((Float)this.mAxis.valueAt(index)).floatValue());
    }

    int precision = this.mPrecision.size();
    parcel.writeInt(precision);
    for (int index = 0; index < precision; index++)
    {
      parcel.writeInt(this.mPrecision.keyAt(index));
      parcel.writeFloat(((Float)this.mPrecision.valueAt(index)).floatValue());
    }
  }

  static class ParcelableCreator
    implements Parcelable.Creator<MotionEvent>
  {
    public MotionEvent createFromParcel(Parcel source)
    {
      return new MotionEvent(source);
    }

    public MotionEvent[] newArray(int size)
    {
      return new MotionEvent[size];
    }
  }
}