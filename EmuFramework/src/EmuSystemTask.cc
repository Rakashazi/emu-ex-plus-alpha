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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuSystemTask.hh>
#include <emuframework/EmuViewController.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"EmuSystemTask"};

EmuSystemTask::EmuSystemTask(EmuApp& app_):
	app{app_},
	onFrameUpdate
	{
		[this](FrameParams params)
		{
			bool renderingFrame = advanceFrames(params);
			if(renderingFrame)
			{
				app.record(FrameTimingStatEvent::waitForPresent);
				framePresentedSem.acquire();
				auto endFrameTime = SteadyClock::now();
				app.reportFrameWorkTime(endFrameTime - params.time);
				app.record(FrameTimingStatEvent::endOfFrame, endFrameTime);
				app.viewController().emuView.setFrameTimingStats({.stats{app.frameTimingStats}, .lastFrameTime{params.lastTime},
					.inputRate{app.system().frameRate()}, .outputRate{frameRateConfig.rate}, .hostRate{hostFrameRate}});
			}
			if(params.isFromRenderer() && !renderingFrame)
			{
				window().drawNow();
				framePresentedSem.acquire();
			}
			return true;
		}
	} {}

void EmuSystemTask::start(Window& win)
{
	if(isStarted())
		return;
	win.setDrawEventPriority(Window::drawEventPriorityLocked); // block UI from posting draws
	setWindowInternal(win);
	taskThread = makeThreadSync(
		[this](auto &sem)
		{
			auto threadId = thisThreadId();
			threadId_ = threadId;
			auto eventLoop = EventLoop::makeForThread();
			winPtr->setFrameEventsOnThisThread();
			addOnFrameDelayed();
			bool started = true;
			commandPort.attach(eventLoop, [this, &started](auto msgs)
			{
				for(auto msg : msgs)
				{
					bool threadIsRunning = msg.command.visit(overloaded
					{
						[&](SetWindowCommand& cmd)
						{
							//log.debug("got set window command");
							cmd.winPtr->moveOnFrame(*winPtr, onFrameUpdate, app.frameClockSource);
							cmd.winPtr->moveOnFrame(*winPtr, onFrameCalibrate(), FrameClockSource::Screen);
							setWindowInternal(*cmd.winPtr);
							assumeExpr(msg.semPtr);
							msg.semPtr->release();
							suspendSem.acquire();
							return true;
						},
						[&](SuspendCommand&)
						{
							//log.debug("got suspend command");
							isSuspended = true;
							assumeExpr(msg.semPtr);
							msg.semPtr->release();
							suspendSem.acquire();
							return true;
						},
						[&](ExitCommand&)
						{
							started = false;
							removeOnFrame();
							winPtr->removeFrameEvents();
							threadId_ = 0;
							frameRateConfig = {};
							EventLoop::forThread().stop();
							return false;
						},
					});
					if(!threadIsRunning)
						return false;
				}
				return true;
			});
			sem.release();
			log.info("starting thread:{} event loop", threadId);
			eventLoop.run(started);
			log.info("exiting thread:{}", threadId);
			commandPort.detach();
		});
}

EmuSystemTask::SuspendContext EmuSystemTask::setWindow(Window& win)
{
	assert(!isSuspended);
	if(!isStarted())
		return {};
	auto oldWinPtr = winPtr;
	commandPort.send({.command = SetWindowCommand{&win}}, MessageReplyMode::wait);
	oldWinPtr->setFrameEventsOnThisThread();
	oldWinPtr->setDrawEventPriority(); // allow UI to post draws again
	app.flushMainThreadMessages();
	return {this};
}

void EmuSystemTask::setWindowInternal(Window& win)
{
	winPtr = &win;
	hostFrameRate = win.screen()->frameRate();
	updateFrameRate();
}

EmuSystemTask::SuspendContext EmuSystemTask::suspend()
{
	if(!isStarted() || isSuspended)
		return {};
	log.info("suspending emulation thread");
	commandPort.send({.command = SuspendCommand{}}, MessageReplyMode::wait);
	app.flushMainThreadMessages();
	return {this};
}

void EmuSystemTask::resume()
{
	if(!isStarted() || !isSuspended)
		return;
	log.info("resuming emulation thread");
	isSuspended = false;
	suspendSem.release();
}

void EmuSystemTask::stop()
{
	if(!isStarted())
		return;
	if(Config::DEBUG_BUILD)
	{
		auto threadId = thisThreadId();
		log.info("request stop emulation thread:{} from:{}", threadId_, threadId);
		assert(threadId_ != thisThreadId());
	}
	commandPort.send({.command = ExitCommand{}});
	taskThread.join();
	winPtr->setFrameEventsOnThisThread();
	winPtr->setDrawEventPriority(); // allow UI to post draws again
	winPtr->setIntendedFrameRate(0);
	winPtr = {};
	app.flushMainThreadMessages();
}

void EmuSystemTask::sendVideoFormatChangedReply(EmuVideo &video)
{
	video.dispatchFormatChanged();
}

void EmuSystemTask::sendFrameFinishedReply(EmuVideo &video)
{
	video.dispatchFrameFinished();
}

void EmuSystemTask::sendScreenshotReply(bool success)
{
	app.runOnMainThread([&app = app, success](ApplicationContext)
	{
		app.printScreenshotResult(success);
	});
}

IG::OnFrameDelegate EmuSystemTask::onFrameCalibrate()
{
	frameRateDetector = {};
	return [this](IG::FrameParams params)
	{
		if(!app.system().isActive())
		{
			log.info("aborted frame rate detection");
			return false;
		}
		if(auto opt = frameRateDetector.run(params.time, Nanoseconds{100}, screen().frameRate().duration()); opt)
		{
			if(opt->count())
			{
				updateHostFrameRate(FrameRate{*opt});
			}
			return false;
		}
		return true;
	};
}

IG::OnFrameDelegate EmuSystemTask::onFrameDelayed(int8_t delay)
{
	return [this, delay](IG::FrameParams params)
	{
		if(params.isFromRenderer() || app.video.image())
		{
			waitingForPresent_ = true;
			winPtr->drawNow();
			framePresentedSem.acquire();
		}
		if(delay)
		{
			addOnFrameDelegate(onFrameDelayed(delay - 1));
		}
		else
		{
			if(app.system().isActive())
			{
				addOnFrame();
			}
		}
		return false;
	};
}

void EmuSystemTask::addOnFrameDelegate(IG::OnFrameDelegate onFrame)
{
	winPtr->addOnFrame(onFrame, app.frameClockSource);
}

void EmuSystemTask::addOnFrameDelayed()
{
	// delay before adding onFrame handler to let timestamps stabilize
	auto delay = winPtr->screen()->frameRate().hz() / 4;
	//log.info("delaying onFrame handler by {} frames", onFrameHandlerDelay);
	addOnFrameDelegate(onFrameDelayed(delay));
	calibrateHostFrameRate();
}

void EmuSystemTask::addOnFrame()
{
	addOnFrameDelegate(onFrameUpdate);
	savedAdvancedFrames = 0;
}

void EmuSystemTask::removeOnFrame()
{
	winPtr->removeOnFrame(onFrameUpdate, app.frameClockSource);
	winPtr->removeOnFrame(onFrameCalibrate(), FrameClockSource::Screen);
}

FrameRateConfig EmuSystemTask::configFrameRate(const Screen& screen)
{
	std::array<FrameRate, 1> overrideRate{app.overrideScreenFrameRate};
	auto supportedRates = app.overrideScreenFrameRate ? std::span<const FrameRate>{overrideRate.data(), 1} : screen.supportedFrameRates();
	return configFrameRate(screen, supportedRates);
}

FrameRateConfig EmuSystemTask::configFrameRate(const Screen& screen, std::span<const FrameRate> supportedRates)
{
	auto &system = app.system();
	frameRateConfig = app.outputTimingManager.frameRateConfig(system, supportedRates, app.frameClockSource);
	system.configFrameRate(app.audio.format().rate, frameRateConfig.rate.duration());
	system.timing.exactFrameDivisor = 0;
	if(frameRateConfig.refreshMultiplier > 0)
	{
		system.timing.exactFrameDivisor = std::round(screen.frameRate().hz() / frameRateConfig.rate.hz());
		log.info("using exact frame divisor:{}", system.timing.exactFrameDivisor);
	}
	return frameRateConfig;
}

void EmuSystemTask::updateHostFrameRate(FrameRate rate)
{
	if(!winPtr) [[unlikely]]
		return;
	hostFrameRate = rate;
	configFrameRate(screen(), std::array{rate});
}

void EmuSystemTask::updateFrameRate()
{
	if(!winPtr) [[unlikely]]
		return;
	setIntendedFrameRate(configFrameRate(screen()));
}

void EmuSystemTask::calibrateHostFrameRate()
{
	if(!app.system().isActive())
		return;
	if(winPtr->evalFrameClockSource(app.frameClockSource) == FrameClockSource::Screen)
	{
		winPtr->addOnFrame(onFrameCalibrate(), FrameClockSource::Screen);
	}
}

void EmuSystemTask::setIntendedFrameRate(FrameRateConfig config)
{
	enableBlankFrameInsertion = false;
	if(app.allowBlankFrameInsertion && config.refreshMultiplier > 1 && app.frameInterval <= 1)
	{
		enableBlankFrameInsertion = true;
		if(!app.overrideScreenFrameRate)
		{
			log.info("{} {}", config.rate.hz(),  config.refreshMultiplier);
			config.rate = config.rate.hz() * config.refreshMultiplier;
			log.info("Multiplied intended frame rate to:{:g}", config.rate.hz());
		}
	}
	return window().setIntendedFrameRate(app.overrideScreenFrameRate ? FrameRate{app.overrideScreenFrameRate} :
		app.frameClockSource == FrameClockSource::Timer || config.refreshMultiplier ? config.rate :
		screen().supportedFrameRates().back()); // frame rate doesn't divide evenly in screen's refresh rate, prefer the highest rate
}

bool EmuSystemTask::advanceFrames(FrameParams frameParams)
{
	assert(hasTime(frameParams.time));
	auto &sys = app.system();
	auto &viewCtrl = app.viewController();
	auto *audioPtr = app.audio ? &app.audio : nullptr;
	auto frameInfo = sys.timing.advanceFrames(frameParams);
	int interval = app.frameInterval;
	if(app.presentationTimeMode == PresentationTimeMode::full ||
		(app.presentationTimeMode == PresentationTimeMode::basic && interval > 1))
	{
		viewCtrl.presentTime = frameParams.presentTime(interval);
	}
	else
	{
		viewCtrl.presentTime = {};
	}
	if(sys.shouldFastForward()) [[unlikely]]
	{
		// for skipping loading on disk-based computers
		if(sys.skipForwardFrames({this}, 20))
		{
			// don't write any audio while skip is in progress
			audioPtr = nullptr;
		}
		frameInfo.advanced = 1;
	}
	if(!frameInfo.advanced)
	{
		if(enableBlankFrameInsertion)
		{
			waitingForPresent_ = true;
			viewCtrl.drawBlankFrame = true;
			window().drawNow();
			return true;
		}
		return false;
	}
	bool allowFrameSkip = interval || sys.frameDurationMultiplier != 1.;
	if(frameInfo.advanced + savedAdvancedFrames < interval)
	{
		// running at a lower target fps
		savedAdvancedFrames += frameInfo.advanced;
	}
	else
	{
		savedAdvancedFrames = 0;
		if(!allowFrameSkip)
		{
			frameInfo.advanced = 1;
		}
		if(frameInfo.advanced > 1 && frameParams.elapsedFrames() > 1)
		{
			app.frameTimingStats.missedFrameCallbacks += frameInfo.advanced - 1;
		}
	}
	assumeExpr(frameInfo.advanced > 0);
	// cap advanced frames if we're falling behind
	if(frameInfo.duration > Milliseconds{70})
		frameInfo.advanced = std::min(frameInfo.advanced, 4);
	EmuVideo *videoPtr = savedAdvancedFrames ? nullptr : &app.video;
	if(videoPtr)
	{
		app.record(FrameTimingStatEvent::startOfFrame, frameParams.time);
		app.record(FrameTimingStatEvent::startOfEmulation);
		waitingForPresent_ = true;
	}
	//log.debug("running {} frame(s), skip:{}", frameInfo.advanced, !videoPtr);
	sys.runFrames({this}, videoPtr, audioPtr, frameInfo.advanced);
	app.inputManager.turboActions.update(app);
	return videoPtr;
}

void EmuSystemTask::notifyWindowPresented()
{
	if(waitingForPresent_)
	{
		waitingForPresent_ = false;
		framePresentedSem.release();
	}
}

std::optional<SteadyClockDuration> FrameRateDetector::run(SteadyClockTimePoint frameTime, SteadyClockDuration slack, SteadyClockDuration screenFrameDuration)
{
	const int framesToTime = 360;
	totalFrameTimeCount++;
	if(totalFrameTimeCount >= framesToTime)
	{
		log.warn("couldn't find stable frame duration after {} frames", totalFrameTimeCount);
		return std::make_optional<SteadyClockDuration>();
	}
	if(frameTimeSamples.size() && frameTime == frameTimeSamples.back())
	{
		return {};
	}
	if(frameTimeSamples.isFull())
		frameTimeSamples.erase(frameTimeSamples.begin());
	frameTimeSamples.emplace_back(frameTime);
	if(frameTimeSamples.size() != frameTimeSamples.capacity())
	{
		return {};
	}
	std::array<SteadyClockDuration, FrameTimeSamples::capacity() - 1> frameDurations;
	for(auto&& [i, duration] : enumerate(frameDurations))
	{
		duration = frameTimeSamples[i + 1] - frameTimeSamples[i];
	}
	for(auto i : iotaCount(frameDurations.size() - 1))
	{
		auto delta  = std::chrono::abs(duration_cast<Nanoseconds>(frameDurations[i + 1] - frameDurations[i]));
		if(delta > slack)
		{
			log.info("frame durations differed by:{}", delta);
			return {};
		}
	}
	auto avgDuration = std::ranges::fold_left(frameDurations, SteadyClockDuration{}, std::plus<>()) / frameDurations.size();
	if(std::chrono::abs(duration_cast<Nanoseconds>(screenFrameDuration - avgDuration)) <= Nanoseconds{1000})
	{
		log.info("detected frame duration:{} nearly matches screen:{}", avgDuration, screenFrameDuration);
		return std::make_optional<SteadyClockDuration>();
	}
	log.info("found frame duration:{}", avgDuration);
	return std::make_optional<SteadyClockDuration>(avgDuration);
}

}
