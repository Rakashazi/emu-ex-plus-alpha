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
#include <emuframework/EmuApp.hh>
#include <algorithm>

EmuView::EmuView(ViewAttachParams attach, EmuVideoLayer *layer, EmuInputView *inputView):
	View{attach},
	layer{layer},
	inputView{inputView}
{}

void EmuView::draw()
{
	using namespace Gfx;
	if(layer)
	{
		layer->draw(projP);
	}
	if(EmuSystem::isActive() && inputView)
	{
		renderer().loadTransform(projP.makeTranslate());
		inputView->draw();
	}
}

void EmuView::place()
{
	if(layer)
	{
		layer->place(viewRect(), projP, inputView);
	}
	if(inputView)
	{
		inputView->place();
	}
}

bool EmuView::inputEvent(Input::Event e)
{
	if(inputView)
	{
		return inputView->inputEvent(e);
	}
	return false;
}

void EmuView::swapLayers(EmuView &view)
{
	std::swap(layer, view.layer);
}
