#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/gui/ScrollView.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/concepts.hh>
#include <imagine/util/variant.hh>
#include <string_view>

namespace IG::Input
{
class Event;
}

namespace IG
{

class Window;

class TableView : public ScrollView
{
public:
	struct ItemsMessage {const TableView& item;};
	struct GetItemMessage {const TableView& item; size_t idx;};
	using ItemMessageVariant = std::variant<GetItemMessage, ItemsMessage>;
	class ItemMessage: public ItemMessageVariant, public AddVisit
	{
	public:
		using ItemMessageVariant::ItemMessageVariant;
		using AddVisit::visit;
	};
	using ItemReply = std::variant<MenuItem*, size_t>;
	using ItemSourceDelegate = MenuItemSourceDelegate<ItemMessage, ItemReply, ItemsMessage, GetItemMessage>;
	using SelectElementDelegate = DelegateFunc<void (const Input::Event &, int i, MenuItem &)>;

	TableView(UTF16Convertible auto &&name, ViewAttachParams attach, ItemSourceDelegate itemSrc):
		ScrollView{attach}, itemSrc{itemSrc}, nameStr{IG_forward(name)},
		selectQuads{attach.rendererTask, {.size = 1}},
		separatorQuads{attach.rendererTask, {.size = maxSeparators, .usageHint = Gfx::BufferUsageHint::streaming}} {}

	TableView(ViewAttachParams attach, ItemSourceDelegate itemSrc):
		TableView{UTF16String{}, attach, itemSrc} {}

	void prepareDraw() override;
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const override;
	void place() override;
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void resetScroll();
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) override;
	void clearSelection() override;
	void onShow() override;
	void onHide() override;
	void onAddedToController(ViewController *, const Input::Event &) override;
	void setFocus(bool focused) override;
	void setOnSelectElement(SelectElementDelegate del);
	auto& item(this auto&& self, size_t idx) { return self.item(self.itemSrc, idx); }
	size_t cells() const;
	WSize cellSize() const;
	void highlightCell(int idx);
	int highlightedCell() const { return selected; }
	[[nodiscard]] TableUIState saveUIState() const;
	void restoreUIState(TableUIState);
	void setAlign(_2DOrigin align);
	std::u16string_view name() const override;
	void resetName(UTF16Convertible auto &&name) { nameStr = IG_forward(name); }
	void resetName() { nameStr.clear(); }
	void resetItemSource(ItemSourceDelegate src = [](ItemMessage) -> ItemReply { return 0uz; }) { itemSrc = src; }

protected:
	static constexpr size_t maxSeparators = 32;
	ItemSourceDelegate itemSrc;
	SelectElementDelegate selectElementDel{};
	UTF16String nameStr{};
	Gfx::IQuads selectQuads;
	Gfx::IColQuads separatorQuads;
	int yCellSize = 0;
	int selected = -1;
	int visibleCells = 0;
	_2DOrigin align{LC2DO};
	bool onlyScrollIfNeeded = false;
	bool selectedIsActivated = false;
	bool hasFocus = true;

	void setYCellSize(int s);
	WRect focusRect();
	void onSelectElement(const Input::Event &, size_t i, MenuItem &);
	int nextSelectableElement(int start, int items);
	int prevSelectableElement(int start, int items);
	bool handleTableInput(const Input::Event &, bool &movedSelected);
	virtual void drawElement(Gfx::RendererCommands &__restrict__, size_t i, MenuItem &item, WRect rect, int xIndent) const;

	auto& item(this auto&& self, ItemSourceDelegate src, size_t idx)
	{
		return *getAs<MenuItem*>(src(GetItemMessage{self, idx}));
	}
};

}
