// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge;
final class NativeBridge {
  static {
    System.loadLibrary("main");
  }
  static native long[] info();
  static native byte[] read(boolean rom, int offset, int length, long[] metadata);
  static native void queue(int[] masks, int[] frames);
  static native void command(int command);
}
