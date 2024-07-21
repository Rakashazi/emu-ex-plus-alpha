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
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/utility.h>
#include <span>

namespace IG::ViewDefs
{

constexpr bool needsBackControlDefault = !Config::envIsAndroid;
constexpr bool needsBackControlIsMutable = !Config::envIsIOS;
constexpr auto imageSamplerConfig = Gfx::SamplerConfigs::nearestMipClamp;

}

namespace IG
{

class ApplicationContext;
class Window;
class View;
class ViewController;
class ViewManager;
class TableView;
class MenuItem;

struct ViewAttachParams
{
	ViewManager &viewManager;
	Window &window;
	Gfx::RendererTask &rendererTask;

	Gfx::Renderer &renderer() const;
	ApplicationContext appContext() const;
};

struct TableUIState
{
	int highlightedCell{-1};
	int scrollOffset{};
};

template <class ItemMessage, class ItemReply, class ItemsMessage, class GetItemMessage>
class MenuItemSourceDelegate : public DelegateFunc<ItemReply (ItemMessage)>
{
public:
	using DelegateFuncBase = DelegateFunc<ItemReply (ItemMessage)>;
	using DelegateFuncBase::DelegateFuncBase;

	constexpr MenuItemSourceDelegate(Container auto&& items):
		DelegateFuncBase{itemsDelegate(IG_forward(items))} {}

	static constexpr ItemReply handleItemMessage(const ItemMessage& msg, auto& items)
	{
		return visit(overloaded
		{
			[&](const ItemsMessage&) -> ItemReply { return std::size(items); },
			[&](const GetItemMessage& m) -> ItemReply
			{
				auto itemPtr = &indirect(std::data(items)[m.idx]);
				if constexpr(requires {itemPtr->menuItem();})
					return &itemPtr->menuItem();
				else
					return itemPtr;
			},
		}, msg);
	};

	template<Container T>
	static constexpr MenuItemSourceDelegate itemsDelegate(T&& items)
	{
		if constexpr(std::is_rvalue_reference_v<T>)
		{
			return [items](ItemMessage msg) { return handleItemMessage(msg, items); };
		}
		else
		{
			return [&items](ItemMessage msg) { return handleItemMessage(msg, items); };
		}
	}

	template<class T>
	static constexpr MenuItemSourceDelegate itemsDelegate(std::span<T> items)
	{
		return [items](ItemMessage msg) { return handleItemMessage(msg, items); };
	}
};

struct ViewInputEventParams
{
	View* parentPtr{};
};

struct ViewDrawParams{};

struct ViewI
{
	constexpr ViewI() = default;
	virtual ~ViewI() = default;
	virtual void place() = 0;
	virtual void prepareDraw();
	virtual void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const = 0;
	virtual bool inputEvent(const Input::Event&, ViewInputEventParams p = {});
	virtual void clearSelection(); // de-select any items from previous input
	virtual void onShow();
	virtual void onHide();
	virtual void onAddedToController(ViewController*, const Input::Event&);
	virtual void setFocus(bool focused);
	virtual std::u16string_view name() const;
	virtual bool onDocumentPicked(const DocumentPickerEvent&);
};

}
