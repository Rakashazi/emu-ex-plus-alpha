#!/usr/bin/env python3
"""Build only the isolated Java service/Parcel fixture; no emulator library is included."""
import os,shutil,subprocess,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[2];fixture=root/'custom/securitytest';build=root/'custom/apk-build/securitytest'
sdk=Path(os.environ.get('ANDROID_SDK_ROOT',str(Path.home()/'Android/Sdk')));tools=sdk/'build-tools/36.0.0';android=sdk/'platforms/android-36/android.jar'
java=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/default'))/'bin';env=dict(os.environ,JAVA_HOME=str(java.parent),PATH=str(java)+':'+os.environ['PATH'])
def run(argv):subprocess.run([str(v) for v in argv],env=env,check=True)
if build.exists():shutil.rmtree(build)
for name in ['gen','classes','dex']:(build/name).mkdir(parents=True)
run([tools/'aapt2','link','-o',build/'resources.apk','--manifest',fixture/'AndroidManifest.xml','-I',android,'--java',build/'gen'])
run([java/'javac','-source','8','-target','8','-cp',android,'-d',build/'classes']+list((fixture/'src').rglob('*.java'))+[root/'custom/android/src/org/snes9x/bridge/BridgeService.java'])
with zipfile.ZipFile(build/'classes.jar','w') as z:
 for p in (build/'classes').rglob('*.class'):z.write(p,p.relative_to(build/'classes'))
run([tools/'d8','--min-api','26','--lib',android,'--output',build/'dex',build/'classes.jar'])
shutil.copyfile(build/'resources.apk',build/'unsigned.apk')
with zipfile.ZipFile(build/'unsigned.apk','a',compression=zipfile.ZIP_DEFLATED) as z:
 for p in (build/'dex').glob('*.dex'):z.write(p,p.name)
run([tools/'zipalign','-f','4',build/'unsigned.apk',build/'aligned.apk'])
private=Path(os.environ.get('SNES_PRIVATE_SIGNING_DIR',str(Path.home()/'.local/share/snes9x-custom-signing')));key=private/'qa.keystore';password=private/'qa-password.txt'
if not key.exists() or not password.exists() or private.stat().st_mode&0o077 or key.stat().st_mode&0o077 or password.stat().st_mode&0o077:raise SystemExit('Initialize protected local QA signing with python3 custom/init-signing.py --qa')
out=root/'custom/artifacts/bridge-securitytest-qa.apk';out.parent.mkdir(exist_ok=True)
run([tools/'apksigner','sign','--ks',key,'--ks-key-alias','snes-custom-qa','--ks-pass','file:'+str(password),'--out',out,build/'aligned.apk'])
run([tools/'apksigner','verify',out]);print(out)
