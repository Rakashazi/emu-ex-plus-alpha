package com.bda.controller;

import android.os.Binder;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.RemoteException;

public abstract interface IControllerService extends IInterface
{
  public abstract void registerListener(IControllerListener paramIControllerListener, int paramInt)
    throws RemoteException;

  public abstract void unregisterListener(IControllerListener paramIControllerListener, int paramInt)
    throws RemoteException;

  public abstract void registerMonitor(IControllerMonitor paramIControllerMonitor, int paramInt)
    throws RemoteException;

  public abstract void unregisterMonitor(IControllerMonitor paramIControllerMonitor, int paramInt)
    throws RemoteException;

  public abstract int getInfo(int paramInt)
    throws RemoteException;

  public abstract int getKeyCode(int paramInt1, int paramInt2)
    throws RemoteException;

  public abstract float getAxisValue(int paramInt1, int paramInt2)
    throws RemoteException;

  public abstract int getState(int paramInt1, int paramInt2)
    throws RemoteException;

  public abstract void sendMessage(int paramInt1, int paramInt2)
    throws RemoteException;

  public abstract void registerListener2(IControllerListener paramIControllerListener, int paramInt)
    throws RemoteException;

  public abstract int getKeyCode2(int paramInt1, int paramInt2)
    throws RemoteException;

  public abstract void allowNewConnections()
    throws RemoteException;

  public abstract void disallowNewConnections()
    throws RemoteException;

  public abstract boolean isAllowingNewConnections()
    throws RemoteException;

  public static abstract class Stub extends Binder
    implements IControllerService
  {
    private static final String DESCRIPTOR = "com.bda.controller.IControllerService";
    static final int TRANSACTION_registerListener = 1;
    static final int TRANSACTION_unregisterListener = 2;
    static final int TRANSACTION_registerMonitor = 3;
    static final int TRANSACTION_unregisterMonitor = 4;
    static final int TRANSACTION_getInfo = 5;
    static final int TRANSACTION_getKeyCode = 6;
    static final int TRANSACTION_getAxisValue = 7;
    static final int TRANSACTION_getState = 8;
    static final int TRANSACTION_sendMessage = 9;
    static final int TRANSACTION_registerListener2 = 10;
    static final int TRANSACTION_getKeyCode2 = 11;
    static final int TRANSACTION_allowNewConnections = 12;
    static final int TRANSACTION_disallowNewConnections = 13;
    static final int TRANSACTION_isAllowingNewConnections = 14;

    public Stub()
    {
      attachInterface(this, "com.bda.controller.IControllerService");
    }

    public static IControllerService asInterface(IBinder obj)
    {
      if (obj == null) {
        return null;
      }
      IInterface iin = obj.queryLocalInterface("com.bda.controller.IControllerService");
      if ((iin != null) && ((iin instanceof IControllerService))) {
        return (IControllerService)iin;
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
        reply.writeString("com.bda.controller.IControllerService");
        return true;
      case 1:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        IControllerListener _arg0 = IControllerListener.Stub.asInterface(data.readStrongBinder());

        int _arg1 = data.readInt();
        registerListener(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 2:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        IControllerListener _arg0 = IControllerListener.Stub.asInterface(data.readStrongBinder());

        int _arg1 = data.readInt();
        unregisterListener(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 3:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        IControllerMonitor _arg0 = IControllerMonitor.Stub.asInterface(data.readStrongBinder());

        int _arg1 = data.readInt();
        registerMonitor(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 4:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        IControllerMonitor _arg0 = IControllerMonitor.Stub.asInterface(data.readStrongBinder());

        int _arg1 = data.readInt();
        unregisterMonitor(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 5:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();
        int _result = getInfo(_arg0);
        reply.writeNoException();
        reply.writeInt(_result);
        return true;
      }
      case 6:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();
        int _result = getKeyCode(_arg0, _arg1);
        reply.writeNoException();
        reply.writeInt(_result);
        return true;
      }
      case 7:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();
        float _result = getAxisValue(_arg0, _arg1);
        reply.writeNoException();
        reply.writeFloat(_result);
        return true;
      }
      case 8:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();
        int _result = getState(_arg0, _arg1);
        reply.writeNoException();
        reply.writeInt(_result);
        return true;
      }
      case 9:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();
        sendMessage(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 10:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        IControllerListener _arg0 = IControllerListener.Stub.asInterface(data.readStrongBinder());

        int _arg1 = data.readInt();
        registerListener2(_arg0, _arg1);
        reply.writeNoException();
        return true;
      }
      case 11:
      {
        data.enforceInterface("com.bda.controller.IControllerService");

        int _arg0 = data.readInt();

        int _arg1 = data.readInt();
        int _result = getKeyCode2(_arg0, _arg1);
        reply.writeNoException();
        reply.writeInt(_result);
        return true;
      }
      case 12:
        data.enforceInterface("com.bda.controller.IControllerService");
        allowNewConnections();
        reply.writeNoException();
        return true;
      case 13:
        data.enforceInterface("com.bda.controller.IControllerService");
        disallowNewConnections();
        reply.writeNoException();
        return true;
      case 14:
      {
        data.enforceInterface("com.bda.controller.IControllerService");
        boolean _result = isAllowingNewConnections();
        reply.writeNoException();
        reply.writeInt(_result ? 1 : 0);
        return true;
      }
      }

      return super.onTransact(code, data, reply, flags);
    }

    private static class Proxy implements IControllerService {
      private IBinder mRemote;

      Proxy(IBinder remote) {
        this.mRemote = remote;
      }

      public IBinder asBinder() {
        return this.mRemote;
      }

      public String getInterfaceDescriptor() {
        return "com.bda.controller.IControllerService";
      }

      public void registerListener(IControllerListener callback, int param) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeStrongBinder(callback != null ? callback.asBinder() : null);
          _data.writeInt(param);
          this.mRemote.transact(1, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void unregisterListener(IControllerListener callback, int param) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeStrongBinder(callback != null ? callback.asBinder() : null);
          _data.writeInt(param);
          this.mRemote.transact(2, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void registerMonitor(IControllerMonitor callback, int param) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeStrongBinder(callback != null ? callback.asBinder() : null);
          _data.writeInt(param);
          this.mRemote.transact(3, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void unregisterMonitor(IControllerMonitor callback, int param) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeStrongBinder(callback != null ? callback.asBinder() : null);
          _data.writeInt(param);
          this.mRemote.transact(4, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public int getInfo(int info) throws RemoteException { Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        int _result;
        try { _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(info);
          this.mRemote.transact(5, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result; } 
      public int getKeyCode(int id, int keyCode) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(id);
          _data.writeInt(keyCode);
          this.mRemote.transact(6, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      public float getAxisValue(int id, int axis) throws RemoteException { Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        float _result;
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(id);
          _data.writeInt(axis);
          this.mRemote.transact(7, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readFloat();
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result; } 
      public int getState(int id, int state) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(id);
          _data.writeInt(state);
          this.mRemote.transact(8, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }

      public void sendMessage(int msg, int param) throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(msg);
          _data.writeInt(param);
          this.mRemote.transact(9, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void registerListener2(IControllerListener callback, int param) throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeStrongBinder(callback != null ? callback.asBinder() : null);
          _data.writeInt(param);
          this.mRemote.transact(10, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public int getKeyCode2(int id, int keyCode) throws RemoteException { Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        int _result;
        try { _data.writeInterfaceToken("com.bda.controller.IControllerService");
          _data.writeInt(id);
          _data.writeInt(keyCode);
          this.mRemote.transact(11, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result; }

      public void allowNewConnections()
        throws RemoteException
      {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          this.mRemote.transact(12, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public void disallowNewConnections() throws RemoteException {
        Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        try {
          _data.writeInterfaceToken("com.bda.controller.IControllerService");
          this.mRemote.transact(13, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }

      public boolean isAllowingNewConnections() throws RemoteException { Parcel _data = Parcel.obtain();
        Parcel _reply = Parcel.obtain();
        boolean _result;
        try { _data.writeInterfaceToken("com.bda.controller.IControllerService");
          this.mRemote.transact(14, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt() != 0;
        }
        finally
        {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
    }
  }
}
