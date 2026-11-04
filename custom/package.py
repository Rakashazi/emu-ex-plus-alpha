#!/usr/bin/env python3
"""Offline APK packaging using Android SDK tools, no Gradle daemon/downloads.
Run after native build: python3 custom/package.py emulator|testclient.
"""
import os, shutil, subprocess, sys, zipfile
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parent.parent
SDK=Path(os.environ.get('ANDROID_SDK_ROOT',str(Path.home()/'Android/Sdk')))
JAVA=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/default'))/'bin'
TOOLS=SDK/'build-tools/36.0.0'; ANDROID=SDK/'platforms/android-36/android.jar'
MODE=sys.argv[1]; assert MODE in ('emulator','testclient')
BUILD=ROOT/'custom/apk-build'/MODE; BUILD.mkdir(parents=True,exist_ok=True)
GEN=BUILD/'gen'; GEN.mkdir(exist_ok=True)
CLASSES=BUILD/'classes'
for folder in [GEN,CLASSES,BUILD/'dex']:
    if folder.exists():shutil.rmtree(folder)
    folder.mkdir(exist_ok=True)
OUT=ROOT/'custom/artifacts'; OUT.mkdir(exist_ok=True)
ENV=dict(os.environ, JAVA_HOME=str(JAVA.parent),PATH=str(JAVA)+':'+os.environ['PATH'])
def run(args): subprocess.run([str(x) for x in args],check=True,env=ENV)
A='{http://schemas.android.com/apk/res/android}'
ET.register_namespace('android',A[1:-1])
jars=[]; flats=[]; extras=[]
if MODE=='emulator':
    for archive in sorted((ROOT/'custom/vendor').glob('*.aar')):
        if archive.stem.startswith('support-core-utils'): continue # not a dependency of support-compat
        dest=BUILD/archive.stem; dest.mkdir(exist_ok=True)
        with zipfile.ZipFile(archive) as z: z.extractall(dest)
        jars.append(dest/'classes.jar')
        if (dest/'res').exists():
            flat=BUILD/(archive.stem+'.zip');run([TOOLS/'aapt2','compile','--dir',dest/'res','-o',flat]);flats.append(flat)
        manifest=dest/'AndroidManifest.xml'
        if manifest.exists(): extras.append(ET.parse(manifest).getroot().get('package'))
    jars+=list((ROOT/'custom/vendor').glob('*.jar'))
    manifest=BUILD/'AndroidManifest.xml'
    run(['bash',ROOT/'imagine/tools/genAndroidMeta.sh','--name=Snes9x EX+ Custom','--version=0.2.0','--vendor=Robert Broglia / Custom Bridge','--game','--permission-write-ext','--permission-bluetooth','--permission-vibrate','--permission-install-shortcut','--xperia-play-optimized','--legacy-storage','--app-ext-storage','--intent-mimetypes=application/zip','--intent-file-extensions=rar 7z smc sfc swc bs st fig mgd',manifest])
    tree=ET.parse(manifest); base=tree.getroot();base.set('package','org.snes9x.custom')
    ET.SubElement(base,'uses-sdk',{A+'minSdkVersion':'26',A+'targetSdkVersion':'36'})
    for name,label in [('READ','Read SNES memory and ROM'),('CONTROL','Control the SNES joypad')]:
        ET.SubElement(base,'permission',{A+'name':'org.snes9x.bridge.permission.'+name,A+'protectionLevel':'dangerous',A+'label':label})
    app=base.find('application');app.set(A+'resizeableActivity','true');app.set(A+'debuggable','false');app.set(A+'allowBackup','false');app.set(A+'fullBackupContent','false');app.set(A+'usesCleartextTraffic','false');app.set(A+'extractNativeLibs','true')
    ET.SubElement(app,'service',{A+'name':'org.snes9x.bridge.BridgeService',A+'exported':'true',A+'permission':'org.snes9x.bridge.permission.READ'})
    tree.write(manifest,encoding='utf-8',xml_declaration=True)
    res=BUILD/'res';(res/'values').mkdir(parents=True,exist_ok=True);(res/'mipmap-mdpi').mkdir(exist_ok=True)
    (res/'values/strings.xml').write_text('<resources><string name="app_name">Snes9x EX+ Custom</string><style name="AppTheme" parent="android:style/Theme.Material.NoActionBar"/></resources>')
    shutil.copy(ROOT/'Snes9x/res/icons/icon-48.png',res/'mipmap-mdpi/icon.png')
    (res/'drawable').mkdir(exist_ok=True)
    shutil.copy(ROOT/'Snes9x/res/icons/icon-48.png',res/'drawable/iconbig.png')
    flat=BUILD/'app-res.zip';run([TOOLS/'aapt2','compile','--dir',res,'-o',flat]);flats.append(flat)
    sources=list((ROOT/'imagine/src/base/android/imagine-v9/src/main/java').rglob('*.java'))+list((ROOT/'custom/android/src').rglob('*.java'))
else:
    manifest=ROOT/'custom/testclient/AndroidManifest.xml'
    sources=list((ROOT/'custom/testclient/src').rglob('*.java'))
resource_apk=BUILD/'resources.apk'
args=[TOOLS/'aapt2','link','-o',resource_apk,'--manifest',manifest,'-I',ANDROID,'--java',GEN,'--auto-add-overlay']
if extras: args+=['--extra-packages',':'.join(x for x in extras if x)]
for flat in flats: args+=['-R',flat]
run(args)
sources+=list(GEN.rglob('*.java'))
run([JAVA/'javac','-source','8','-target','8','-cp',os.pathsep.join(map(str,[ANDROID]+jars)),'-d',CLASSES]+sources)
classes_jar=BUILD/'app-classes.jar'
with zipfile.ZipFile(classes_jar,'w') as z:
    for p in CLASSES.rglob('*.class'):z.write(p,p.relative_to(CLASSES))
dex=BUILD/'dex';dex.mkdir(exist_ok=True)
run([TOOLS/'d8','--min-api','26','--lib',ANDROID,'--output',dex,classes_jar]+jars)
unsigned=BUILD/'unsigned.apk';shutil.copy(resource_apk,unsigned)
with zipfile.ZipFile(unsigned,'a',compression=zipfile.ZIP_DEFLATED) as z:
    for p in dex.glob('*.dex'):z.write(p,p.name)
    if MODE=='emulator':
        lib=ROOT/'Snes9x/build/android/src/main/jniLibs/arm64-v8a/libmain.so'
        if not lib.exists():raise SystemExit('Native Release library missing: '+str(lib))
        z.write(lib,'lib/arm64-v8a/libmain.so')
        for p in (ROOT/'custom/licenses').glob('*'):z.write(p,'assets/licenses/'+p.name)
        for name in ['COPYING.GPL','Snes9x/COPYING']:
            z.write(ROOT/name,'assets/licenses/'+name.replace('/','_'))
        for p in (ROOT/'Snes9x/res').rglob('*'):
            if p.is_file() and 'assets' in p.parts: z.write(p,'assets/'+p.name)
aligned=BUILD/'aligned.apk';run([TOOLS/'zipalign','-f','-p','4',unsigned,aligned])
kind='release' if MODE=='emulator' else 'qa'
private=Path(os.environ.get('SNES_PRIVATE_SIGNING_DIR',str(Path.home()/'.local/share/snes9x-custom-signing')))
key=private/(kind+'.keystore');password=private/(kind+'-password.txt')
if not key.exists() or not password.exists():raise SystemExit('Initialize or restore the protected '+kind+' signer using custom/init-signing.py')
if private.stat().st_mode&0o077 or key.stat().st_mode&0o077 or password.stat().st_mode&0o077:raise SystemExit('Signing directory/files must enforce mode 0700/0600')
final=OUT/('snes9x-custom-release.apk' if MODE=='emulator' else 'bridge-test-qa.apk')
args=[TOOLS/'apksigner','sign','--ks',key,'--ks-key-alias','snes-custom-'+kind,'--ks-pass','file:'+str(password)]
if MODE=='emulator':args+=['--debuggable-apk-permitted','false']
run(args+['--out',final,aligned])
verified=subprocess.check_output([str(TOOLS/'apksigner'),'verify','--verbose','--print-certs',str(final)],env=ENV,text=True)
pin=(ROOT/'custom/security'/(kind+'-certificate.sha256')).read_text().strip()
if 'Signer #1 certificate SHA-256 digest: '+pin not in verified:raise SystemExit('Unexpected signer')
print(verified);print(final)
