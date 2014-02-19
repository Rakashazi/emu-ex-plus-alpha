package com.bda.controller;

import android.os.Binder;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.Parcelable.Creator;
import android.os.RemoteException;

public abstract interface IControllerListener extends IInterface
{
  public abstract void onKeyEvent(KeyEvent paramKeyEvent)
    throws RemoteException;

  public abstract void onMotionEvent(MotionEvent paramMotionEvent)
    throws RemoteException;

  public abstract void onStateEvent(StateEvent paramStateEvent)
    throws RemoteException;

  public static abstract class Stub extends Binder
    implements IControllerListener
  {
    private static final String DESCRIPTOR = "com.bda.controller.IControllerListener";
    static final int TRANSACTION_onKeyEvent = 1;
    static final int TRANSACTION_onMotionEvent = 2;
    static final int TRANSACTION_onStateEvent = 3;

    public Stub()
    {
      attachInterface(this, "com.bda.controller.IControllerListener");
    }

    public static IControllerListener asInterface(IBinder obj)
    {
      if (obj == null) {
        return null;
      }
      IInterface iin = obj.queryLocalInterface("com.bda.controller.IControllerListener");
      if ((iin != null) && ((iin instanceof IControllerListener))) {
        return (IControllerListener)iin;
      }
      return new Proxy(obj);
    }

    public IBinder asBinder() {
      return this;
    }

    public boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {
      switch (code)
      {
      case 1598968902:
        reply.writeString("com.bda.controller.IControllerListener");
        return true;
      case 1:
      {
        data.enforceInterface("com.bda.controller.IControllerListener");
        KeyEvent _arg0;
        if (data.readInt() != 0) {
          _arg0 = (KeyEvent)KeyEvent.CREATOR.createFromParcel(data);
        }
        else {
          _arg0 = null;
        }
        onKeyEvent(_arg0);
        reply.writeNoException();
        return true;
      }
      case 2:
      {
        data.enforceInterface("com.bda.controller.IControllerListener");
        MotionEvent _arg0;
        if (data.readInt() != 0) {
          _arg0 = (MotionEvent)MotionEvent.CREATOR.createFromParcel(data);
        }
        else {
          _arg0 = null;
        }
        onMotionEvent(_arg0);
        reply.writeNoException();
        return true;
      }
      case 3:
      {
        data.enforceInterface("com.bda.controller.IControllerListener");
        StateEvent _arg0;
        if (data.readInt() != 0) {
          _arg0 = (StateEvent)StateEvent.CREATOR.createFromParcel(data);
        }
        else {
          _arg0 = null;
        }
        onStateEvent(_arg0);
        reply.writeNoException();
        return true;
      }
      }

      return super.onTransact(code, data, reply, flags);
    }

    private static class Proxy implements IControllerListener {
      private IBinder mRemote;

      Proxy(IBinder remote) {
        this.mRemote = remote;
      }

      public IBinder asBinder() {
        return this.mRemote;
      }

      public String getInterfaceDescriptor() {
        return "com.bda.controller.IControllerListener";
      }

      public void onKeyEvent(KeyEvent event) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerListener");
          if (event != null) {
            _data.writeInt(1);
            event.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          this.mRemote.transact(1, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void onMotionEvent(MotionEvent event) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerListener");
          if (event != null) {
            _data.writeInt(1);
            event.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          this.mRemote.transact(2, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void onStateEvent(StateEvent event) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerListener");
          if (event != null) {
            _data.writeInt(1);
            event.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          this.mRemote.transact(3, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
    }
  }
}
