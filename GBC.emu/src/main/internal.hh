#pragma once

#include <emuframework/Option.hh>
#include <gambatte.h>

namespace EmuEx
{

class GbcInput : public gambatte::InputGetter
{
public:
	unsigned bits{};

	constexpr GbcInput() = default;
	constexpr unsigned operator()() override { return bits; }
};

extern Byte1Option optionFullGbcSaturation;
extern Byte1Option optionGBPal;
extern Byte1Option optionUseBuiltinGBPalette;
extern Byte1Option optionReportAsGba;
extern Byte1Option optionAudioResampler;
extern gambatte::GB gbEmu;
extern GbcInput gbcInput;

}
