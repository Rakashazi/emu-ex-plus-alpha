package com.bda.controller;

import android.os.Binder;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.RemoteException;

public abstract interface IControllerMonitor extends IInterface
{
  public abstract void onLog(int paramInt1, int paramInt2, String paramString)
    throws RemoteException;

  public static abstract class Stub extends Binder
    implements IControllerMonitor
  {
    private static final String DESCRIPTOR = "com.bda.controller.IControllerMonitor";
    static final int TRANSACTION_onLog = 1;

    public Stub()
    {
      attachInterface(this, "com.bda.controller.IControllerMonitor");
    }

    public static IControllerMonitor asInterface(IBinder obj)
    {
      if (obj == null) {
        return null;
      }
      IInterface iin = obj.queryLocalInterface("com.bda.controller.IControllerMonitor");
      if ((iin != null) && ((iin instanceof IControllerMonitor))) {
        return (IControllerMonitor)iin;
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
        reply.writeString("com.bda.controller.IControllerMonitor");
        return true;
      case 1:
        data.enforceInterface("com.bda.controller.IControllerMonitor");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();

        String _arg2 = data.readString();
        onLog(_arg0, _arg1, _arg2);
        reply.writeNoException();
        return true;
      }

      return super.onTransact(code, data, reply, flags);
    }

    private static class Proxy implements IControllerMonitor {
      private IBinder mRemote;

      Proxy(IBinder remote) {
        this.mRemote = remote;
      }

      public IBinder asBinder() {
        return this.mRemote;
      }

      public String getInterfaceDescriptor() {
        return "com.bda.controller.IControllerMonitor";
      }

      public void onLog(int level, int id, String message) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerMonitor");
          _data.writeInt(level);
          _data.writeInt(id);
          _data.writeString(message);
          this.mRemote.transact(1, _data, _reply, 0);
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