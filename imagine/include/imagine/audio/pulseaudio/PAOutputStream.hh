#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/audio/OutputStream.hh>

struct pa_context;
struct pa_stream;
#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
struct pa_glib_mainloop;
#else
struct pa_threaded_mainloop;
#endif

namespace IG::Audio
{

class PAOutputStream : public OutputStream
{
public:
	PAOutputStream();
	~PAOutputStream();
	IG::ErrorCode open(OutputStreamConfig config) final;
	void play() final;
	void pause() final;
	void close() final;
	void flush() final;
	bool isOpen() final;
	bool isPlaying() final;
	explicit operator bool() const;

private:
	pa_context* context{};
	pa_stream* stream{};
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	pa_glib_mainloop* mainloop{};
	bool mainLoopSignaled = false;
	#else
	pa_threaded_mainloop* mainloop{};
	#endif
	OnSamplesNeededDelegate onSamplesNeeded{};
	Format pcmFormat;
	bool isCorked = true;

	void lockMainLoop();
	void unlockMainLoop();
	void signalMainLoop();
	void waitMainLoop();
	void startMainLoop();
	void stopMainLoop();
	void freeMainLoop();
	void iterateMainLoop();
};

}
