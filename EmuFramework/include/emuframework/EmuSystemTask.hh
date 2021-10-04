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
#include <thread>

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
		std::binary_semaphore *semPtr{};
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
		constexpr CommandMessage(Command command, std::binary_semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward = false):
			args{video, audio, frames, skipForward}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
		void setReplySemaphore(std::binary_semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	EmuSystemTask(EmuApp &);
	void start();
	void pause();
	void stop();
	void runFrame(EmuVideo *video, EmuAudio *audio, uint8_t frames, bool skipForward, bool runSync);
	void sendVideoFormatChangedReply(EmuVideo &video, std::binary_semaphore *frameFinishedSemPtr);
	void sendFrameFinishedReply(EmuVideo &video, std::binary_semaphore *frameFinishedSemPtr);
	void sendScreenshotReply(int num, bool success);
	EmuApp &app() const;
	bool resetVideoFormatChanged() { return std::exchange(videoFormatChanged, false); }

private:
	EmuApp *appPtr{};
	Base::MessagePort<CommandMessage> commandPort{"EmuSystemTask Command"};
	std::thread taskThread{};
	bool videoFormatChanged{};
};
