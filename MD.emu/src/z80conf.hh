#pragma once

#include <z80.hh>
#include <cstdint>

inline int z80IrqCallback() { return 0xFF; }

constexpr Z80Desc z80Desc{.onIrq = z80IrqCallback};

constexpr uint16_t z80CycleCountScaler = 15;
