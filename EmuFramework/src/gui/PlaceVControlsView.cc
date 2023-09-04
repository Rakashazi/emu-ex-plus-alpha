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

namespace EmuEx
{

constexpr std::array snapPxSizes{0, 2, 4, 8, 16, 32, 64};

PlaceVControlsView::PlaceVControlsView(ViewAttachParams attach, VController &vController_):
	View{attach},
	exitText{"Exit", &defaultFace()},
	snapText{"Snap: 0px", &defaultFace()},
	vController{vController_}
{
	app().applyOSNavStyle(appContext(), true);
}

PlaceVControlsView::~PlaceVControlsView()
{
	app().applyOSNavStyle(appContext(), false);
}

void PlaceVControlsView::place()
{
	dragTracker.reset();
	exitText.compile(renderer());
	snapText.compile(renderer());
	exitBtnRect = WRect{{}, exitText.pixelSize()} + (viewRect().pos(C2DO) - exitText.pixelSize() / 2) + WPt{0, exitText.height()};
	snapBtnRect = WRect{{}, snapText.pixelSize()} + (viewRect().pos(C2DO) - snapText.pixelSize() / 2) - WPt{0, exitText.height()};
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
						if(snapPxIdx)
						{
							auto snapPx = snapPxSizes[snapPxIdx];
							auto remainder = newPos % snapPx;
							if(remainder.x)
								newPos.x = newPos.x + snapPx - remainder.x;
							if(remainder.y)
								newPos.y = newPos.y + snapPx - remainder.y;
						}
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
					if(d.elem)
						return;
					if(exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
					{
						dismiss();
					}
					else if(snapBtnRect.overlaps(state.pos()) && snapBtnRect.overlaps(state.downPos()))
					{
						snapPxIdx = (snapPxIdx + 1) % snapPxSizes.size();
						snapText.resetString(std::format("Snap: {}px", snapPxSizes[snapPxIdx]));
						snapText.compile(renderer());
						postDraw();
					}
				});
			return true;
		}
	}, e);
}

void PlaceVControlsView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	vController.draw(cmds, true);
	cmds.setColor({.5, .5, .5});
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	const int lineSize = 1;
	cmds.drawRect({{viewRect().x, viewRect().yCenter()},
		{viewRect().x2, viewRect().yCenter() + lineSize}});
	cmds.drawRect({{viewRect().xCenter(), viewRect().y},
		{viewRect().xCenter() + lineSize, viewRect().y2}});
	cmds.setColor({0, 0, 0, .5});
	cmds.drawRect(exitBtnRect);
	cmds.drawRect(snapBtnRect);
	basicEffect.enableAlphaTexture(cmds);
	exitText.draw(cmds, exitBtnRect.pos(C2DO), C2DO, ColorName::WHITE);
	snapText.draw(cmds, snapBtnRect.pos(C2DO), C2DO, ColorName::WHITE);
}

void PlaceVControlsView::onShow()
{
	vController.applyButtonAlpha(.75);
}

}
