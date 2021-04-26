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

#include <imagine/gui/View.hh>
#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
#include <imagine/gfx/GfxText.hh>
#endif

class EmuInputView;
class EmuVideoLayer;

class EmuView : public View
{
public:
	EmuView();
	EmuView(ViewAttachParams attach, EmuVideoLayer *layer);
	void place() final;
	void prepareDraw() final;
	void draw(Gfx::RendererCommands &cmds) final;
	bool inputEvent(Input::Event e) final;
	bool hasLayer() const { return layer; }
	void setLayoutInputView(EmuInputView *view);
	void updateAudioStats(unsigned underruns, unsigned overruns, unsigned callbacks, double avgCallbackFrames, unsigned frames);
	void clearAudioStats();
	EmuVideoLayer *videoLayer() const { return layer; }

private:
	EmuVideoLayer *layer{};
	EmuInputView *inputView{};
	#ifdef CONFIG_EMUFRAMEWORK_AUDIO_STATS
	Gfx::Text audioStatsText{};
	Gfx::GCRect audioStatsRect{};
	#endif
};
