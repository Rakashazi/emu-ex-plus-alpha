#include <scd/scd.h>
#include <imagine/logger/logger.h>
#include <imagine/util/ranges.hh>
#include "shared.h"
#include "mem.hh"

uint8_t comFlagsSync[2] = { 0 };
uint8_t comSync[0x20] = { 0 };
bool doingSync = 0;
uint8_t comWriteTarget = 0;
unsigned comFlagsPoll[2] = { 0 };
unsigned comPoll[0x20] = { 0 };

static void syncSubCpu(int cycles, unsigned target)
{
	assert(extraCpuSync);
	doingSync = 1;
	comWriteTarget = target;
	logMsg("syncing S cpu to cycle %d for target 0x%X @ cycle %d, M @ %d", cycles, target, sCD.cpu.cycleCount, mm68k.cycleCount);
	scd_runSubCpu(std::min(cycles, mm68k.endCycles));
	logMsg("back from S CPU sync @ cycle %d", sCD.cpu.cycleCount);
	doingSync = 0;
}

unsigned mainGateRead8(unsigned address)
{
	//logMsg("GATE read8 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
	if(((address >> 8) & 0xFF) == 0x20)
	{
		unsigned subAddr = address & 0x3f;
		switch(subAddr)
		{
			case 0:
			{
				unsigned d = ((sCD.gate[0x33]<<13)&0x8000) >> 8;//sCD.gate[0x33]&0x4;
				//logMsg("read irq 0x%X", d);
				return d;
			}
			case 1: //logMsg("read reset");
				return sCD.busreq;
			case 2: //logMsg("read prg mem mode");
				return sCD.gate[0x2];
			case 3: //logMsg("read word mem mode");
			{
				//scd_runSubCpu(mm68k.cycleCount);
				unsigned d = sCD.gate[3]&0xc7;
				// the DMNA delay must only be visible on s68k side (Lunar2, Silpheed)
				if(sCD.delayedDMNA)
				{
					//logMsg("reported delayed DMNA");
					d &= ~1; d |= 2;
				}
				return d;
			}
			case 6: logMsg("read h-int 1"); break;
			case 7: logMsg("read h-int 2"); break;
			case 0xe:
				//logMsg("M read M-Com flags 0x%X", sCD.gate[0xe]);
				return sCD.gate[0xe];
			case 0xf:
			{
				if(extraCpuSync)
				{
					if(comFlagsSync[1])
					{
						logMsg("M read S-Com flags 0x%X", sCD.gate[0xf]);
						comFlagsPoll[1] = 0;
					}
					else if(comFlagsSync[0]) // M cpu has set M-Com flags
					{
						comFlagsPoll[1]++;
						if(comFlagsPoll[1] == 2)
						{
							logMsg("M is polling S-Com flags");
							syncSubCpu(mm68k.cycleCount, 1);
							if(comFlagsSync[1])
								logMsg("M S-Com flags poll broken");
						}
					}
					comFlagsSync[1] = 0;
				}
				return sCD.gate[0xf];
			}
			case 0x10 ... 0x1f: // comm command
				//logMsg("read comm cmd byte");
				return sCD.gate[subAddr];
			case 0x20 ... 0x2F: // comm status
			{
				if(extraCpuSync)
				{
					if(comSync[subAddr-0x10])
					{
						logMsg("M read8 Com status 0x%X @ 0x%X", sCD.gate[subAddr], subAddr);
						comPoll[subAddr-0x10] = 0;
					}
					else
					{
						comPoll[subAddr-0x10]++;
						if(comPoll[subAddr-0x10] == 4)
						{
							logMsg("M is polling Com status 0x%X", subAddr);
							syncSubCpu(mm68k.cycleCount, subAddr);
							if(comSync[subAddr-0x10])
								logMsg("M Com status poll broken");
						}
						if(comSync[subAddr-0x10])
							logMsg("M read8 Com status after sync 0x%X @ 0x%X", sCD.gate[subAddr], subAddr);
					}
					comSync[subAddr-0x10] = 0;
				}
				return sCD.gate[subAddr];
			}
			default:
				bug_unreachable("bad GATE read8 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
		}
		return 0;
	}
	else
		return ctrl_io_read_byte(address);
}

unsigned mainGateRead16(unsigned address)
{
	if(((address >> 8) & 0xFF) == 0x20)
	{
		//logMsg("GATE read16 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
		unsigned subAddr = address & 0x3f;
		switch(subAddr)
		{
			case 0:
				return ((sCD.gate[0x33]<<13)&0x8000) | sCD.busreq; // here IFL2 is always 0, just like in Gens
			case 2:
			{
				//scd_runSubCpu(mm68k.cycleCount);
				unsigned d = (sCD.gate[subAddr]<<8) | (sCD.gate[subAddr+1]&0xc7);
				// the DMNA delay must only be visible on s68k side (Lunar2, Silpheed)
				if(sCD.delayedDMNA) { d &= ~1; d |= 2; }
				return d;
			}
			case 4:
				bug_unreachable("gate reg 4");
				return sCD.gate[4]<<8;
			case 6:
				//logMsg("read h-int");
				return *(uint16a *)(cart.rom + 0x72);
			case 8:
				bug_unreachable("gate reg 8");
				return Read_CDC_Host(0);
			case 0xa:
				logMsg("m68k FIXME: reserved read");
				return 0;
			case 0xc:
			{
				unsigned d = sCD.stopwatchTimer >> 16;
				logMsg("M stopwatch timer read (%04x)", d);
				return d;
			}
			case 0x10 ... 0x1e: // comm command
			{
				return (sCD.gate[subAddr]<<8) | sCD.gate[subAddr+1];
			}
			case 0x20 ... 0x2e: // comm status
			{
				if(extraCpuSync)
				{
					if(comSync[subAddr-0x10] || comSync[subAddr-0xf])
					{
						logMsg("M read16 Com status 0x%X @ 0x%X", sCD.gate[subAddr], subAddr);
						comPoll[subAddr-0x10] = comPoll[subAddr-0xf] = 0;
					}
					else
					{
						comPoll[subAddr-0x10]++;
						if(comPoll[subAddr-0x10] == 4)
						{
							logMsg("M is polling 16 Com status 0x%X", subAddr);
							syncSubCpu(mm68k.cycleCount, subAddr);
							if(comSync[subAddr-0x10])
								logMsg("M Com status poll broken");
						}
					}
					comSync[subAddr-0x10] = comSync[subAddr-0xf] = 0;
				}
				unsigned data = (sCD.gate[subAddr]<<8) | sCD.gate[subAddr+1];
				return data;
			}
			default:
				bug_unreachable("bad GATE read16 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
		}
		return 0;
	}
	else
		return ctrl_io_read_word(address);
}

static void writeMComFlags(unsigned data)
{
	if(extraCpuSync)
	{
		if(sCD.gate[0xe] == data) return;
		if(comFlagsSync[0])
		{
			syncSubCpu(mm68k.cycleCount, 1);
			if(comFlagsSync[0])
			{
				logMsg("M write M-Com flags 0x%X, S hasn't read", data);
			}
			else
				logMsg("M write M-Com flags 0x%X, S synced", data);
		}
		else
			logMsg("M write M-Com flags 0x%X", data);
		comFlagsSync[0] = 1;
	}
	sCD.gate[0xe] = data;
	//scd_runSubCpu(mm68k.cycleCount + mm68k.cycles[mm68k.ir]);
}


void mainGateWrite8(unsigned address, unsigned data)
{
	if(((address >> 8) & 0xFF) == 0x20)
	{
		unsigned subAddr = address & 0x3f;
		switch(subAddr)
		{
			case 0: //logMsg("write irq");
				data &= 1;
				if ((data&1) && (sCD.gate[0x33]&(1<<2)))
				{
					//logMsg("m68k: s68k irq 2");
					scd_interruptSubCpu(2);
				}
				break;
			case 1: //logMsg("write reset, 0x%X", data);
				data &= 3;
				if (!(data&1)) sCD.subResetPending = 1; // reset pending, needed to be sure we fetch the right vectors on reset
				if ( (sCD.busreq&1) != (data&1)) logMsg("sub 68k reset %i", !(data&1));
				if ( (sCD.busreq&2) != (data&2)) logMsg("sub 68k brq %i", (data&2)>>1);
				if (sCD.subResetPending && (data&3)==1)
				{
					scd_resetSubCpu(); // S68k comes out of RESET or BRQ state
					sCD.subResetPending = 0;
					logMsg("resetting sub 68k");
				}
				sCD.busreq = data;
				updateMainCpuPrgMap(sCD);
				break;
			case 2: //logMsg("write mem protect %d", data);
				if(data != sCD.gate[0x2])
					logMsg("new mem protect 0%X", data);
				sCD.gate[0x2] = data;
				break;
			case 3: //logMsg("m write mem mode %d", data);
			{
				unsigned dold = sCD.gate[0x3]&0x1f;
				data &= 0xc2;
				unsigned newBank = (data>>6)&3, oldBank = (sCD.gate[0x3]>>6)&3;
				if (oldBank != newBank)
				{
					logMsg("prg bank: %i -> %i", oldBank, newBank);
					updateMainCpuPrgMap(sCD, newBank);
				}
				if (dold & 4) // is 1M mode active
				{
					data ^= 2; // writing 0 to DMNA actually sets it, 1 does nothing
				}
				else
				{
					if ((data & 2) && !(dold & 2))
					{
						//logMsg("set delayed DMNA");
						sCD.delayedDMNA = 1; // we must delay setting DMNA bit (needed for Silpheed)
						data &= ~2;
					}
				}
				sCD.gate[0x3] = data | dold; // really use s68k side register
				updateCpuWordMap(sCD);
				//logMsg("DMNA: %d", sCD.gate[3] & 0x2 >> 1);
				break;
			}
			case 6: //logMsg("write h-int 1");
				cart.rom[0x72 + 1] = data;
				break;
			case 7: //logMsg("write h-int 2");
				cart.rom[0x72] = data;
				break;
			case 0xf:
				writeMComFlags((data << 1) | ((data >> 7) & 1)); // rol8 1 (special case)
				break;
			case 0xe:
				writeMComFlags(data);
				break;
			case 0x10 ... 0x1f:
			{
				if(extraCpuSync)
				{
					//if(sCD.gate[subAddr] == data) break;
					if(comSync[subAddr-0x10])
					{
						syncSubCpu(mm68k.cycleCount, subAddr);
						//scd_runSubCpu(IG::min(sCD.cpu.cycleCount + 800, mm68k.cycleCount));
						if(comSync[subAddr-0x10])
						{
							logMsg("M write Com command 0x%X @ 0x%X, S hasn't read", data, subAddr);
						}
						else
							logMsg("M write Com command 0x%X @ 0x%X, S synced", data, subAddr);
					}
					else
						logMsg("M write Com command 0x%X @ 0x%X", data, subAddr);
					comSync[subAddr-0x10] = 1;
				}
				sCD.gate[subAddr] = data;
				//syncSubCpu(mm68k.cycleCount + 800);
				break;
			}
			default:
				bug_unreachable("bad GATE write8 %08X = %02X (%08X)", address, data, m68k_get_reg (mm68k, M68K_REG_PC));
		}
	}
	else
		ctrl_io_write_byte(address, data);
}

void mainGateWrite16(unsigned address, unsigned data)
{
	if(((address >> 8) & 0xFF) == 0x20)
	{
		unsigned a = address & 0xfffffe;
		switch(a)
		{
			case 0xe:
			{ // special case, 2 byte writes would be handled differently
				writeMComFlags(data >> 8);
				break;
			}
			case 0x10 ... 0x1e:
			{
				if(extraCpuSync)
				{
					if(sCD.gate[a] == (data >> 8) && sCD.gate[a+1] == (data & 0xFF))
						break;
					if(comSync[a-0x10] || comSync[a-0xf])
					{
						syncSubCpu(mm68k.cycleCount, a+1);
						if(comSync[a-0x10] || comSync[a-0xf])
						{
							logMsg("M write16 Com command 0x%X @ 0x%X, S hasn't read", data, a);
						}
						else
							logMsg("M write16 Com command 0x%X @ 0x%X, S synced", data, a);
					}
					else
						logMsg("M write16 Com command 0x%X @ 0x%X", data, a);
					comSync[a-0x10] = comSync[a-0xf] = 1;
				}
				sCD.gate[a] = data >> 8;
				sCD.gate[a+1] = data & 0xFF;
				//syncSubCpu(mm68k.cycleCount + 800);
				break;
			}
			default:
			{
				//logMsg("GATE write16 %08X = %02X (%08X)", address, data, m68k_get_reg (mm68k, M68K_REG_PC));
				mainGateWrite8(address, data >> 8);
				mainGateWrite8(address+1, data & 0xFF);
			}
		}
	}
	else
		ctrl_io_write_word(address, data);
}

// WORD

unsigned mainReadWordDecoded8(unsigned address)
{
	logMsg("Main read8 decoded %08X", address);
	address&=0xffffff;
	unsigned bank = sCD.gate[3]&1;
	address = (address&3) | (cell_map(address >> 2) << 2); // cell arranged
	return READ_BYTE(sCD.word.ram1M[bank],address);
}

unsigned mainReadWordDecoded16(unsigned address)
{
	logMsg("Main read16 decoded %08X", address);
	address&=0xfffffe;
	unsigned bank = sCD.gate[3]&1;
	address = (address&2) | (cell_map(address >> 2) << 2); // cell arranged
	return *(uint16 *)(sCD.word.ram1M[bank]+address);
}

void mainWriteWordDecoded8(unsigned address, unsigned data)
{
	logMsg("Main write8 decoded %08X = %X", address, data);
	address&=0xffffff;
	unsigned bank = sCD.gate[3]&1;
	address = (address&3) | (cell_map(address >> 2) << 2); // cell arranged
	WRITE_BYTE(sCD.word.ram1M[bank],address,data);
}

void mainWriteWordDecoded16(unsigned address, unsigned data)
{
	logMsg("Main write16 decoded %08X = %X", address, data);
	address&=0xfffffe;
	unsigned bank = sCD.gate[3]&1;
	address = (address&2) | (cell_map(address >> 2) << 2); // cell arranged
	*(uint16 *)(sCD.word.ram1M[bank]+address) = data;
}

// SRAM cart

unsigned sramCartRead8(unsigned address)
{
	return READ_BYTE(sram.sram, (address & 0x1ffff) >> 1);
}

unsigned sramCartRead16(unsigned address)
{
	return *(uint16 *)(sram.sram + ((address & 0x1ffff) >> 1));
}

void sramCartWrite8(unsigned address, unsigned data)
{
	WRITE_BYTE(sram.sram, (address & 0x1ffff) >> 1, data);
}

void sramCartWrite16(unsigned address, unsigned data)
{
	*(uint16 *)(sram.sram + ((address & 0x1ffff) >> 1)) = data;
}

// SRAM write protect register

unsigned bcramRegRead8(unsigned address)
{
	if (address == 0x7fffff)
	{
		unsigned d = sCD.bcramReg;
		logMsg("BCRAM Reg read8 %X = %X", address, d);
		return d;
	}
	else
	{
		bug_unreachable("Bad read8 %X", address);
		return 0;
	}
}

void bcramRegWrite8(unsigned address, unsigned data)
{
	if (address == 0x7fffff)
	{
		//logMsg("BCRAM Reg write8 %X = %X", address, data);
		sCD.bcramReg = data;
		updateMainCpuSramMap(sCD, data);
	}
	else
	{
		bug_unreachable("Bad write8 %X", address);
	}
}

// SRAM cart size register

unsigned sramCartRegRead8(unsigned address)
{
	//logMsg("SRAM read8 %X", address);
	if(address==0x400001)
	{
		logMsg("SRAM cart size read8");
		return 3;
	}
	return 0;//m68k_read_bus_8(address);
}

unsigned sramCartRegRead16(unsigned address)
{
	//logMsg("SRAM read16 %X", address);
	if(address==0x400000)
	{
		logMsg("SRAM cart size read16");
		return 3;
	}
	return 0;//m68k_read_bus_16(address);
}

// Memory map update funcs

void updateMainCpuPrgMap(SegaCD &sCD, unsigned newBank)
{
	if((sCD.busreq & 3) != 1)
	{
		for (int i=0x2; i<0x4; i++)
		{
			auto newBase = &sCD.prg.bank[newBank][(i-2)<<16];
			//if(newBase != mm68k.memory_map[i].base) logMsg("PRG new base addr %p", newBase);
			mm68k.memory_map[i].base = newBase;
			mm68k.memory_map[i].read8    = 0;
			mm68k.memory_map[i].read16   = 0;
			mm68k.memory_map[i].write8   = 0;
			mm68k.memory_map[i].write16  = 0;
		}
	}
	else
	{
		for (int i=0x2; i<0x4; i++)
		{
			mm68k.memory_map[i].base = 0;
			mm68k.memory_map[i].read8    = nullRead8;
			mm68k.memory_map[i].read16   = nullRead16;
			mm68k.memory_map[i].write8   = m68k_unused_8_w;
			mm68k.memory_map[i].write16  = m68k_unused_16_w;
		}
	}
}

void updateMainCpuPrgMap(SegaCD &sCD)
{
	unsigned newBank = (sCD.gate[3]>>6)&3;
	updateMainCpuPrgMap(sCD, newBank);
}

void updateCpuWordMap(SegaCD &sCD, unsigned modeReg)
{
	if(modeReg&4)
	{
		int bank = modeReg&1;
		// 1Mbit direct access MAIN
		for (int i=0x20; i<0x22; i++)
		{
			mm68k.memory_map[i].base     = sCD.word.ram1M[bank] + ((i-0x20)<<16);
			mm68k.memory_map[i].read8    = 0;
			mm68k.memory_map[i].read16   = 0;
			mm68k.memory_map[i].write8   = 0;
			mm68k.memory_map[i].write16  = 0;
		}
		// 1Mbit decode MAIN
		for (int i=0x22; i<0x24; i++)
		{
			mm68k.memory_map[i].base     = 0;
			mm68k.memory_map[i].read8    = mainReadWordDecoded8;
			mm68k.memory_map[i].read16   = mainReadWordDecoded16;
			mm68k.memory_map[i].write8   = mainWriteWordDecoded8;
			mm68k.memory_map[i].write16  = mainWriteWordDecoded16;
		}

		bank = (modeReg&1)^1;
		// 2Mbit decode SUB
		for (int i=0x8; i<0xc; i++)
		{
			sCD.cpu.memory_map[i].base     = 0;
			sCD.cpu.memory_map[i].read8    = subReadWordDecoded8;
			sCD.cpu.memory_map[i].read16   = subReadWordDecoded16;
			sCD.cpu.memory_map[i].write8   = subWriteWordDecoded8;
			sCD.cpu.memory_map[i].write16  = subWriteWordDecoded16;
		}
		// 1Mbit direct access SUB
		for (int i=0xc; i<0xe; i++)
		{
			sCD.cpu.memory_map[i].base     = sCD.word.ram1M[bank] + ((i-0xc)<<16);
			sCD.cpu.memory_map[i].read8    = 0;
			sCD.cpu.memory_map[i].read16   = 0;
			sCD.cpu.memory_map[i].write8   = 0;
			sCD.cpu.memory_map[i].write16  = 0;
		}
	}
	else
	{
		// 2Mbit direct access MAIN
		for (int i=0x20; i<0x24; i++)
		{
			mm68k.memory_map[i].base     = sCD.word.ram2M + ((i-0x20)<<16);
			mm68k.memory_map[i].read8    = 0;
			mm68k.memory_map[i].read16   = 0;
			mm68k.memory_map[i].write8   = 0;
			mm68k.memory_map[i].write16  = 0;
		}
		// 2Mbit direct access SUB
		for (int i=0x8; i<0xc; i++)
		{
			sCD.cpu.memory_map[i].base     = sCD.word.ram2M + ((i-8)<<16);
			sCD.cpu.memory_map[i].read8    = 0;
			sCD.cpu.memory_map[i].read16   = 0;
			sCD.cpu.memory_map[i].write8   = 0;
			sCD.cpu.memory_map[i].write16  = 0;
		}
		// unmapped 1Mbit SUB
		for (int i=0xc; i<0xe; i++)
		{
			sCD.cpu.memory_map[i].base     = 0;
			sCD.cpu.memory_map[i].read8    = 0;
			sCD.cpu.memory_map[i].read16   = 0;
			sCD.cpu.memory_map[i].write8   = 0;
			sCD.cpu.memory_map[i].write16  = 0;
		}
	}
}

void updateCpuWordMap(SegaCD &sCD)
{
	updateCpuWordMap(sCD, sCD.gate[3]);
}

void updateMainCpuSramMap(SegaCD &sCD, unsigned bcramReg)
{
	if(bcramReg&1)
	{
		logMsg("set SRAM read/write");
		for(auto i : IG::iotaCount(2))
		{
			mm68k.memory_map[0x60 + i].write8   = sramCartWrite8;
			mm68k.memory_map[0x60 + i].write16  = sramCartWrite16;
		}
	}
	else
	{
		logMsg("set SRAM read-only");
		for(auto i : IG::iotaCount(2))
		{
			mm68k.memory_map[0x60 + i].write8   = m68k_unused_8_w;
			mm68k.memory_map[0x60 + i].write16  = m68k_unused_16_w;
		}
	}
}

void updateMainCpuSramMap(SegaCD &sCD)
{
	updateMainCpuSramMap(sCD, sCD.bcramReg);
}
