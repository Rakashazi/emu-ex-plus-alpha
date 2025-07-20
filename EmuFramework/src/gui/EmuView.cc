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

#include <emuframework/EmuView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/OutputTimingManager.hh>
#include <imagine/base/Screen.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/logger/logger.h>
#include <algorithm>
#include <format>

namespace EmuEx
{

[[maybe_unused]]
constexpr SystemLogger log{"EmuView"};

EmuView::EmuView(ViewAttachParams attach, EmuVideoLayer *layer, EmuSystem &sys):
	View{attach},
	layer{layer},
	sysPtr{&sys},
	statsDisplay{Gfx::Text{attach.rendererTask, &defaultFace()}, Gfx::IQuads{attach.rendererTask, {.size = 1}}} {}

void EmuView::prepareDraw()
{
	if(showStats)
	{
		statsDisplay.text.makeGlyphs();
		statsDisplay.text.face()->precacheAlphaNum(renderer());
	}
}

void EmuView::draw(Gfx::RendererCommands&__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	if(layer && system().isStarted())
	{
		layer->draw(cmds);
	}
}

void EmuView::drawStatsText(Gfx::RendererCommands &__restrict__ cmds)
{
	if(!showStats)
		return;
	if(!statsDisplay.text.isVisible())
		return;
	using namespace IG::Gfx;
	cmds.basicEffect().disableTexture(cmds);
	cmds.set(BlendMode::ALPHA);
	cmds.setColor({0., 0., 0., .7});
	cmds.drawQuad(statsDisplay.bgQuads, 0);
	cmds.basicEffect().enableAlphaTexture(cmds);
	statsDisplay.text.draw(cmds, statsDisplay.rect.pos(LC2DO) + WPt{statsDisplay.text.spaceWidth(), 0}, LC2DO, ColorName::WHITE);
}

void EmuView::place()
{
	if(layer)
	{
		layer->place(viewRect(), displayRect(), inputView, system());
	}
	placeStats();
}

void EmuView::updateStatsDisplay()
{
	std::string fullStr;
	if(showFrameTimingStats)
		fullStr += frameTimingStatsStr;
	if(showAudioStats)
	{
		if(fullStr.size())
			fullStr += "\n\n";
		fullStr += audioStatsStr;
	}
	statsDisplay.text.resetString(fullStr);
}

void EmuView::placeStats()
{
	if(!showStats)
		return;
	if(statsDisplay.text.compile())
	{
		WRect rect = {{},
			{statsDisplay.text.pixelSize().x + statsDisplay.text.spaceWidth() * 2, statsDisplay.text.fullHeight()}};
		rect.setPos(viewRect().pos(LC2DO), LC2DO);
		statsDisplay.rect = rect;
		statsDisplay.bgQuads.write(0, {.bounds = rect.as<int16_t>()});
	}
}

void EmuView::setShowFrameTimingStats(bool on)
{
	showFrameTimingStats = on;
	updateShowStats();
}

void EmuView::setShowAudioStats(bool on)
{
	showFrameTimingStats = on;
	updateShowStats();
}

void EmuView::setFrameTimingStats(FrameTimingViewStats viewStats)
{
	if(!showFrameTimingStats)
		return;
	const Screen& emuScreen = *screen();
	auto& stats = viewStats.stats;
	Milliseconds deltaDuration{};
	if(hasTime(viewStats.lastFrameTime))
		deltaDuration = duration_cast<Milliseconds>(stats.startOfFrame - viewStats.lastFrameTime);
	auto frameDuration = duration_cast<Milliseconds>(stats.endOfFrame - stats.startOfFrame);
	auto clockHz = emuScreen.frameTimerRate().hz() ?: viewStats.hostRate.hz();
	frameTimingStatsStr = std::format("Frame Time Stats\n\n"
		"Screen: {:g}Hz\n"
		"Clock Source: {:g}Hz\n"
		"Input: {:g}Hz\n"
		"Output: {:g}Hz\n"
		"Delta Time: {}\n"
		"Frame Time: {}\n",
		emuScreen.frameRate().hz(), clockHz,
		viewStats.inputRate.hz(), viewStats.outputRate.hz(),
		deltaDuration, frameDuration);
	if(enableFullFrameTimingStats)
	{
		auto callbackOverhead = duration_cast<Milliseconds>(stats.startOfEmulation - stats.startOfFrame);
		auto emulationTime = duration_cast<Milliseconds>(stats.waitForPresent - stats.startOfEmulation);
		auto presentTime = duration_cast<Milliseconds>(stats.endOfFrame - SteadyClockTimePoint{stats.waitForPresent});
		frameTimingStatsStr += std::format("Callback/Emulate/Present Time: {} {} {}\n"
			"Missed Frames: {}",
			callbackOverhead, emulationTime, presentTime, int(stats.missedFrameCallbacks));
	}
	updateStatsDisplay();
	placeStats();
}

void EmuView::setAudioStats(AudioStats stats)
{
	if(!showAudioStats)
		return;
	audioStatsStr = std::format("Audio Stats\n\n"
		"Underruns:{}\n"
		"Overruns:{}\n"
		"Callbacks per second:{}\n"
		"Frames per callback:{:g}\n"
		"Total frames:{}",
		stats.underruns, stats.overruns, stats.callbacks, stats.avgCallbackFrames, stats.frames);
	updateStatsDisplay();
	placeStats();
}

}
