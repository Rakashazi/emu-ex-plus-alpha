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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>

namespace IG
{

BaseAlertView::BaseAlertView(ViewAttachParams attach, IG::utf16String label, TableView::ItemsDelegate items, TableView::ItemDelegate item):
	View{attach},
	text{std::move(label), &attach.viewManager().defaultFace()},
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
		[this](Input::Event e, int i, MenuItem &item)
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
	using namespace IG::Gfx;
	int xSize = viewRect().xSize() * .8;
	text.setMaxLineSize(projP.unprojectXSize(xSize) * 0.95f);
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
	using namespace IG::Gfx;
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

void BaseAlertView::setLabel(IG::utf16String label)
{
	text.setString(std::move(label));
}

AlertView::AlertView(ViewAttachParams attach, IG::utf16String label, unsigned menuItems):
	BaseAlertView{attach, std::move(label), item},
	item{menuItems}
{}

void AlertView::setItem(size_t idx, IG::utf16String name, TextMenuItem::SelectDelegate del)
{
	assert(idx < item.size());
	item[idx].setName(std::move(name), &manager().defaultFace());
	item[idx].setOnSelect(del);
}

YesNoAlertView::YesNoAlertView(ViewAttachParams attach, IG::utf16String label,
	IG::utf16String yesStr, IG::utf16String noStr,
	TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo):
	BaseAlertView(attach, std::move(label),
		[](const TableView &) -> size_t
		{
			return 2;
		},
		[this](const TableView &, size_t idx) -> MenuItem&
		{
			return idx == 0 ? yes : no;
		}),
	yes{yesStr.size() ? std::move(yesStr) : u"Yes", &defaultFace(), onYes ? onYes : makeDefaultSelectDelegate()},
	no{noStr.size() ? std::move(noStr) : u"No", &defaultFace(), onNo ? onNo : makeDefaultSelectDelegate()}
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
	return [](){};
}

}
