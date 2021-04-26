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
#include <imagine/input/Input.hh>
#include <algorithm>

EmuView::EmuView() {}

EmuView::EmuView(ViewAttachParams attach, EmuVideoLayer *layer):
	View{attach},
	layer{layer}
{}

void EmuView::prepareDraw()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.makeGlyphs(renderer());
	#endif
}

void EmuView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	if(layer)
	{
		layer->draw(cmds, projP);
	}
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	if(audioStatsText.isVisible())
	{
		cmds.setCommonProgram(CommonProgram::NO_TEX);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setColor(0., 0., 0., .7);
		GeomRect::draw(cmds, audioStatsRect);
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		audioStatsText.draw(cmds, projP.alignXToPixel(audioStatsRect.x + TableView::globalXIndent),
			projP.alignYToPixel(audioStatsRect.yCenter()), LC2DO, projP);
	}
	#endif
}

void EmuView::place()
{
	if(layer)
	{
		layer->place(viewRect(), projP, inputView);
	}
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	if(audioStatsText.compile(renderer(), projP))
	{
		audioStatsRect = projP.bounds();
		audioStatsRect.y2 = (audioStatsRect.y + audioStatsText.nominalHeight * audioStatsText.lines)
			+ audioStatsText.nominalHeight * .5f; // adjust to bottom
	}
	#endif
}

bool EmuView::inputEvent(Input::Event e)
{
	return false;
}

void EmuView::setLayoutInputView(EmuInputView *view)
{
	inputView = view;
}

void EmuView::updateAudioStats(unsigned underruns, unsigned overruns, unsigned callbacks, double avgCallbackFrames, unsigned frames)
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.setString(string_makePrintf<512>("Underruns:%u\nOverruns:%u\nCallbacks per second:%u\nFrames per callback:%.2f\nTotal frames:%u",
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
