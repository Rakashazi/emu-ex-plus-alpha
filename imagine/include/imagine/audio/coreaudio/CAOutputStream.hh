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

#include <imagine/audio/defs.hh>
#include <AudioUnit/AudioUnit.h>

namespace IG::Audio
{

class CAOutputStream
{
public:
	CAOutputStream();
	~CAOutputStream();
	CAOutputStream &operator=(CAOutputStream &&) = delete;
	StreamError open(OutputStreamConfig config);
	void play();
	void pause();
	void close();
	void flush();
	bool isOpen();
	bool isPlaying();
	explicit operator bool() const;

private:
	AudioComponentInstance outputUnit{};
	AudioStreamBasicDescription streamFormat;
	OnSamplesNeededDelegate onSamplesNeeded{};
	bool isPlaying_{};
	bool isOpen_{};
};

}
