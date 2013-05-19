#pragma once

#include <alsa/asoundlib.h>
#include <config/machine.hh>

namespace Audio
{
	typedef snd_pcm_uframes_t uframes;
	static const int maxRate = Config::MACHINE_IS_PANDORA ? 44100 : 48000;
}
