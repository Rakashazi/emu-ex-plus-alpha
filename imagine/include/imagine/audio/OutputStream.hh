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

#if defined __ANDROID__
#include <imagine/audio/opensl/OpenSLESOutputStream.hh>
#include <imagine/audio/android/AAudioOutputStream.hh>
#elif defined __APPLE__
#include <imagine/audio/coreaudio/CAOutputStream.hh>
#else
	#if CONFIG_PACKAGE_PULSEAUDIO
	#include <imagine/audio/pulseaudio/PAOutputStream.hh>
	#endif
	#if CONFIG_PACKAGE_ALSA
	#include <imagine/audio/alsa/ALSAOutputStream.hh>
	#endif
#endif

#include <imagine/audio/defs.hh>
#include <imagine/time/Time.hh>
#include <imagine/audio/Format.hh>
#include <imagine/util/variant.hh>
#include <variant>

namespace IG::Audio
{

struct OutputStreamConfig
{
public:
	Format format{};
	OnSamplesNeededDelegate onSamplesNeeded{};
	Microseconds wantedLatencyHint{20000};
	bool startPlaying = true;

	constexpr OutputStreamConfig() = default;
	constexpr OutputStreamConfig(Format format, OnSamplesNeededDelegate onSamplesNeeded = nullptr):
		format{format}, onSamplesNeeded{onSamplesNeeded} {}
};

class NullOutputStream
{
public:
	StreamError open(OutputStreamConfig) { return {}; }
	void play() {}
	void pause() {}
	void close() {}
	void flush() {}
	bool isOpen() { return false; }
	bool isPlaying() { return false; }
};

#if defined __ANDROID__
using OutputStreamVariant = std::variant<AAudioOutputStream, OpenSLESOutputStream, NullOutputStream>;
#elif defined __APPLE__
using OutputStreamVariant = std::variant<CAOutputStream, NullOutputStream>;
#else
	using OutputStreamVariant = std::variant<
	#ifdef CONFIG_PACKAGE_PULSEAUDIO
	PAOutputStream,
	#endif
	#ifdef CONFIG_PACKAGE_ALSA
	ALSAOutputStream,
	#endif
	NullOutputStream>;
#endif

class OutputStream : public OutputStreamVariant, public AddVisit
{
public:
	using OutputStreamVariant::OutputStreamVariant;
	using OutputStreamVariant::operator=;
	using AddVisit::visit;

	constexpr OutputStream(): OutputStreamVariant{std::in_place_type<NullOutputStream>} {}
	void setApi(const Manager &, Api api = Api::DEFAULT);
	StreamError open(OutputStreamConfig config);
	void play();
	void pause();
	void close();
	void flush();
	bool isOpen();
	bool isPlaying();
	void reset();
	explicit constexpr operator bool() const { return !std::holds_alternative<NullOutputStream>(*this); }
};

}
