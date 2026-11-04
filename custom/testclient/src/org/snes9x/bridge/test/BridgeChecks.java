// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge.test;
import android.app.Instrumentation;
import android.content.*;
import android.os.*;
import java.util.*;
import java.util.concurrent.*;

/** Runs in a separate APK/UID, leaving the emulator Activity in the foreground. */
public final class BridgeChecks extends Instrumentation {
  private IBinder remote;
  private final Binder token = new Binder();
  private String suite;
  @Override
  public void onCreate(Bundle args) {
    suite = args == null ? "read" : args.getString("suite", "read");
    start();
  }
  private Bundle call(String op, Bundle args) throws Exception {
    args.putInt("version", 1);
    args.putString("op", op);
    Parcel in = Parcel.obtain(), out = Parcel.obtain();
    try {
      in.writeInterfaceToken("org.snes9x.bridge.v1");
      in.writeBundle(args);
      if (!remote.transact(1, in, out, 0))
        throw new AssertionError("transact false");
      out.readException();
      return out.readBundle(getClass().getClassLoader());
    } finally {
      in.recycle();
      out.recycle();
    }
  }
  private Bundle call(String op) throws Exception {
    return call(op, new Bundle());
  }
  private Bundle read(String region, int offset, int length) throws Exception {
    Bundle b = new Bundle();
    b.putString("region", region);
    b.putInt("offset", offset);
    b.putInt("length", length);
    return call("read", b);
  }
  private void check(boolean v, String reason) {
    if (!v)
      throw new AssertionError(reason);
  }
  private void rejects(String region, int offset, int length) throws Exception {
    boolean failed = false;
    try {
      read(region, offset, length);
    } catch (IllegalArgumentException expected) {
      failed = true;
    }
    check(failed, "range accepted " + offset + ":" + length);
  }
  @Override
  public void onStart() {
    Bundle result = new Bundle();
    CountDownLatch latch = new CountDownLatch(1);
    ServiceConnection connection = new ServiceConnection() {
      public void onServiceConnected(ComponentName n, IBinder b) {
        remote = b;
        latch.countDown();
      }
      public void onServiceDisconnected(ComponentName n) {
        remote = null;
      }
    };
    boolean bound = false;
    try {
      bound = getTargetContext().bindService(
          new Intent().setComponent(
              new ComponentName("org.snes9x.custom", "org.snes9x.bridge.BridgeService")),
          connection, Context.BIND_AUTO_CREATE);
      check(bound && latch.await(5, TimeUnit.SECONDS), "bind timeout");
      Bundle info = call("info");
      check(info.getInt("version") == 1, "version");
      if (suite.equals("unauthorized")) {
        throw new AssertionError("Unauthorized call accepted");
      }
      for (int wait=0; wait<100 && !info.getBoolean("ready"); wait++) {
        Thread.sleep(50); info=call("info");
      }
      Bundle full = read("wram", 0, 0x20000);
      check(full.getByteArray("bytes").length == 0x20000, "WRAM length");
      rejects("wram", -1, 1);
      rejects("wram", 0, 0);
      rejects("wram", 0x1ffff, 2);
      rejects("rom", info.getInt("romSize"), 1);
      byte[] rom = read("rom", 0, Math.min(0x20000, info.getInt("romSize"))).getByteArray("bytes");
      check(rom.length > 0, "ROM empty");
      if(suite.equals("capture")) {
        try(java.io.FileOutputStream out=new java.io.FileOutputStream(new java.io.File(getTargetContext().getFilesDir(),"capture-wram.bin"))){out.write(full.getByteArray("bytes"));}
      }
      if (suite.equals("readonly")) {
        boolean denied = false;
        try {
          Bundle b = new Bundle();
          b.putBinder("token", token);
          call("acquire", b);
        } catch (SecurityException expected) {
          denied = true;
        }
        check(denied, "READ granted control rights");
      }
      if (suite.equals("controller") || suite.equals("manual")) {
        byte[] ram = full.getByteArray("bytes");
        check(ram[0x10] == 'B' && ram[0x11] == 'R' && ram[0x12] == 'D' && ram[0x13] == 'G',
            "Load original bridge-input-test.sfc first");
        Bundle acquire = new Bundle();
        acquire.putBinder("token", token);
        call("acquire", acquire);
        Bundle q = new Bundle();
        q.putIntArray("masks", suite.equals("manual")?new int[]{0x80}:new int[] {0x80, 0, 0x8000, 0});
        q.putIntArray("frames", suite.equals("manual")?new int[]{600}:new int[] {7, 3, 11, 4});
        call("queue", q);
        if(suite.equals("manual")){Bundle stage=new Bundle();stage.putString("stage","READY_MANUAL_INPUT");sendStatus(1,stage);}
        for (int i = 0; i < (suite.equals("manual")?80:10); i++) {
          Thread.sleep(100);
          call("heartbeat");
        }
        ram = read("wram", 0, 0x20000).getByteArray("bytes");
        int count = u16(ram, 0);
        List<String> runs = new ArrayList<>();
        int previous = -1, run = 0;
        for (int i = Math.max(0, count - (suite.equals("manual")?500:90)); i < count; i++) {
          int mask = u16(ram, 0x200 + (i & 511) * 2);
          if (mask == previous)
            run++;
          else {
            if (previous > 0)
              runs.add(Integer.toHexString(previous) + ":" + run);
            previous = mask;
            run = 1;
          }
        }
        if (previous > 0)
          runs.add(Integer.toHexString(previous) + ":" + run);
        if(suite.equals("manual")){
          boolean manualSeen=false;for(String item:runs)if(item.startsWith("200:"))manualSeen=true;
          check(manualSeen,"Manual Left input was not sampled: "+runs);
          for(int i=Math.max(0,count-20);i<count;i++)check((u16(ram,0x200+(i&511)*2)&0x80)==0,"Automation resumed after manual input");
        } else check(runs.contains("80:7") && runs.contains("8000:11"),
            "Core pad duration mismatch " + runs + " NMI="+count+" pad="+u16(ram,2)+" beforeFrame="+full.getLong("frame")+" afterFrame="+call("info").getLong("frame"));
        call("release");
        result.putString("controllerRuns", runs.toString());
      }
      result.putString("result",
          "PASS " + suite + " generation=" + full.getLong("generation")
              + " frame=" + full.getLong("frame"));
      finish(-1, result);
    } catch (Throwable error) {
      if (suite.equals("unauthorized") && error instanceof SecurityException) {
        result.putString("result", "PASS unauthorized denied");
        finish(-1, result);
      } else {
        result.putString("result", "FAIL " + error);
        finish(0, result);
      }
    } finally {
      if (bound)
        getTargetContext().unbindService(connection);
    }
  }
  private static int u16(byte[] b, int p) {
    return (b[p] & 255) | ((b[p + 1] & 255) << 8);
  }
}
