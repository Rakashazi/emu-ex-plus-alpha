#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

extern "C"
{
	#include "memory.h"
	#include "emu.h"
}
#undef READ_WORD
#undef WRITE_WORD
#undef READ_BYTE
#undef WRITE_BYTE
#include <musashi/m68k.h>
#include "InstructionCycleTable.hh"
#include <imagine/util/utility.h>

static M68KCPU mm68k(m68ki_cycles, true);

int neogeo68KIrqAck(M68KCPU &m68ki_cpu, int int_level)
{
	//logMsg("got interrupt level:%d", int_level);
	m68ki_cpu.int_level = 0;
	return M68K_INT_ACK_AUTOVECTOR;
}

CLINK void cpu_68k_init(void)
{
	m68k_init(mm68k);

	for(int i = 0; i < 0x100; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_invalid_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_invalid_word(addr)&0xFFFF;
			};
		mm68k.memory_map[i].write8 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_invalid_byte(addr, data);
			};
		mm68k.memory_map[i].write16 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_invalid_word(addr, data);
			};
	}

	// CPU ROM
	for(int i = 0; i < 0x10; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_cpu_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_cpu_word(addr)&0xFFFF;
			};
	}

	// Work RAM
	for(int i = 0x10; i < 0x20; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_ram_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_ram_word(addr)&0xFFFF;
			};
		mm68k.memory_map[i].write8 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_ram_byte(addr, data);
			};
		mm68k.memory_map[i].write16 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_ram_word(addr, data);
			};
	}

	// CPU ROM Bank #2
	for(int i = 0x20; i < 0x30; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_bk_normal_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_bk_normal_word(addr)&0xFFFF;
			};
	}

	mm68k.memory_map[0x2f].write8 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_bk_normal_byte(addr, data);
		};
	mm68k.memory_map[0x2f].write16 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_bk_normal_word(addr, data);
		};

	// Controller #1
	mm68k.memory_map[0x30].read8 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl1_byte(addr)&0xFF;
		};
	mm68k.memory_map[0x30].read16 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl1_word(addr)&0xFFFF;
		};

	// Controller #2
	mm68k.memory_map[0x34].read8 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl2_byte(addr)&0xFF;
		};
	mm68k.memory_map[0x34].read16 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl2_word(addr)&0xFFFF;
		};

	// Controller #3 & PD4990
	mm68k.memory_map[0x38].read8 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl3_byte(addr)&0xFF;
		};
	mm68k.memory_map[0x38].read16 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_ctl3_word(addr)&0xFFFF;
		};
	mm68k.memory_map[0x38].write8 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_pd4990_byte(addr, data);
		};
	mm68k.memory_map[0x38].write16 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_pd4990_word(addr, data);
		};

	// Coin & Z80
	mm68k.memory_map[0x32].read8 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_coin_byte(addr)&0xFF;
		};
	mm68k.memory_map[0x32].read16 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_coin_word(addr)&0xFFFF;
		};
	mm68k.memory_map[0x32].write8 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_z80_byte(addr, data);
		};
	mm68k.memory_map[0x32].write16 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_z80_word(addr, data);
		};

	// Video
	mm68k.memory_map[0x3c].read8 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_video_byte(addr)&0xFF;
		};
	mm68k.memory_map[0x3c].read16 =
		[](unsigned int addr) -> unsigned int
		{
			return mem68k_fetch_video_word(addr)&0xFFFF;
		};
	mm68k.memory_map[0x3c].write8 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_video_byte(addr, data);
		};
	mm68k.memory_map[0x3c].write16 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_video_word(addr, data);
		};

	// Settings
	mm68k.memory_map[0x3a].write8 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_setting_byte(addr, data);
		};
	mm68k.memory_map[0x3a].write16 =
		[](unsigned int addr, unsigned int data)
		{
			mem68k_store_setting_word(addr, data);
		};

	// Palette RAM
	for(int i = 0x40; i < 0x80; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_pal_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_pal_word(addr)&0xFFFF;
			};
		mm68k.memory_map[i].write8 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_pal_byte(addr, data);
			};
		mm68k.memory_map[i].write16 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_pal_word(addr, data);
			};
	}

	// Memory Card
	for(int i = 0x80; i < 0x81; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_memcrd_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_memcrd_word(addr)&0xFFFF;
			};
		mm68k.memory_map[i].write8 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_memcrd_byte(addr, data);
			};
		mm68k.memory_map[i].write16 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_memcrd_word(addr, data);
			};
	}

	// System ROM
	for(int i = 0xC0; i < 0xD0; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_bios_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_bios_word(addr)&0xFFFF;
			};
	}

	// Backup RAM
	for(int i = 0xD0; i < 0xE0; i++)
	{
		mm68k.memory_map[i].read8 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_sram_byte(addr)&0xFF;
			};
		mm68k.memory_map[i].read16 =
			[](unsigned int addr) -> unsigned int
			{
				return mem68k_fetch_sram_word(addr)&0xFFFF;
			};
		mm68k.memory_map[i].write8 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_sram_byte(addr, data);
			};
		mm68k.memory_map[i].write16 =
			[](unsigned int addr, unsigned int data)
			{
				mem68k_store_sram_word(addr, data);
			};
	}

	bankaddress = 0;
	if (memory.rom.cpu_m68k.size > 0x100000)
	{
		bankaddress = 0x100000;
	}
}

CLINK void cpu_68k_reset(void)
{
	m68k_pulse_reset(mm68k);
}

CLINK int cpu_68k_run(Uint32 nb_cycle)
{
	if(conf.raster)
	{
		mm68k.cycleCount = 0;
		m68k_run(mm68k, nb_cycle);
		int extraCycles = mm68k.cycleCount - nb_cycle;
		/*logDMsg("ran %u/%u cycles (%d extra) for line:%d",
			mm68k.cycleCount, nb_cycle, extraCycles, memory.vid.current_line);*/
		return extraCycles;
	}
	else
	{
		//logMsg("running cycles:%u", nb_cycle);
		memory.vid.current_line = 0;
		mm68k.cycleCount = 0;
		const int cyclesPerLine = nb_cycle / 264.0;
		int cycles = cyclesPerLine;
		for(int i = 0; i < 265; i++)
		{
			m68k_run(mm68k, cycles);
			//logMsg("line:%d ran to cycle:%u", i, mm68k.cycleCount);
			cycles += cyclesPerLine;
			memory.vid.current_line++;
		}
		return 0;
	}
}

CLINK void cpu_68k_interrupt(int a)
{
	//logMsg("interrupt:%d", a);
	mm68k.setIRQDelay(a);
}

CLINK Uint32 cpu_68k_getpc(void)
{
	logMsg("current PC:0x%X", mm68k.pc);
	return mm68k.pc;
}

CLINK void cpu_68k_mkstate(Stream *gzf,int mode)
{
	mkstate_data(gzf, &mm68k.cycleCount, sizeof(mm68k.cycleCount), mode);
	if(mode == STWRITE)
	{
		unsigned int tmp16, tmp32;
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D0);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D1);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D2);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D3);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D4);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D5);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D6);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_D7);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A0);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A1);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A2);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A3);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A4);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A5);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A6);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_A7);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_PC);  mkstate_data(gzf, &tmp32, 4, mode);
		tmp16 = m68k_get_reg(mm68k, M68K_REG_SR);  mkstate_data(gzf, &tmp16, 2, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_USP); mkstate_data(gzf, &tmp32, 4, mode);
		tmp32 = m68k_get_reg(mm68k, M68K_REG_ISP); mkstate_data(gzf, &tmp32, 4, mode);
	}
	else
	{
		unsigned int tmp16, tmp32;
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D0, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D1, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D2, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D3, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D4, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D5, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D6, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_D7, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A0, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A1, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A2, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A3, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A4, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A5, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A6, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_A7, tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_PC, tmp32);
		mkstate_data(gzf, &tmp16, 2, mode); m68k_set_reg(mm68k, M68K_REG_SR, tmp16);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_USP,tmp32);
		mkstate_data(gzf, &tmp32, 4, mode); m68k_set_reg(mm68k, M68K_REG_ISP,tmp32);
	}
}

// debug hooks

const char *m68KAddrToStr(M68KCPU &cpu, unsigned addr)
{
	unsigned idx = (addr>>16)&0xff;
	return "";
}

static bool isVerboseCPUWrite(M68KCPU &cpu, unsigned addr)
{
	unsigned idx = (addr>>16)&0xff;
	return false;
}

static bool isVerboseCPURead(M68KCPU &cpu, unsigned addr)
{
	unsigned idx = (addr>>16)&0xff;
	return false;
}

void m68k_read_immediate_16_hook(M68KCPU &cpu, unsigned addr)
{
	unsigned mapIdx = ((addr)>>16)&0xff;
	if(isVerboseCPURead(cpu, addr))
		logMsg("read im 16: %s:0x%X, real %p+0x%X", m68KAddrToStr(cpu, addr), addr, cpu.memory_map[mapIdx].base, addr & 0xffff);
}

void m68k_read_immediate_32_hook(M68KCPU &cpu, unsigned addr)
{

}

void m68k_read_pcrelative_8_hook(M68KCPU &cpu, unsigned addr)
{
	unsigned mapIdx = ((addr)>>16)&0xff;
	if(isVerboseCPURead(cpu, addr))
		logMsg("read rel 8: %s:0x%X, real %p+0x%X", m68KAddrToStr(cpu, addr), addr, cpu.memory_map[mapIdx].base, addr & 0xffff);
}

void m68ki_read_8_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map)
{
	if(isVerboseCPURead(cpu, address))
	  logMsg("read 8: %s:0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, map->base, address & 0xffff);
}

void m68ki_read_16_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map)
{
	if(isVerboseCPURead(cpu, address))
		logMsg("read 16: %s:0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, map->base, address & 0xffff);
}

void m68ki_read_32_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map)
{
	if(isVerboseCPURead(cpu, address))
 		logMsg("read 32: %s:0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, map->base, address & 0xffff);
}

void m68ki_write_8_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map, unsigned value)
{
	if(isVerboseCPUWrite(cpu, address))
 		logMsg("write 8: %s:0x%X with 0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, value, map->base, address & 0xffff);
}

void m68ki_write_16_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map, unsigned value)
{
	if(isVerboseCPUWrite(cpu, address))
		logMsg("write 16: %s:0x%X with 0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, value, map->base, address & 0xffff);
}

void m68ki_write_32_hook(M68KCPU &cpu, unsigned address, const _m68k_memory_map *map, unsigned value)
{
	if(isVerboseCPUWrite(cpu, address))
	  logMsg("write 32: %s:0x%X with 0x%X, real %p+0x%X", m68KAddrToStr(cpu, address), address, value, map->base, address & 0xffff);
}
