#!/usr/bin/env python3
"""Initialize a private local signer once. Never replace an established release identity."""
import argparse,base64,hashlib,os,secrets,subprocess
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--qa',action='store_true');args=parser.parse_args()
root=Path(__file__).resolve().parent;kind='qa' if args.qa else 'release'
java=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/default'))/'bin'
private=Path(os.environ.get('SNES_PRIVATE_SIGNING_DIR',str(Path.home()/'.local/share/snes9x-custom-signing')))
private.mkdir(parents=True,exist_ok=True);private.chmod(0o700)
if private.stat().st_mode&0o077:raise SystemExit('Signing directory must enforce private mode 0700')
key=private/(kind+'.keystore');password=private/(kind+'-password.txt')
public=root/'security';cert=public/(kind+'-certificate.pem')
if not key.exists():
 if cert.exists():raise SystemExit('Restore the established private signer; replacement is forbidden')
 fd=os.open(password,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 with os.fdopen(fd,'w') as out:out.write(secrets.token_urlsafe(48)+'\n')
 subprocess.run([str(java/'keytool'),'-genkeypair','-storetype','PKCS12','-keystore',str(key),'-storepass:file',str(password),'-keypass:file',str(password),'-alias','snes-custom-'+kind,'-dname','CN=Snes9x Custom '+kind+',O=RndmMeme','-keyalg','RSA','-keysize','3072','-sigalg','SHA256withRSA','-validity','10000'],check=True)
key.chmod(0o600);password.chmod(0o600)
if key.stat().st_mode&0o077 or password.stat().st_mode&0o077:raise SystemExit('Signing files must enforce mode 0600')
export=subprocess.check_output([str(java/'keytool'),'-exportcert','-rfc','-keystore',str(key),'-storepass:file',str(password),'-alias','snes-custom-'+kind]).replace(b'\r\n',b'\n')
if cert.exists() and cert.read_bytes()!=export:raise SystemExit('Established public certificate differs; refusing replacement')
public.mkdir(exist_ok=True);cert.write_bytes(export)
der=base64.b64decode(b''.join(line for line in export.splitlines() if not line.startswith(b'---')))
(public/(kind+'-certificate.sha256')).write_text(hashlib.sha256(der).hexdigest()+'\n')
print('Private '+kind+' identity protected; only public certificate exported')
