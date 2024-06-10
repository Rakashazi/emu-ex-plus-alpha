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
#include <imagine/thread/Thread.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/variant.hh>

namespace EmuEx
{

using namespace IG;

class EmuVideo;
class EmuAudio;
class EmuApp;

class EmuSystemTask
{
public:
	struct FrameParamsCommand
	{
		FrameParams params;
	};

	struct FramePresentedCommand {};
	struct PauseCommand {};
	struct ExitCommand {};

	using CommandVariant = std::variant<FrameParamsCommand, FramePresentedCommand, PauseCommand, ExitCommand>;
	class Command: public CommandVariant, public AddVisit
	{
	public:
		using CommandVariant::CommandVariant;
		using AddVisit::visit;
	};

	struct CommandMessage
	{
		std::binary_semaphore *semPtr{};
		Command command{PauseCommand{}};

		void setReplySemaphore(std::binary_semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	EmuSystemTask(EmuApp &);
	void start();
	void pause();
	void stop();
	void updateFrameParams(FrameParams);
	void notifyFramePresented();
	void sendVideoFormatChangedReply(EmuVideo &);
	void sendFrameFinishedReply(EmuVideo &);
	void sendScreenshotReply(bool success);
	auto threadId() const { return threadId_; }

private:
	EmuApp &app;
	MessagePort<CommandMessage> commandPort{"EmuSystemTask Command"};
	std::thread taskThread;
	ThreadId threadId_{};
	FrameParams frameParams;
public:
	bool framePending{};
};

}
