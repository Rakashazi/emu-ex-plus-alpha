#include <scd/scd.h>
#include "pcm.h"
#include "mem.hh"
#include <emuframework/EmuSystem.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/utility.h>
#include <imagine/util/mayAliasInt.h>

#define READ_FONT_DATA(basemask) \
{ \
      unsigned fnt = *(uint32a*)(sCD.gate + 0x4c); \
      unsigned col0 = (fnt >> 8) & 0x0f, col1 = (fnt >> 12) & 0x0f;   \
      if (fnt & (basemask << 0)) d  = col1      ; else d  = col0;       \
      if (fnt & (basemask << 1)) d |= col1 <<  4; else d |= col0 <<  4; \
      if (fnt & (basemask << 2)) d |= col1 <<  8; else d |= col0 <<  8; \
      if (fnt & (basemask << 3)) d |= col1 << 12; else d |= col0 << 12; \
}

// Gate Array / PCM

extern uint8_t comFlagsSync[2];
extern uint8_t comSync[0x20];
extern bool doingSync;
extern uint8_t comWriteTarget;
extern unsigned comFlagsPoll[2];
extern unsigned comPoll[0x20];
extern M68KCPU mm68k;

static void endSyncSubCpu(unsigned target)
{
	assert(extraCpuSync);
	if(doingSync && comWriteTarget == target)
	{
		sCD.cpu.endCycles = sCD.cpu.cycleCount;
		logMsg("end S CPU sync for target 0x%X run @ cycle %d, M @ %d", target, sCD.cpu.endCycles, mm68k.cycleCount);
	}
}

static void endSyncSubCpu()
{
	assert(extraCpuSync);
	if(doingSync)
	{
		//sCD.cpu.cycleCount = sCD.cpu.endCycles;
		sCD.cpu.endCycles = sCD.cpu.cycleCount;
		logMsg("end S CPU sync for run @ cycle %d, M @ %d", sCD.cpu.endCycles, mm68k.cycleCount);
	}
	//else
	{
		//sCD.cpu.cycleCount = sCD.cpu.endCycles;
	}
}

static unsigned subGateRegRead16(unsigned a)
{
	switch(a)
	{
		case 0:
			return ((sCD.gate[0]&3)<<8) | 1; // ver = 0, not in reset state
		case 2:
		{
			unsigned d = (sCD.gate[2]<<8) | (sCD.gate[3]&0x1f);
			// TODO check for polling
			return d;
		}
		case 4:
		{
			uint8_t cdcMode = sCD.gate[a];
			uint8_t cdcRSO = sCD.gate[a+1];
			uint16 data = (cdcMode << 8) | cdcRSO;
			//logMsg("%X (DSR: %d EDT: %d DD: %X | CA: %X)", data, (cdcMode>>6)&1, cdcMode>>7, cdcMode & 0x7, cdcRSO);
			return data;
		}
		case 6:
			return CDC_Read_Reg();
		case 8:
			return Read_CDC_Host(1); // Gens returns 0 here on byte reads
		case 0xC:
		{
			unsigned d = sCD.stopwatchTimer >> 16;
			logMsg("S stopwatch timer read (%04x)", d);
			return d;
		}
		case 0x30:
			//logMsg("s68k int3 timer read (%02x)", sCD.gate[0x31]);
			return sCD.gate[0x31];
		case 0x34: // fader
			return 0; // no busy bit
		case 0x50: // font data (check: Lunar 2, Silpheed)
		{
			unsigned d;
			READ_FONT_DATA(0x00100000);
			return d;
		}
		case 0x52:
		{
			unsigned d;
			READ_FONT_DATA(0x00010000);
			return d;
		}
		case 0x54:
		{
			unsigned d;
			READ_FONT_DATA(0x10000000);
			return d;
		}
		case 0x56:
		{
			unsigned d;
			READ_FONT_DATA(0x01000000);
			return d;
		}
		case 0x68:
		{
			bug_unreachable("subcode address");
			return 0;
		}
		default:
		{
			//logMsg("GATE sub-CPU read16 %08X", a);
			unsigned d = (sCD.gate[a]<<8) | sCD.gate[a+1];
			if(extraCpuSync)
			{
				switch(a)
				{
					case 0x0e:
						if(comFlagsSync[0])
						{
							logMsg("S read16 M-Com flags 0x%X", d);
							comFlagsPoll[0] = 0;
						}
						else if(comFlagsSync[1])
						{
							comFlagsPoll[0]++;
							if(comFlagsPoll[0] == 2)
							{
								logMsg("S is polling 16 M-Com flags");
								endSyncSubCpu();
							}
						}
						comFlagsSync[0] = 0;
						break;
					case 0x010 ... 0x1e:
						if(comSync[a-0x10] || comSync[a-0xf])
						{
							logMsg("S read16 Com comm 0x%X @ 0x%X", sCD.gate[a], a);
							comPoll[a-0x10] = 0;
						}
						else
						{
							comPoll[a-0x10]++;
							if(comPoll[a-0x10] == 4)
							{
								logMsg("S is polling 16 Com comm 0x%X", a);
								endSyncSubCpu();
							}
						}
						comSync[a-0x10] = comSync[a-0xf] = 0;
						break;
				}
			}
			return d;
		}
	}
}

unsigned subGateRead8(unsigned address)
{
	if((address&0xfffe00) == 0xff8000)
	{
		address &= 0x1ff;
		//logMsg("s68k_regs r8: [%02x] @ %06x", a, SekPcS68k);
		if (address >= 0x0e && address < 0x30)
		{
			unsigned d = sCD.gate[address];
			if(extraCpuSync)
			{
				switch(address)
				{
					case 0x0e:
						if(comFlagsSync[0])
						{
							logMsg("S read M-Com flags 0x%X", d);
							comFlagsPoll[0] = 0;
						}
						else if(comFlagsSync[1])
						{
							comFlagsPoll[0]++;
							if(comFlagsPoll[0] == 2)
							{
								logMsg("S is polling M-Com flags");
								endSyncSubCpu();
							}
						}
						comFlagsSync[0] = 0;
						break;
					case 0x0f:
						//logMsg("S read S-Com flags 0x%X", d);
						break;
					case 0x010 ... 0x1f:
						if(comSync[address-0x10])
						{
							logMsg("S read8 Com comm 0x%X @ 0x%X", sCD.gate[address], address);
							comPoll[address-0x10] = 0;
						}
						else
						{
							comPoll[address-0x10]++;
							if(comPoll[address-0x10] == 4)
							{
								logMsg("S is polling Com comm 0x%X", address);
								endSyncSubCpu();
							}
						}
						comSync[address-0x10] = 0;
						break;
				}
			}
			return d;
		}
		else if (address >= 0x58 && address < 0x68)
		{
			//logMsg("gfx read 8");
			return gfx_cd_read(sCD.rot_comp, address&~1);
		}
		else
		{
			//logMsg("read s68 gate 8 %X", address);
			unsigned d = subGateRegRead16(address&~1);
			if ((address&1)==0) d>>=8;
			return d;
		}
	}
	else if((address&0xff8000)==0xff0000) //  PCM
	{
		//logMsg("s68k_pcm r8: [%06x]", address);
		address &= 0x7fff;
		if (address >= 0x2000)
			return sCD.pcmMem.bank[sCD.pcm.bank][(address>>1)&0xfff];
		else if (address >= 0x20)
		{
			address &= 0x1e;
			unsigned data = sCD.pcm.ch[address>>2].addr >> PCM_STEP_SHIFT;
			if (address & 2) data >>= 8;
			return data;
		}
	}
	logWarn("bad sub gate read8 %08X (%08X)", address, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	return 0;
}

unsigned subGateRead16(unsigned address)
{
	//logMsg("sub gate read16 %08X (%08X)", address, m68k_get_reg (sCD.cpu, M68K_REG_PC));

	if((address&0xfffe00) == 0xff8000)
	{
		address &= 0x1fe;
		if (address >= 0x58 && address < 0x68)
			return gfx_cd_read(sCD.rot_comp, address);
		else
			return subGateRegRead16(address);
	}
	else if((address&0xff8000)==0xff0000)
	{
		address &= 0x7fff;
		unsigned data = 0;
		if (address >= 0x2000)
			data = sCD.pcmMem.bank[sCD.pcm.bank][(address>>1)&0xfff];
		else if (address >= 0x20) {
			address &= 0x1e;
			data = sCD.pcm.ch[address>>2].addr >> PCM_STEP_SHIFT;
			if (address & 2) data >>= 8;
		}
		return data;
	}

	logWarn("bad sub gate read16 %08X (%08X)", address, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	return 0;
}

static void writeSComFlags(unsigned data)
{
	if(extraCpuSync)
	{
		if(sCD.gate[0xf] == data) return;
		if(comFlagsSync[1])
		{
			logMsg("S write S-Com flags 0x%X, M hasn't read", data);
		}
		else
			logMsg("S write S-Com flags 0x%X", data);
		comFlagsSync[1] = 1;
	}
	sCD.gate[0xf] = data;
	if(extraCpuSync)
		endSyncSubCpu(1);
}

void subGateWrite8(unsigned address, unsigned data)
{
	if(((address >> 8) & 0xFF) == 0x80)
	{
		unsigned subAddr = address & 0x1ff;
		switch(subAddr)
		{
			case 0x0:
				//logMsg("set LEDs 0x%X", data);
				sCD.gate[subAddr] = data;
				break;
			case 0x1:
				sCD.gate[subAddr] = data;
				break;
			case 0x2:
				return; // write-protect bits read-only on S68K
			case 0x3: //logMsg("s write mem mode %d", data);
			{
				int dold = sCD.gate[3];
				data &= 0x1d;
				data |= dold&0xc2;
				if (data&4)
				{
					if ((data ^ dold) & 5)
					{
						data &= ~2; // in case of mode or bank change we clear DMNA (m68k req) bit
					}
					if (!(dold & 4))
					{
						logMsg("wram mode 2M->1M");
					}
				}
				else
				{
					if (dold & 4)
					{
						logMsg("wram mode 1M->2M");
						if (!(data&1))
						{ // it didn't set the ret bit, which means it doesn't want to give WRAM to m68k
							data &= ~3;
							data |= (dold&1) ? 2 : 1; // then give it to the one which had bank0 in 1M mode
						}
					}
					else
						data |= dold&1;
					if (data&1) data &= ~2; // return word RAM to m68k in 2M mode
				}
				sCD.gate[subAddr] = data;
				updateCpuWordMap(sCD, data);
			}
			break;
			case 4:
				//logMsg("s68k CDC dest: %x", data&7);
				sCD.gate[subAddr] = (sCD.gate[subAddr]&0xC0) | (data&7); // CDC mode
				break;
			case 0x5:
				//logMsg("s68k CDC reg addr: %x", data&0xf);
				sCD.gate[subAddr] = data;
				break;
			case 0x6:
				sCD.gate[subAddr] = data;
				break;
			case 0x7:
				//logMsg("write CDC reg 0x%X", data);
				CDC_Write_Reg(data);
				break;
			case 0xa:
				logMsg("s68k set CDC dma addr");
				sCD.gate[subAddr] = data;
				break;
			case 0xb:
				logMsg("s68k set CDC dma addr 2");
				sCD.gate[subAddr] = data;
				break;
			case 0xc:
			case 0xd:
				logMsg("s68k set stopwatch timer (val %d)", data);
				sCD.stopwatchTimer = 0;
				break;
			case 0xe:
				writeSComFlags((data>>1) | (data<<7)); // ror8 1, Gens note: Dragons lair
				break;
			case 0xf:
				writeSComFlags(data);
				break;
			case 0x20 ... 0x2f:
			{
				if(extraCpuSync)
				{
					if(sCD.gate[subAddr] == data) break;
					if(comSync[subAddr-0x10])
					{
						logMsg("S write Com status 0x%X @ 0x%X, M hasn't read", data, subAddr);
					}
					else
						logMsg("S write Com status 0x%X @ 0x%X", data, subAddr);
					comSync[subAddr-0x10] = 1;
				}
				sCD.gate[subAddr] = data;
				if(extraCpuSync)
					endSyncSubCpu(subAddr);
				break;
			}
			case 0x30:
				// empty register
				break;
			case 0x31:
				//logMsg("s68k set int3 timer: %02x, int active: %d", data, (sCD.gate[0x33] & (1<<3)) != 0);
				sCD.timer_int3 = (data & 0xff) << 16;
				sCD.gate[subAddr] = data;
				break;
			case 0x32:
				sCD.gate[subAddr] = data;
				break;
			case 0x33:
				//logMsg("s68k irq mask: %02x", data);
				if ((data&(1<<4)) && (sCD.gate[0x37]&4) && !(sCD.gate[0x33]&(1<<4)))
				{
					//logMsg("CDD status");
					CDD_Export_Status();
				}
				sCD.gate[0x33] = data;
				break;
			case 0x34: // fader
				logMsg("set fader %02x", data);
				sCD.gate[0x34] = data & 0x7f;
				scd_updateCddaVol();
				break;
			case 0x35: // fader
				logMsg("set fader low %02x", data);
				sCD.gate[0x35] = data;
				scd_updateCddaVol();
				break;
			case 0x37:
			{
				logMsg("CCD control: %02x", data);
				unsigned d_old = sCD.gate[0x37];
				sCD.gate[0x37] = data&7;
				if ((data&4) && !(d_old&4))
				{
					logMsg("CDD status from control");
					CDD_Export_Status();
				}
				break;
			}
			case 0x42 ... 0x4a:
				sCD.gate[subAddr] = data;
				break;
			case 0x4b:
				sCD.gate[subAddr] = data;
				CDD_Import_Command();
				break;
			case 0x4c:
				sCD.gate[subAddr] = data;
				logMsg("wrote unused font color byte");
				break;
			case 0x4d: // font color
				sCD.gate[subAddr] = data;
				break;
			case 0x4e: // font bit MSB
				sCD.gate[subAddr] = data;
				break;
			case 0x4f: // font bit LSB
				sCD.gate[subAddr] = data;
				break;
			case 0x58 ... 0x67:
				gfx_cd_write16(sCD.rot_comp, subAddr&~1, (data<<8)|data);
				break;
			default:
				logWarn("bad sub GATE write8 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
		}
	}
	else if ((address&0xff8000)==0xff0000)
	{
		address &= 0x7fff;
		if (address >= 0x2000)
		{
			//logMsg("PCM ram write 8 %X", data);
			sCD.pcmMem.bank[sCD.pcm.bank][(address>>1)&0xfff] = data;
		}
		else if (address < 0x12)
		{
			//logMsg("PCM write 8 %X", data);
			pcm_write(address>>1, data);
		}
	}
	else
	{
		logWarn("bad sub GATE write8 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	}
}

void subGateWrite16(unsigned address, unsigned data)
{
	if(((address >> 8) & 0xFF) == 0x80)
	{
		unsigned subAddr = address & 0x1ff;
		switch(subAddr)
		{
			case 0x58 ... 0x67:
				gfx_cd_write16(sCD.rot_comp, subAddr, data);
				break;
			case 0xe:
				// special case, 2 byte writes would be handled differently
				writeSComFlags(data);
				break;
			default:
				//logMsg("sub gate write16 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
				subGateWrite8(address, data >> 8);
				subGateWrite8(address+1, data & 0xFF);
		}
	}
	else if ((address&0xff8000)==0xff0000)
	{
		address &= 0x7fff;
    if (address >= 0x2000)
    {
    	//logMsg("PCM ram write 16 %X", data);
    	sCD.pcmMem.bank[sCD.pcm.bank][(address>>1)&0xfff] = data;
    }
    else if (address < 0x12)
    {
    	//logMsg("PCM write 16 %X", data);
      pcm_write(address>>1, data & 0xff);
    }
  }
	else
	{
		logWarn("bad sub GATE write16 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	}
}

// PRG

void subPrgWriteProtectCheck8(unsigned address, unsigned data)
{
	if(address >= unsigned(sCD.gate[2]<<8))
		WRITE_BYTE(sCD.prg.b, address, data);
	else
		logMsg("write protected");
}

void subPrgWriteProtectCheck16(unsigned address, unsigned data)
{
	if(address >= unsigned(sCD.gate[2]<<8))
		*(uint16a*)(sCD.prg.b + address) = data;
	else
		logMsg("write protected");
}

// WORD

static void decode_write8(unsigned a, uint8_t d, int r3)
{
	//logMsg("decode write 8");
	uint8_t *pd = sCD.word.ram1M[(r3 & 1)^1] + (((a>>1)^1)&0x1ffff);
	uint8_t oldmask = (a&1) ? 0xf0 : 0x0f;

	r3 &= 0x18;
	d  &= 0x0f;
	if (!(a&1)) d <<= 4;

	if (r3 == 8) {
	if ((!(*pd & (~oldmask))) && d) goto do_it;
	} else if (r3 > 8) {
	if (d) goto do_it;
	} else {
	goto do_it;
	}

	return;
	do_it:
	*pd = d | (*pd & oldmask);
}


static void decode_write16(unsigned a, uint16 d, int r3)
{
	//logMsg("decode write 16");
	uint8_t *pd = sCD.word.ram1M[(r3 & 1)^1] + (((a>>1)^1)&0x1ffff);

	//if ((a & 0x3ffff) < 0x28000) return;

	r3 &= 0x18;
	d  &= 0x0f0f;
	d  |= d >> 4;

	if (r3 == 8) {
	uint8_t dold = *pd;
	if (!(dold & 0xf0)) dold |= d & 0xf0;
	if (!(dold & 0x0f)) dold |= d & 0x0f;
	*pd = dold;
	} else if (r3 > 8) {
	uint8_t dold = *pd;
	if (!(d & 0xf0)) d |= dold & 0xf0;
	if (!(d & 0x0f)) d |= dold & 0x0f;
	*pd = d;
	} else {
	*pd = d;
	}
}

unsigned subReadWordDecoded8(unsigned address)
{
	logMsg("Sub read8 decoded %08X", address);
	address&=0xffffff;
	unsigned bank = (sCD.gate[3]&1)^1;
	unsigned d = READ_BYTE(sCD.word.ram1M[bank],(address>>1)&0x1ffff);
	if (address&1) d &= 0x0f;
		else d >>= 4;
	return d;
}

unsigned subReadWordDecoded16(unsigned address)
{
	logMsg("Sub read16 decoded %08X", address);
	address&=0xfffffe;
	unsigned bank = (sCD.gate[3]&1)^1;
	unsigned d = sCD.word.ram1M[bank][((address>>1)^1)&0x1ffff];
	d |= d << 4; d &= ~0xf0;
	return d;
}

void subWriteWordDecoded8(unsigned address, unsigned data)
{
	logMsg("Sub write8 decoded %08X = %X", address, data);
	decode_write8(address, data, sCD.gate[3]);
}

void subWriteWordDecoded16(unsigned address, unsigned data)
{
	logMsg("Sub write16 decoded %08X = %X", address, data);
	decode_write16(address, data, sCD.gate[3]);
}

// BRAM

unsigned bramRead8(unsigned address)
{
	unsigned a = (address>>1)&0x1fff;
	unsigned d = bram[a];
	//logMsg("BRAM read8 %X = %X", address, d);
	return d;
}

unsigned bramRead16(unsigned address)
{
	// TODO: test if BRAM is ever accessed in 16-bits
	logWarn("bram read16");
	unsigned a = (address>>1)&0x1fff;
	unsigned d = bram[a++];
	d|= bram[a] << 8;
	logMsg("BRAM read16 %X = %X", address, d);
	return d;
}

void bramWrite8(unsigned address, unsigned data)
{
	logMsg("BRAM write8 %X = %X", address, data);
	unsigned a = (address>>1)&0x1fff;
	bram[a] = data;
	EmuEx::gSystem().onBackupMemoryWritten();
}

void bramWrite16(unsigned address, unsigned data)
{
	// TODO: test if BRAM is ever accessed in 16-bits
	logMsg("BRAM write16 %X = %X", address, data);
	unsigned a = (address>>1)&0x1fff;
	bram[a++] = data;
	bram[a] = data >> 8;
	EmuEx::gSystem().onBackupMemoryWritten();
}

// Undefined

unsigned subUndefRead8(unsigned address)
{
	logWarn("UNDEF read8 %08X (%08X)", address, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	return 0;
}

unsigned subUndefRead16(unsigned address)
{
	logWarn("UNDEF read16 %08X (%08X)", address, m68k_get_reg (sCD.cpu, M68K_REG_PC));
	return 0;
}

void subUndefWrite8(unsigned address, unsigned data)
{
	logWarn("UNDEF write8 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
}

void subUndefWrite16(unsigned address, unsigned data)
{
	logWarn("UNDEF write16 %08X = %02X (%08X)", address, data, m68k_get_reg (sCD.cpu, M68K_REG_PC));
}

