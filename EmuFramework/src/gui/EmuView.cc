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
#include <emuframework/EmuSystem.hh>
#include <emuframework/OutputTimingManager.hh>
#include <imagine/base/Screen.hh>
#include <imagine/gfx/Renderer.hh>
#include <algorithm>
#include <format>

namespace EmuEx
{

EmuView::EmuView(ViewAttachParams attach, EmuVideoLayer *layer, EmuSystem &sys):
	View{attach},
	layer{layer},
	sysPtr{&sys},
	frameTimeStats{Gfx::Text{attach.rendererTask, &defaultFace()}, Gfx::IQuads{attach.rendererTask, {.size = 1}}} {}

void EmuView::prepareDraw()
{
	doIfUsed(frameTimeStats, [&](auto &stats){ stats.text.makeGlyphs(); stats.text.face()->precacheAlphaNum(renderer()); });
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.makeGlyphs(renderer());
	#endif
}

void EmuView::draw(Gfx::RendererCommands&__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	if(layer && system().isStarted())
	{
		layer->draw(cmds);
	}
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	if(audioStatsText.isVisible())
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, audioStatsRect);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		audioStatsText.draw(cmds, audioStatsRect.x + TableView::globalXIndent,
			audioStatsRect.yCenter(), LC2DO, ColorName::WHITE);
	}
	#endif
}

void EmuView::drawframeTimeStatsText(Gfx::RendererCommands &__restrict__ cmds)
{
	doIfUsed(frameTimeStats, [&](auto &stats)
	{
		if(!stats.text.isVisible())
			return;
		using namespace IG::Gfx;
		cmds.basicEffect().disableTexture(cmds);
		cmds.set(BlendMode::ALPHA);
		cmds.setColor({0., 0., 0., .7});
		cmds.drawQuad(stats.bgQuads, 0);
		cmds.basicEffect().enableAlphaTexture(cmds);
		stats.text.draw(cmds, stats.rect.pos(LC2DO) + WPt{stats.text.spaceWidth(), 0}, LC2DO, ColorName::WHITE);
	});
}

void EmuView::place()
{
	if(layer)
	{
		layer->place(viewRect(), displayRect(), inputView, system());
	}
	placeFrameTimeStats();
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	if(audioStatsText.compile(renderer()))
	{
		audioStatsRect = viewRect.bounds();
		audioStatsRect.y2 = (audioStatsRect.y + audioStatsText.nominalHeight * audioStatsText.lines)
			+ audioStatsText.nominalHeight / 2; // adjust to bottom
	}
	#endif
}

void EmuView::placeFrameTimeStats()
{
	doIfUsed(frameTimeStats, [&](auto &stats)
	{
		if(stats.text.compile())
		{
			WRect rect = {{},
				{stats.text.pixelSize().x + stats.text.spaceWidth() * 2, stats.text.fullHeight()}};
			rect.setPos(viewRect().pos(LC2DO), LC2DO);
			stats.rect = rect;
			stats.bgQuads.write(0, {.bounds = rect.as<int16_t>()});
		}
	});
}

void EmuView::updateFrameTimeStats(FrameTimeStats stats, SteadyClockTimePoint currentFrameTimestamp)
{
	auto screenFrameTime = duration_cast<Milliseconds>(screen()->frameTime());
	auto deadline = duration_cast<Milliseconds>(screen()->presentationDeadline());
	auto timestampDiff = duration_cast<Milliseconds>(currentFrameTimestamp - stats.startOfFrame);
	auto callbackOverhead = duration_cast<Milliseconds>(stats.startOfEmulation - stats.startOfFrame);
	auto emulationTime = duration_cast<Milliseconds>(stats.waitForPresent - stats.startOfEmulation);
	auto presentTime = duration_cast<Milliseconds>(stats.endOfFrame - stats.waitForPresent);
	auto frameTime = duration_cast<Milliseconds>(stats.endOfFrame - stats.startOfFrame);
	doIfUsed(frameTimeStats, [&](auto &statsUI)
	{
		statsUI.text.resetString(std::format("Frame Time Stats\n\n"
			"Screen Frame Time: {}\n"
			"Deadline: {}\n"
			"Timestamp Diff: {}\n"
			"Frame Callback: {}\n"
			"Emulate: {}\n"
			"Present: {}\n"
			"Total: {}\n"
			"Missed Callbacks: {}",
			screenFrameTime, deadline, timestampDiff, callbackOverhead, emulationTime,
			presentTime, frameTime, stats.missedFrameCallbacks));
		placeFrameTimeStats();
	});
}

void EmuView::updateAudioStats([[maybe_unused]] int underruns, [[maybe_unused]] int overruns, [[maybe_unused]] int callbacks, [[maybe_unused]] double avgCallbackFrames, [[maybe_unused]] int frames)
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.setString(std::format("Underruns:{}\nOverruns:{}\nCallbacks per second:{}\nFrames per callback:{:g}\nTotal frames:{}",
		underruns, overruns, callbacks, avgCallbackFrames, frames), &View::defaultFace);
	place();
	#endif
}

void EmuView::clearAudioStats()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.setString(nullptr);
	#endif
}

}
