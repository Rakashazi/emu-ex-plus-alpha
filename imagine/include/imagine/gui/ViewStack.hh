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
#include <vector>

class NavView;

class BasicViewController : public ViewController
{
public:
	using RemoveViewDelegate = DelegateFunc<void ()>;

	BasicViewController();
	RemoveViewDelegate &onRemoveView() { return removeViewDel; }
	void push(std::unique_ptr<View> v, Input::Event e);
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal) override;
	using ViewController::pushAndShow;
	void dismissView(View &v, bool refreshLayout = true) override;
	void dismissView(int idx, bool refreshLayout = true) override;
	void place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP);
	void place();
	bool hasView() { return (bool)view; }
	bool inputEvent(Input::Event e) override;
	void draw(Gfx::RendererCommands &cmds);

protected:
	std::unique_ptr<View> view{};
	RemoveViewDelegate removeViewDel{};
	IG::WindowRect viewRect{};
	Gfx::ProjectionPlane projP{};
};

class ViewStack : public ViewController
{
public:
	ViewStack();
	void setNavView(std::unique_ptr<NavView> nav);
	NavView *navView() const;
	void place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP);
	void place();
	bool inputEvent(Input::Event e) override;
	bool moveFocusToNextView(Input::Event e, _2DOrigin direction) override;
	void prepareDraw();
	void draw(Gfx::RendererCommands &cmds);
	void push(std::unique_ptr<View> v, Input::Event e);
	void push(std::unique_ptr<View> v);
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal) override;
	using ViewController::pushAndShow;
	void pop() override;
	void popAndShow() override;
	void popToRoot();
	void popAll();
	void popTo(View &v) override;
	void popTo(int idx);
	void show();
	View &top() const;
	View &viewAtIdx(uint32_t idx) const;
	int viewIdx(View &v) const;
	int viewIdx(View::NameStringView name) const;
	int viewIdx(const char *name) const;
	bool contains(View &v) const;
	bool contains(View::NameStringView name) const;
	bool contains(const char *name) const;
	void dismissView(View &v, bool refreshLayout = true) override;
	void dismissView(int idx, bool refreshLayout = true) override;
	void showNavView(bool show);
	void setShowNavViewBackButton(bool show);
	uint32_t size() const;
	bool viewHasFocus() const;
	bool hasModalView() const;
	void popModalViews();

protected:
	struct ViewEntry
	{
		ViewEntry(std::unique_ptr<View> v, bool needsNavView):
			v{std::move(v)}, needsNavView{needsNavView}
		{}
		std::unique_ptr<View> v;
		bool needsNavView;
		bool isModal;
	};
	std::vector<ViewEntry> view{};
	std::unique_ptr<NavView> nav{};
	//ViewController *nextController{};
	IG::WindowRect viewRect{}, customViewRect{};
	Gfx::ProjectionPlane projP{};
	bool showNavBackBtn = true;
	bool showNavView_ = true;
	bool navViewHasFocus = false;
	bool changingViewFocus = false;

	void showNavLeftBtn();
	bool topNeedsNavView() const;
	bool navViewIsActive() const;
	void popViews(int num);
};
