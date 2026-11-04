// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge.test;
import android.app.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.os.*;
import android.widget.*;
import java.util.concurrent.*;

/** External APK for permission, boundary and frame-read acceptance on a real device. */
public final class MainActivity extends Activity {
  private static final String READ = "org.snes9x.bridge.permission.READ",
                              CONTROL = "org.snes9x.bridge.permission.CONTROL";
  private IBinder remote;
  private final Binder token = new Binder();
  private TextView status;
  private final ExecutorService worker = Executors.newSingleThreadExecutor();
  private final Handler handler = new Handler(Looper.getMainLooper());
  private boolean bound;
  private Bundle call(Bundle request) throws RemoteException {
    request.putInt("version", 1);
    Parcel data = Parcel.obtain(), reply = Parcel.obtain();
    try {
      data.writeInterfaceToken("org.snes9x.bridge.v1");
      data.writeBundle(request);
      if (remote == null || !remote.transact(IBinder.FIRST_CALL_TRANSACTION, data, reply, 0))
        throw new RemoteException("Service unavailable");
      reply.readException();
      return reply.readBundle(getClassLoader());
    } finally {
      data.recycle();
      reply.recycle();
    }
  }
  private void show(String text) {
    runOnUiThread(() -> status.setText(text));
  }
  private void run(String op) {
    worker.execute(() -> {
      try {
        Bundle r = new Bundle();
        r.putString("op", op);
        if (op.equals("read")) {
          r.putInt("offset", 0);
          r.putInt("length", 0x20000);
        }
        if (op.equals("acquire"))
          r.putBinder("token", token);
        if (op.equals("queue")) {
          r.putIntArray("masks", new int[] {0x80, 0, 0x8000, 0});
          r.putIntArray("frames", new int[] {2, 2, 3, 1});
        }
        Bundle result = call(r);
        byte[] bytes = result.getByteArray("bytes");
        show(op + ": OK\nGeneration=" + result.getLong("generation") + " Frame="
            + result.getLong("frame") + (bytes == null ? "" : " Bytes=" + bytes.length));
      } catch (Exception e) {
        show(op + ": " + e);
      }
    });
  }
  private final ServiceConnection connection = new ServiceConnection() {
    public void onServiceConnected(ComponentName n, IBinder b) {
      remote = b;
      show("Verbunden. Lesen benötigt keine Steuerrechte.");
    }
    public void onServiceDisconnected(ComponentName n) {
      remote = null;
      show("Verbindung verloren");
    }
  };
  private final Runnable heartbeat = new Runnable() {
    public void run() {
      if (remote != null)
        worker.execute(() -> {
          try {
            Bundle r = new Bundle();
            r.putString("op", "heartbeat");
            call(r);
          } catch (Exception ignored) {
          }
        });
      handler.postDelayed(this, 500);
    }
  };
  @Override
  public void onCreate(Bundle state) {
    super.onCreate(state);
    LinearLayout layout = new LinearLayout(this);
    layout.setOrientation(1);
    layout.setPadding(24, 24, 24, 24);
    status = new TextView(this);
    layout.addView(status);
    String[] labels = {"Leserecht anfragen", "Verbinden", "WRAM lesen",
        "Steuerrecht ausdrücklich anfragen", "Steuerung übernehmen", "Sequenz: A 2 / B 3 Frames",
        "Pause", "Fortsetzen", "Abbrechen", "Freigeben"};
    String[] ops = {"readPermission", "connect", "read", "controlPermission", "acquire", "queue",
        "pause", "resume", "cancel", "release"};
    for (int i = 0; i < labels.length; i++) {
      final String op = ops[i];
      Button b = new Button(this);
      b.setText(labels[i]);
      b.setOnClickListener(v -> {
        if (op.endsWith("Permission")) {
          requestPermissions(new String[] {op.equals("readPermission") ? READ : CONTROL}, 1);
        } else if (op.equals("connect")) {
          if (!bound)
            try {
              bound = bindService(new Intent().setComponent(new ComponentName(
                                      "org.snes9x.custom", "org.snes9x.bridge.BridgeService")),
                  connection, BIND_AUTO_CREATE);
              show("Bind: " + bound);
            } catch (Exception e) {
              show(e.toString());
            }
        } else
          run(op);
      });
      layout.addView(b);
    }
    ScrollView scroll = new ScrollView(this);
    scroll.addView(layout);
    setContentView(scroll);
    handler.post(heartbeat);
  }
  @Override
  public void onDestroy() {
    handler.removeCallbacks(heartbeat);
    if (bound)
      unbindService(connection);
    worker.shutdownNow();
    super.onDestroy();
  }
}
