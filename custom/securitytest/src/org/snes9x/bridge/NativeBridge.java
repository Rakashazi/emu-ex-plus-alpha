// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge;
import android.os.*;
/** QA-only fixed data. No emulator library, game memory, input or files are accessed. */
final class NativeBridge {
  static long queued;
  static long[] info(){return new long[]{1,1,3,1,queued};}
  static byte[] read(boolean rom,int offset,int length,long[] metadata){
    int size=rom?3:0x20000;
    if(offset<0||length<1||length>0x20000||offset>size||length>size-offset)throw new IllegalArgumentException("RANGE");
    metadata[0]=1;metadata[1]=queued;return new byte[length];
  }
  static void queue(int[] masks,int[] frames){queued++;}
  static void command(int command){
    // Stall only this isolated service's main looper to exercise lease checks independently of its watchdog.
    if(command==1)new Handler(Looper.getMainLooper()).post(()->SystemClock.sleep(3000));
  }
}
