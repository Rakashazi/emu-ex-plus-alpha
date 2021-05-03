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

#define LOGTAG "BaseAlertView"

#include <imagine/gui/AlertView.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>

BaseAlertView::BaseAlertView(ViewAttachParams attach, const char *label, TableView::ItemsDelegate items, TableView::ItemDelegate item):
	View{attach},
	text{label, &attach.viewManager().defaultFace()},
	menu
	{
		attach,
		items,
		item
	}
{
	menu.setAlign(C2DO);
	menu.setScrollableIfNeeded(true);
	menu.setOnSelectElement(
		[this](Input::Event e, uint32_t i, MenuItem &item)
		{
			bool wasDismissed = false;
			setOnDismiss(
				[&wasDismissed](View &)
				{
					logMsg("called alert onDismiss");
					wasDismissed = true;
					return false;
				});
			bool shouldDismiss = item.select(*this, e);
			if(wasDismissed)
			{
				logDMsg("dismissed in onSelect");
				return;
			}
			setOnDismiss(nullptr);
			if(shouldDismiss)
			{
				logMsg("dismissing");
				dismiss();
			}
		});
}

void BaseAlertView::place()
{
	using namespace Gfx;
	int xSize = viewRect().xSize() * .8;
	text.setMaxLineSize(projP.unprojectXSize(xSize) * 0.95_gc);
	text.compile(renderer(), projP);

	int menuYSize = menu.cells() * text.face()->nominalHeight()*2;
	int labelYSize = IG::makeEvenRoundedUp(projP.projectYSize(text.fullHeight()));
	IG::WindowRect viewFrame;
	viewFrame.setPosRel(viewRect().pos(C2DO),
			{xSize, labelYSize + menuYSize}, C2DO);

	labelFrame = projP.unProjectRect(viewFrame.x, viewFrame.y, viewFrame.x2, viewFrame.y + labelYSize);

	IG::WindowRect menuViewFrame;
	menuViewFrame.setPosRel({viewFrame.x, viewFrame.y + (int)labelYSize},
			{viewFrame.xSize(), menuYSize}, LT2DO);
	menu.setViewRect(menuViewFrame, projP);
	menu.place();
}

bool BaseAlertView::inputEvent(Input::Event e)
{
	if(e.pushed() && e.isDefaultCancelButton())
	{
		dismiss();
		return true;
	}
	return menu.inputEvent(e);
}

void BaseAlertView::prepareDraw()
{
	text.makeGlyphs(renderer());
	menu.prepareDraw();
}

void BaseAlertView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	cmds.setColor(.4, .4, .4, .8);
	GeomRect::draw(cmds, labelFrame);
	cmds.setColor(.1, .1, .1, .6);
	GeomRect::draw(cmds, menu.viewRect(), projP);
	cmds.set(ColorName::WHITE);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	text.draw(cmds, labelFrame.xPos(C2DO), projP.alignYToPixel(labelFrame.yPos(C2DO)), C2DO, projP);
	//setClipRect(1);
	//setClipRectBounds(menu.viewRect());
	menu.draw(cmds);
	//setClipRect(0);
}

void BaseAlertView::onAddedToController(ViewController *c, Input::Event e)
{
	menu.setController(c, e);
}

void BaseAlertView::setLabel(const char *label)
{
	text.setString(label);
}

AlertView::AlertView(ViewAttachParams attach, const char *label, uint32_t menuItems):
	BaseAlertView{attach, label, item},
	item{menuItems}
{}

void AlertView::setItem(uint32_t idx, const char *name, TextMenuItem::SelectDelegate del)
{
	assert(idx < item.size());
	item[idx].setName(name, &manager().defaultFace());
	item[idx].setOnSelect(del);
}

YesNoAlertView::YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
	TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo):
	BaseAlertView(attach, label,
		[](const TableView &)
		{
			return 2;
		},
		[this](const TableView &, int idx) -> MenuItem&
		{
			return idx == 0 ? yes : no;
		}),
	yes{yesStr ? yesStr : "Yes", &defaultFace(), onYes ? onYes : makeDefaultSelectDelegate()},
	no{noStr ? noStr : "No", &defaultFace(), onNo ? onNo : makeDefaultSelectDelegate()}
{}

void YesNoAlertView::setOnYes(TextMenuItem::SelectDelegate del)
{
	yes.setOnSelect(del);
}

void YesNoAlertView::setOnNo(TextMenuItem::SelectDelegate del)
{
	no.setOnSelect(del);
}

TextMenuItem::SelectDelegate YesNoAlertView::makeDefaultSelectDelegate()
{
	return TextMenuItem::makeSelectDelegate([](){});
}
