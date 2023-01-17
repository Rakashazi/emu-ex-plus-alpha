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
#include <imagine/input/Input.hh>
#include <algorithm>

namespace EmuEx
{

EmuView::EmuView() {}

EmuView::EmuView(ViewAttachParams attach, EmuVideoLayer *layer, EmuSystem &sys):
	View{attach},
	layer{layer},
	sysPtr{&sys}
{}

void EmuView::prepareDraw()
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.makeGlyphs(renderer());
	#endif
}

void EmuView::draw(Gfx::RendererCommands &__restrict__ cmds)
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
		cmds.setColor(1., 1., 1., 1.);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		audioStatsText.draw(cmds, audioStatsRect.x + TableView::globalXIndent,
			audioStatsRect.yCenter(), LC2DO);
	}
	#endif
}

void EmuView::place()
{
	if(layer)
	{
		layer->place(viewRect(), displayRect(), inputView, system());
	}
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	if(audioStatsText.compile(renderer()))
	{
		audioStatsRect = viewRect.bounds();
		audioStatsRect.y2 = (audioStatsRect.y + audioStatsText.nominalHeight * audioStatsText.lines)
			+ audioStatsText.nominalHeight / 2; // adjust to bottom
	}
	#endif
}

bool EmuView::inputEvent(const Input::Event &e)
{
	return false;
}

void EmuView::setLayoutInputView(EmuInputView *view)
{
	inputView = view;
}

void EmuView::updateAudioStats(int underruns, int overruns, int callbacks, double avgCallbackFrames, int frames)
{
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	audioStatsText.setString(fmt::format("Underruns:{}\nOverruns:{}\nCallbacks per second:{}\nFrames per callback:{:.2f}\nTotal frames:{}",
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
