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
#include <imagine/audio/Format.hh>
#include <imagine/base/CustomEvent.hh>

typedef struct AAudioStreamStruct AAudioStream;
typedef struct AAudioStreamBuilderStruct AAudioStreamBuilder;

namespace IG::Audio
{

class Manager;

class AAudioOutputStream
{
public:
	AAudioOutputStream(const Manager &);
	~AAudioOutputStream();
	AAudioOutputStream &operator=(AAudioOutputStream &&) = delete;
	StreamError open(OutputStreamConfig config);
	void play();
	void pause();
	void close();
	void flush();
	bool isOpen();
	bool isPlaying();
	explicit operator bool() const;

private:
	AAudioStream *stream{};
	AAudioStreamBuilder *builder{};
	OnSamplesNeededDelegate onSamplesNeeded{};
	CustomEvent disconnectEvent;
	bool isPlaying_{};

	void setBuilderData(AAudioStreamBuilder *builder, Format format, bool lowLatencyMode);
};

}
