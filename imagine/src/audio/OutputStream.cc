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
#include <imagine/audio/Manager.hh>
#include <imagine/audio/OutputStream.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/utility.h>
#include <imagine/util/variant.hh>

namespace IG::Audio
{

void OutputStream::setApi(const Manager &mgr, Api api)
{
	api = mgr.makeValidAPI(api);
	switch(api)
	{
		#ifdef CONFIG_PACKAGE_PULSEAUDIO
		case Api::PULSEAUDIO: emplace<PAOutputStream>(); return;
		#endif
		#ifdef CONFIG_PACKAGE_ALSA
		case Api::ALSA: emplace<ALSAOutputStream>(); return;
		#endif
		#ifdef __ANDROID__
		case Api::OPENSL_ES: emplace<OpenSLESOutputStream>(mgr); return;
		case Api::AAUDIO: emplace<AAudioOutputStream>(mgr); return;
		#endif
		#ifdef __APPLE__
		case Api::COREAUDIO: emplace<CAOutputStream>(); return;
		#endif
		default:
			bug_unreachable("audio API should always be valid");
	}
}

StreamError OutputStream::open(OutputStreamConfig config) { return visit([&](auto &v){ return v.open(config); }); }
void OutputStream::play() { visit([&](auto &v){ v.play(); }); }
void OutputStream::pause() { visit([&](auto &v){ v.pause(); }); }
void OutputStream::close() { visit([&](auto &v){ v.close(); }); }
void OutputStream::flush() { visit([&](auto &v){ v.flush(); }); }
bool OutputStream::isOpen() { return visit([&](auto &v){ return v.isOpen(); }); }
bool OutputStream::isPlaying() { return visit([&](auto &v){ return v.isPlaying(); }); }
void OutputStream::reset() { emplace<NullOutputStream>(); }

OutputStreamConfig Manager::makeNativeOutputStreamConfig() const
{
	return {nativeFormat()};
}

}
