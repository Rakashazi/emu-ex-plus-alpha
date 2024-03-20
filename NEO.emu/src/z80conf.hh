#pragma once

#include <cstdint>
#include <z80.hh>

inline int z80IrqCallback() { return 0; }

inline uint8_t dummy[0x400];
extern uint8_t mame_z80mem[0x10000];

extern "C"
{
uint8_t z80_port_read(uint16_t PortNo);
void z80_port_write(uint16_t PortNb, uint8_t Value);
}

inline uint8_t z80PortRead(unsigned p) { return z80_port_read(p); }
inline void z80PortWrite(unsigned p, uint8_t data) { z80_port_write(p, data); }

inline uint8_t z80MemRead(unsigned addr) { return mame_z80mem[addr]; }
inline void z80MemWrite(unsigned addr, uint8_t data) { mame_z80mem[addr] = data; }

constexpr Z80Desc z80Desc
{
	.onIrq = z80IrqCallback,
	.useStaticConfig = true,
	.staticReadMap = []()
	{
		std::array<uint8_t*, 64> map{};
		for(size_t i = 0x00; i < 0x40; i++) // ROM + RAM
		{
			map[i] = &mame_z80mem[i * 0x400];
		}
		return map;
	}(),
	.staticWriteMap = []()
	{
		std::array<uint8_t*, 64> map;
    for(size_t i = 0x00; i < 0x38; i++) // ROM
    {
      map[i] = dummy;
    }
    for(size_t i = 0x38; i < 0x40; i++) // RAM
		{
			map[i] = &mame_z80mem[i * 0x400];
		}
    return map;
	}(),
	.staticWriteMem = z80MemWrite,
	.staticReadMem = z80MemRead,
	.staticWritePort = z80PortWrite,
	.staticReadPort = z80PortRead,
};

constexpr uint16_t z80CycleCountScaler = 1;
