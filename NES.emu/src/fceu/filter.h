#pragma once

int32 NeoFilterSound(FCEU_SoundSample2 *in, FCEU_SoundSample2 *out, uint32 inlen, int32 *leftover, FCEU_SoundSample* WaveFinal);
void MakeFilters(int32 rate);
template<class InSample>
void SexyFilter(InSample *in, FCEU_SoundSample *out, int32 count);
