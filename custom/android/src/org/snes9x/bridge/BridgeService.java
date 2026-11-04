// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge;

import android.app.Service;
import android.content.Intent;
import android.os.*;

/** Local, versioned Binder protocol. No sockets, host pointers or writable memory. */
public final class BridgeService extends Service {
  public static final String DESCRIPTOR = "org.snes9x.bridge.v1";
  public static final String READ = "org.snes9x.bridge.permission.READ";
  public static final String CONTROL = "org.snes9x.bridge.permission.CONTROL";
  private IBinder owner;
  private int ownerUid = -1;
  private IBinder.DeathRecipient death;
  private long touched;
  private static final int MAX_REQUEST_BYTES = 8192;
  private static final long LEASE_MS = 2000;
  private final Handler handler = new Handler(Looper.getMainLooper());
  private synchronized void release() {
    NativeBridge.command(0);
    if (owner != null && death != null)
      owner.unlinkToDeath(death, 0);
    owner = null;
    ownerUid = -1;
    death = null;
  }
  private synchronized void controller() {
    enforceCallingPermission(CONTROL, "Separate controller permission required");
    expire();
    if (owner == null || ownerUid != Binder.getCallingUid())
      throw new SecurityException("NO_CONTROL_SESSION");
    touched = SystemClock.elapsedRealtime();
  }
  private void expire() {
    if (owner != null && SystemClock.elapsedRealtime() - touched >= LEASE_MS)
      release();
  }
  private static void type(Bundle request, String name, Class<?> expected) {
    if (request.containsKey(name) && !expected.isInstance(request.get(name)))
      throw new IllegalArgumentException("TYPE: " + name);
  }
  private final Binder binder = new Binder() {
    @Override
    protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
        throws RemoteException {
      if (code == INTERFACE_TRANSACTION) {
        reply.writeString(DESCRIPTOR);
        return true;
      }
      if (code != FIRST_CALL_TRANSACTION)
        return super.onTransact(code, data, reply, flags);
      // v1 is synchronous: do not execute a command for a request with no result channel.
      if ((flags & FLAG_ONEWAY) != 0 || reply == null)
        return false;
      data.enforceInterface(DESCRIPTOR);
      enforceCallingPermission(READ, "Memory reading must be authorized");
      if (data.dataAvail() > MAX_REQUEST_BYTES)
        throw new IllegalArgumentException("REQUEST_SIZE");
      Bundle request = data.readBundle(getClassLoader());
      if (data.dataAvail() != 0)
        throw new IllegalArgumentException("TRAILING_DATA");
      if (request != null) {
        type(request, "version", Integer.class);
        type(request, "op", String.class);
      }
      if (request == null || request.getInt("version", 0) != 1)
        throw new IllegalArgumentException("VERSION");
      String operation = request.getString("op", "");
      if (operation.length() > 16)
        throw new IllegalArgumentException("OPERATION");
      Bundle result = new Bundle();
      result.putInt("version", 1);
      synchronized (BridgeService.this) {
        switch (operation) {
          case "info": {
            long[] info = NativeBridge.info();
            result.putBoolean("loaded", info[0] != 0);
            result.putBoolean("ready", info[1] != 0);
            result.putInt("romSize", (int) info[2]);
            result.putLong("generation", info[3]);
            result.putLong("frame", info[4]);
            result.putInt("wramSize", 0x20000);
            result.putInt("maxRead", 0x20000);
            break;
          }
          case "read": {
            type(request, "region", String.class);
            type(request, "offset", Integer.class);
            type(request, "length", Integer.class);
            String region = request.getString("region", "wram");
            if (!region.equals("wram") && !region.equals("rom"))
              throw new IllegalArgumentException("REGION");
            long[] meta = new long[2];
            result.putByteArray("bytes",
                NativeBridge.read(region.equals("rom"), request.getInt("offset", -1),
                    request.getInt("length", 0), meta));
            result.putLong("generation", meta[0]);
            result.putLong("frame", meta[1]);
            break;
          }
          case "acquire": {
            enforceCallingPermission(CONTROL, "Separate controller permission required");
            expire();
            if (owner != null)
              throw new IllegalStateException("CONTROL_BUSY");
            IBinder token = request.getBinder("token");
            if (token == null)
              throw new IllegalArgumentException("TOKEN");
            final IBinder captured = token;
            IBinder.DeathRecipient recipient = () -> {
              synchronized (BridgeService.this) {
                if (owner == captured)
                  release();
              }
            };
            token.linkToDeath(recipient, 0);
            owner = token;
            death = recipient;
            ownerUid = Binder.getCallingUid();
            touched = SystemClock.elapsedRealtime();
            break;
          }
          case "queue":
            controller();
            type(request, "masks", int[].class);
            type(request, "frames", int[].class);
            NativeBridge.queue(request.getIntArray("masks"), request.getIntArray("frames"));
            break;
          case "cancel":
            controller();
            NativeBridge.command(0);
            break;
          case "pause":
            controller();
            NativeBridge.command(1);
            break;
          case "resume":
            controller();
            NativeBridge.command(2);
            break;
          case "heartbeat":
            controller();
            NativeBridge.command(3);
            break;
          case "release":
            controller();
            release();
            break;
          default:
            throw new IllegalArgumentException("OPERATION");
        }
      }
      reply.writeNoException();
      reply.writeBundle(result);
      return true;
    }
  };
  private final Runnable watchdog = new Runnable() {
    @Override
    public void run() {
      synchronized (BridgeService.this) {
        expire();
      }
      handler.postDelayed(this, 250);
    }
  };
  @Override
  public void onCreate() {
    super.onCreate();
    handler.post(watchdog);
  }
  @Override
  public IBinder onBind(Intent intent) {
    return binder;
  }
  @Override
  public void onDestroy() {
    handler.removeCallbacks(watchdog);
    release();
    super.onDestroy();
  }
}
