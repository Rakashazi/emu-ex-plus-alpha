#pragma once

#include <cstdint>
#include <array>

namespace IG
{

template <unsigned SIZE>
using ByteArray = std::array<uint8_t, SIZE>;

}
