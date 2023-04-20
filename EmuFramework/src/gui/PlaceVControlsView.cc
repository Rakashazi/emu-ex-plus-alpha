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

#include "PlaceVControlsView.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

PlaceVControlsView::PlaceVControlsView(ViewAttachParams attach, VController &vController_):
	View{attach},
	text{"Click center to go back", &defaultFace()},
	vController{vController_},
	animate
	{
		[this](IG::FrameParams params)
		{
			window().setNeedsDraw(true);
			//logMsg("updating fade");
			return textFade.update(params.timestamp());
		}
	}
{
	app().applyOSNavStyle(appContext(), true);
	textFade = {1.};
	animationStartTimer.runIn(IG::Seconds{2}, {},
		[this]()
		{
			logMsg("starting fade");
			textFade = {1., 0., {}, SteadyClock::now(), Milliseconds{400}};
			window().addOnFrame(animate);
		});
}

PlaceVControlsView::~PlaceVControlsView()
{
	app().applyOSNavStyle(appContext(), false);
	window().removeOnFrame(animate);
}

void PlaceVControlsView::place()
{
	dragTracker.reset();
	auto exitBtnPos = viewRect().pos(C2DO);
	int exitBtnSize = window().widthMMInPixels(10.);
	exitBtnRect = IG::makeWindowRectRel(exitBtnPos - IG::WP{exitBtnSize/2, exitBtnSize/2}, {exitBtnSize, exitBtnSize});
	text.compile(renderer());
}

bool PlaceVControlsView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &e)
		{
			if(e.pushed() && animationStartTimer.isArmed())
			{
				animationStartTimer.dispatchEarly();
			}
			size_t layoutIdx = window().isPortrait();
			dragTracker.inputEvent(e,
				[&](Input::DragTrackerState, DragData &d)
				{
					if(d.elem)
						return;
					auto tryGrabElement = [&](VControllerElement &elem)
					{
						if(elem.state == VControllerState::OFF || !elem.bounds().contains(e.pos()))
							return false;
						for(const auto &state : dragTracker.stateList())
						{
							if(state.data.elem == &elem)
								return false; // element already grabbed
						}
						d.elem = &elem;
						d.startPos = elem.bounds().pos(C2DO);
						return true;
					};
					for(auto &elem : vController.deviceElements())
					{
						if(tryGrabElement(elem))
							return;
					}
					for(auto &elem : vController.guiElements())
					{
						if(tryGrabElement(elem))
							return;
					}
				},
				[&](Input::DragTrackerState state, Input::DragTrackerState, DragData &d)
				{
					if(d.elem)
					{
						auto newPos = d.startPos + state.downPosDiff();
						auto bounds = window().bounds();
						d.elem->setPos(newPos, bounds);
						auto layoutPos = VControllerLayoutPosition::fromPixelPos(d.elem->bounds().pos(C2DO), d.elem->bounds().size(), bounds);
						//logMsg("set pos %d,%d from %d,%d", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
						auto &vCtrlLayoutPos = d.elem->layoutPos[layoutIdx];
						vCtrlLayoutPos.origin = layoutPos.origin;
						vCtrlLayoutPos.pos = layoutPos.pos;
						app().viewController().placeEmuViews();
						postDraw();
					}
				},
				[&](Input::DragTrackerState state, DragData &d)
				{
					if(!d.elem && exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
					{
						dismiss();
					}
				});
			return true;
		}
	}, e);
}

void PlaceVControlsView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	vController.draw(cmds, true, .75);
	cmds.setColor({.5, .5, .5});
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	const int lineSize = 1;
	cmds.drawRect({{viewRect().x, viewRect().yCenter()},
		{viewRect().x2, viewRect().yCenter() + lineSize}});
	cmds.drawRect({{viewRect().xCenter(), viewRect().y},
		{viewRect().xCenter() + lineSize, viewRect().y2}});

	if(textFade != 0.)
	{
		cmds.setColor({0, 0, 0, textFade / 2.f});
		cmds.drawRect({viewRect().pos(C2DO) - text.pixelSize() / 2 - text.spaceWidth(),
			viewRect().pos(C2DO) + text.pixelSize() / 2 + text.spaceWidth()});
		basicEffect.enableAlphaTexture(cmds);
		text.draw(cmds, viewRect().pos(C2DO), C2DO, Color{1., 1., 1., textFade});
	}
}

}
