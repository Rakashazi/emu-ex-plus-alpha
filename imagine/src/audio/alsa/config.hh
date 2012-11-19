#pragma once

#include <alsa/asoundlib.h>

namespace Audio
{
	typedef snd_pcm_uframes_t uframes;
	static const int maxRate = 48000;
}
