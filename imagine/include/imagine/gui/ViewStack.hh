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
#include <imagine/gui/View.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/gfx/Quads.hh>
#include <vector>
#include <memory>
#include <string_view>

namespace IG
{

class BasicViewController : public ViewController
{
public:
	using RemoveViewDelegate = DelegateFunc<void ()>;

	constexpr BasicViewController() = default;
	RemoveViewDelegate &onRemoveView() { return removeViewDel; }
	void push(std::unique_ptr<View>, const Input::Event &);
	void pushAndShow(std::unique_ptr<View>, const Input::Event &, bool needsNavView, bool isModal) override;
	using ViewController::pushAndShow;
	void dismissView(View &v, bool refreshLayout = true) override;
	void dismissView(int idx, bool refreshLayout = true) override;
	void place(const WRect &rect);
	void place();
	bool hasView() { return (bool)view; }
	bool inputEvent(const Input::Event &) override;
	void draw(Gfx::RendererCommands &cmds);

protected:
	std::unique_ptr<View> view;
	RemoveViewDelegate removeViewDel;
	WRect viewRect{};
};

class ViewStack : public ViewController
{
public:
	ViewStack(ViewAttachParams);
	void setNavView(std::unique_ptr<NavView> nav);
	NavView *navView() const;
	void place(WindowRect viewRect, WindowRect displayRect);
	void place();
	bool inputEvent(const Input::Event &) override;
	bool moveFocusToNextView(const Input::Event &, _2DOrigin direction) override;
	void prepareDraw();
	void draw(Gfx::RendererCommands &cmds);
	void push(std::unique_ptr<View>, const Input::Event &);
	void push(std::unique_ptr<View>);
	void pushAndShow(std::unique_ptr<View>, const Input::Event &, bool needsNavView, bool isModal) override;
	using ViewController::pushAndShow;
	void pop() override;
	void popAndShow() override;
	void popToRoot();
	void popAll();
	void popTo(View &v) override;
	void popTo(int idx);
	void show();
	View* parentView(View& v) override;
	View &top() const;
	View &viewAtIdx(int idx) const;
	int viewIdx(View &v) const;
	int viewIdx(std::u16string_view name) const;
	int viewIdx(std::string_view name) const;
	bool contains(View &v) const;
	bool contains(std::u16string_view name) const;
	bool contains(std::string_view name) const;
	void dismissView(View &v, bool refreshLayout = true) override;
	void dismissView(int idx, bool refreshLayout = true) override;
	void showNavView(bool show);
	void setShowNavViewBackButton(bool show);
	size_t size() const;
	bool viewHasFocus() const;
	bool hasModalView() const;
	void popModalViews();

protected:
	struct ViewEntry
	{
		std::unique_ptr<View> ptr;
		bool needsNavView{};
		bool isModal{};
	};
	std::vector<ViewEntry> view;
	std::unique_ptr<NavView> nav;
	//ViewController *nextController{};
	WindowRect viewRect{}, customViewRect{};
	WindowRect displayRect{}, customDisplayRect{};
	Gfx::IColQuads bottomGradientQuads;
	bool showNavBackBtn = true;
	bool showNavView_ = true;
	bool navViewHasFocus = false;
	bool changingViewFocus = false;

	void showNavLeftBtn();
	bool topNeedsNavView() const;
	bool navViewIsActive() const;
	void popViews(size_t num);
};

}
