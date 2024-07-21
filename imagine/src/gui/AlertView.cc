/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/AlertView.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/input/Event.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math.hh>

namespace IG
{

constexpr SystemLogger log{"AlertView"};

void BaseAlertView::init()
{
	menu.setAlign(C2DO);
	menu.setScrollableIfNeeded(true);
	menu.setOnSelectElement([this](const Input::Event& e, int i, MenuItem& item)
	{
		bool shouldDismiss = item.inputEvent(e, {.parentPtr = this});
		if(shouldDismiss)
		{
			log.info("dismissing via item #{}", i);
			dismiss();
		}
	});
}

void BaseAlertView::place()
{
	using namespace IG::Gfx;
	int xSize = viewRect().xSize() * .8;
	text.compile({.maxLineSize = int(xSize * 0.95f)});

	int menuYSize = menu.cells() * text.face()->nominalHeight()*2;
	int labelYSize = IG::makeEvenRoundedUp(text.fullHeight());
	WRect viewFrame;
	viewFrame.setPosRel(viewRect().pos(C2DO),
			{xSize, labelYSize + menuYSize}, C2DO);

	labelFrame = {{viewFrame.x, viewFrame.y}, {viewFrame.x2, viewFrame.y + labelYSize}};
	bgQuads.write(0, {.bounds = labelFrame.as<int16_t>()});

	WRect menuViewFrame;
	menuViewFrame.setPosRel({viewFrame.x, viewFrame.y + (int)labelYSize},
			{viewFrame.xSize(), menuYSize}, LT2DO);
	menu.setViewRect(menuViewFrame);
	menu.place();
	bgQuads.write(1, {.bounds = menu.viewRect().as<int16_t>()});
}

bool BaseAlertView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	if(e.keyEvent() && e.keyEvent()->pushed(Input::DefaultKey::CANCEL))
	{
		dismiss();
		return true;
	}
	return menu.inputEvent(e);
}

void BaseAlertView::prepareDraw()
{
	text.makeGlyphs();
	menu.prepareDraw();
}

void BaseAlertView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::ALPHA);
	basicEffect.disableTexture(cmds);
	cmds.setVertexArray(bgQuads);
	cmds.setColor({.4, .4, .4, .8});
	cmds.drawQuad(0);
	cmds.setColor({.1, .1, .1, .6});
	cmds.drawQuad(1);
	basicEffect.enableAlphaTexture(cmds);
	text.draw(cmds, {labelFrame.xPos(C2DO), labelFrame.yPos(C2DO)}, C2DO, ColorName::WHITE);
	menu.draw(cmds);
}

void BaseAlertView::onAddedToController(ViewController *c, const Input::Event &e)
{
	menu.setController(c, e);
}

void YesNoAlertView::setOnYes(TextMenuItem::SelectDelegate del)
{
	yesNo[0].onSelect = del;
}

void YesNoAlertView::setOnNo(TextMenuItem::SelectDelegate del)
{
	yesNo[1].onSelect = del;
}

}
