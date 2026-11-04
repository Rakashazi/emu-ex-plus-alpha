// SPDX-License-Identifier: GPL-3.0-or-later
package org.snes9x.bridge;
import android.app.*;
import android.content.*;
import android.os.*;
import java.util.concurrent.*;

/** Headless actual-Parcel tests of the production Java service in an isolated process. */
public final class SecurityChecks extends Instrumentation {
  IBinder remote;boolean baseline;String suite;StringBuilder log=new StringBuilder();
  @Override public void onCreate(Bundle args){baseline=args!=null&&Boolean.parseBoolean(args.getString("baseline","false"));suite=args==null?"hardened":args.getString("suite","hardened");start();}
  Bundle request(String op){Bundle b=new Bundle();b.putInt("version",1);b.putString("op",op);return b;}
  Bundle call(Bundle b,boolean trailing)throws Exception{
    Parcel in=Parcel.obtain(),out=Parcel.obtain();try{
      in.writeInterfaceToken(BridgeService.DESCRIPTOR);in.writeBundle(b);if(trailing)in.writeInt(123);
      require(remote.transact(1,in,out,0),"Transaction rejected");out.readException();return out.readBundle(getClass().getClassLoader());
    }finally{in.recycle();out.recycle();}
  }
  Bundle call(String op)throws Exception{return call(request(op),false);}
  interface Action{void run()throws Exception;}
  void rejects(Action action,String name)throws Exception{
    boolean rejected=false;try{action.run();}catch(IllegalArgumentException|SecurityException expected){rejected=true;}
    if(baseline)log.append("BASELINE ").append(name).append(rejected?": rejected\n":": accepted\n");
    else {require(rejected,name+" accepted");log.append("PASS ").append(name).append(" rejected\n");}
  }
  void require(boolean ok,String message){if(!ok)throw new AssertionError(message);}
  @Override public void onStart(){
    Bundle result=new Bundle();CountDownLatch ready=new CountDownLatch(1);
    ServiceConnection connection=new ServiceConnection(){public void onServiceConnected(ComponentName n,IBinder b){remote=b;ready.countDown();}public void onServiceDisconnected(ComponentName n){remote=null;}};
    boolean bound=false;
    try{
      Context context=getTargetContext();require(context.getPackageName().equals("org.snes9x.bridge.securitytest"),"Isolated package only");
      bound=context.bindService(new Intent().setComponent(new ComponentName(context.getPackageName(),BridgeService.class.getName())),connection,Context.BIND_AUTO_CREATE);
      require(bound&&ready.await(5,TimeUnit.SECONDS),"Bind timeout");
      if(suite.equals("unauthorized")){
        boolean denied=false;try{call("info");}catch(SecurityException expected){denied=true;}require(denied,"Unauthorized READ accepted");result.putString("stream","PASS actual Binder READ permission enforced in isolated process\n");finish(Activity.RESULT_OK,result);return;
      }
      require(call("info").getBoolean("loaded"),"Fixture not loaded");
      Bundle read=request("read");read.putString("region","wram");read.putInt("offset",0);read.putInt("length",0x20000);require(call(read,false).getByteArray("bytes").length==0x20000,"WRAM reply length");
      if(suite.equals("readonly")){
        Bundle control=request("acquire");control.putBinder("token",new Binder());rejects(()->call(control,false),"READ-only controller acquisition");result.putString("stream",log+"PASS full WRAM read with READ only\n");finish(Activity.RESULT_OK,result);return;
      }
      Bundle wrongVersion=request("info");wrongVersion.putString("version","1");rejects(()->call(wrongVersion,false),"wrong version type");
      Bundle wrongOffset=request("read");wrongOffset.putString("offset","0");wrongOffset.putInt("length",1);rejects(()->call(wrongOffset,false),"wrong offset type");
      Bundle large=request("info");large.putByteArray("padding",new byte[16384]);rejects(()->call(large,false),"oversized request");
      Bundle extra=request("info");extra.putInt("unknown",1);call(extra,false);log.append("PASS bounded extension field remains compatible\n");
      rejects(()->call(request("info"),true),"trailing parcel data");
      Bundle acquire=request("acquire");acquire.putBinder("token",new Binder());call(acquire,false);
      Bundle queue=request("queue");queue.putIntArray("masks",new int[256]);int[] frames=new int[256];java.util.Arrays.fill(frames,1);queue.putIntArray("frames",frames);call(queue,false);
      log.append("PASS maximum legitimate sequence accepted\n");
      long before=call("info").getLong("frame");Parcel oneway=Parcel.obtain();try{oneway.writeInterfaceToken(BridgeService.DESCRIPTOR);oneway.writeBundle(queue);remote.transact(1,oneway,null,IBinder.FLAG_ONEWAY);}finally{oneway.recycle();}
      boolean unchanged=call("info").getLong("frame")==before;
      if(baseline)log.append("BASELINE one-way control ").append(unchanged?"blocked\n":"executed\n");else{require(unchanged,"One-way control executed");log.append("PASS one-way control blocked\n");}
      call("pause");Thread.sleep(2250);rejects(()->call("heartbeat"),"expired lease while watchdog delayed");
      try{call("release");}catch(SecurityException expected){}
      call(acquire,false);call("release");log.append("PASS reacquire and release\n");
      log.append("No real emulator, tracker, ROM, game input or windows accessed\n");result.putString("stream",log.toString());finish(Activity.RESULT_OK,result);
    }catch(Throwable e){java.io.StringWriter s=new java.io.StringWriter();e.printStackTrace(new java.io.PrintWriter(s));result.putString("stream",log+"FAIL\n"+s);finish(Activity.RESULT_CANCELED,result);}
    finally{if(bound)getTargetContext().unbindService(connection);}
  }
}
