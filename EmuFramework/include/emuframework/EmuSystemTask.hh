#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/base/MessagePort.hh>
#include <imagine/base/CustomEvent.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/PixmapDesc.hh>

class EmuVideo;
class EmuAudio;
class EmuApp;

class EmuSystemTask
{
public:
	enum class Command: uint8_t
	{
		UNSET,
		RUN_FRAME,
		PAUSE,
		EXIT,
	};

	struct CommandMessage
	{
		IG::Semaphore *semPtr{};
		union Args
		{
			struct RunArgs
			{
				EmuVideo *video;
				EmuAudio *audio;
				uint8_t frames;
				bool skipForward;
			} run;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward = false):
			args{video, audio, frames, skipForward}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
		void setReplySemaphore(IG::Semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	EmuSystemTask(EmuApp &);
	void start();
	void pause();
	void stop();
	void runFrame(EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward = false);
	void sendVideoFormatChangedReply(EmuVideo &video);
	void sendFrameFinishedReply(EmuVideo &video);
	void sendScreenshotReply(int num, bool success);
	EmuApp &app() const;

private:
	EmuApp *appPtr{};
	Base::MessagePort<CommandMessage> commandPort{"EmuSystemTask Command"};
	bool started = false;
};
