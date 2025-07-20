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

#include <emuframework/OutputTimingManager.hh>
#include <imagine/base/MessagePort.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/variant.hh>
#include <imagine/util/ScopeGuard.hh>

namespace EmuEx
{

using namespace IG;

class EmuVideo;
class EmuAudio;
class EmuApp;

class FrameRateDetector
{
public:
	std::optional<SteadyClockDuration> run(SteadyClockTimePoint frameTime, SteadyClockDuration slack, SteadyClockDuration screenFrameDuration);

private:
	using FrameTimeSamples = StaticArrayList<SteadyClockTimePoint, 4>;
	FrameTimeSamples frameTimeSamples;
	int totalFrameTimeCount{};
};

class EmuSystemTask
{
public:
	struct SuspendCommand {};
	struct ExitCommand {};
	struct SetWindowCommand
	{
		Window* winPtr;
	};

	using CommandVariant = std::variant<SuspendCommand, ExitCommand, SetWindowCommand>;
	class Command: public CommandVariant, public AddVisit
	{
	public:
		using CommandVariant::CommandVariant;
		using AddVisit::visit;
	};

	struct CommandMessage
	{
		std::binary_semaphore* semPtr{};
		Command command{SuspendCommand{}};

		void setReplySemaphore(std::binary_semaphore* semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	struct SuspendContext
	{
		SuspendContext() = default;
		SuspendContext(EmuSystemTask* taskPtr):taskPtr{taskPtr} {}
		SuspendContext(SuspendContext&& rhs) noexcept { *this = std::move(rhs); }
		SuspendContext& operator=(SuspendContext&& rhs)
		{
			taskPtr = std::exchange(rhs.taskPtr, nullptr);
			return *this;
		}

		void resume()
		{
			if(taskPtr)
				std::exchange(taskPtr, nullptr)->resume();
		}

		~SuspendContext() { resume(); }

	private:
		EmuSystemTask* taskPtr{};
	};

	EmuSystemTask(EmuApp&);
	void start(Window&);
	[[nodiscard]]
	SuspendContext setWindow(Window&);
	[[nodiscard]]
	SuspendContext suspend();
	void stop();
	bool isStarted() const { return threadId_; }
	void sendVideoFormatChangedReply(EmuVideo&);
	void sendFrameFinishedReply(EmuVideo&);
	void sendScreenshotReply(bool success);
	auto threadId() const { return threadId_; }
	Window &window(this auto&& self) { return *self.winPtr; }
	Screen &screen(this auto&& self) { return *self.window().screen(); }
	void updateHostFrameRate(FrameRate);
	void updateFrameRate();
	void calibrateHostFrameRate();
	bool advanceFrames(FrameParams);
	bool waitingForPresent() const { return waitingForPresent_; }
	void notifyWindowPresented();

private:
	EmuApp& app;
	Window* winPtr{};
	IG::OnFrameDelegate onFrameUpdate;
	MessagePort<CommandMessage> commandPort{"EmuSystemTask Command"};
	std::thread taskThread;
	ThreadId threadId_{};
	std::binary_semaphore framePresentedSem{0};
	std::binary_semaphore suspendSem{0};
	FrameRate hostFrameRate;
	FrameRateConfig frameRateConfig;
	int savedAdvancedFrames{};
	FrameRateDetector frameRateDetector;
public:
	bool enableBlankFrameInsertion{};
private:
	bool waitingForPresent_{};
	bool isSuspended{};

	void resume();
	void addOnFrameDelayed();
	void addOnFrame();
	void removeOnFrame();
	IG::OnFrameDelegate onFrameCalibrate();
	IG::OnFrameDelegate onFrameDelayed(int8_t delay);
	void addOnFrameDelegate(IG::OnFrameDelegate);
	void setIntendedFrameRate(FrameRateConfig);
	FrameRateConfig configFrameRate(const Screen&);
	FrameRateConfig configFrameRate(const Screen&, std::span<const FrameRate> supportedRates);
	void setWindowInternal(Window&);
};

}
