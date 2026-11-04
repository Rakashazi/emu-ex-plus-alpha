#!/usr/bin/env python3
"""Generate an original 32 KiB LoROM test fixture, no commercial ROM bytes.
WRAM: 0..1=NMI count, 2..3=autojoy pad0 state, 0x10..0x13=BRDG.
Screen intentionally force-blank. Tests observe execution through WRAM.
"""
from pathlib import Path
rom=bytearray([0xea])*32768
boot=bytearray.fromhex('78 18 fb c2 30 a2 ff 1f 9a e2 20 a9 80 8d 00 21')
for i,b in enumerate(b'BRDG'):boot+=bytes([0xa9,b,0x85,0x10+i])
# Clear frame count in 16-bit accumulator mode, then enable NMI+autojoy.
boot+=bytes.fromhex('c2 20 a9 00 00 85 00 e2 20 a9 81 8d 00 42 cb 80 fd')
rom[:len(boot)]=boot
# Save flags before changing register widths; 1000-iteration delay allows autojoy to finish.
nmi=bytes.fromhex('08 c2 30 48 da a2 e8 03 ca d0 fd a5 00 29 ff 01 0a aa ad 18 42 9d 00 02 85 02 e6 00 fa 68 28 40')
rom[0x100:0x100+len(nmi)]=nmi
rom[0x7fc0:0x7fd5]=b'BRIDGE INPUT TEST    '
rom[0x7fd5:0x7fdc]=bytes([0x20,0,5,0,1,0,0])
for off in [0x7fea,0x7ffa]:rom[off:off+2]=bytes([0,0x81])
rom[0x7ffc:0x7ffe]=bytes([0,0x80])
rom[0x7fdc:0x7fe0]=bytes([0xff,0xff,0,0])
checksum=sum(rom)&0xffff
rom[0x7fdc:0x7fe0]=(checksum^0xffff).to_bytes(2,'little')+checksum.to_bytes(2,'little')
out=Path(__file__).resolve().parent/'artifacts/bridge-input-test.sfc';out.parent.mkdir(exist_ok=True);out.write_bytes(rom)
assert len(rom)==32768
print(out)
