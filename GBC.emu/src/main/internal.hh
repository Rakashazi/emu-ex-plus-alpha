#pragma once

#include <emuframework/Option.hh>
#include <gambatte.h>

namespace gambatte
{
extern bool useFullColorSaturation;
}

extern Option<OptionMethodRef<bool, gambatte::useFullColorSaturation>, uint8> optionFullGbcSaturation;
extern Byte1Option optionGBPal;
extern Byte1Option optionUseBuiltinGBPalette;
extern Byte1Option optionReportAsGba;
extern Byte1Option optionAudioResampler;
extern gambatte::GB gbEmu;

void applyGBPalette();
