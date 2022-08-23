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

#include <imagine/gui/viewDefs.hh>
#include <imagine/gui/ViewAttachParams.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/utility.h>
#include <imagine/util/string/utf16.hh>
#include <memory>

namespace IG::Input
{
class Event;
}

namespace IG::Gfx
{
class Renderer;
class RendererTask;
class GlyphTextureSet;
}

namespace IG
{

class Window;
class Screen;
class ApplicationContext;
class Application;
class View;
class ViewManager;
class MenuItem;

class ViewController
{
public:
	constexpr ViewController() = default;
	ViewController &operator=(ViewController &&) = delete;
	virtual void pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView, bool isModal) = 0;
	void pushAndShow(std::unique_ptr<View> v, const Input::Event &e);
	void pushAndShow(std::unique_ptr<View> v);
	virtual void pop();
	virtual void popAndShow();
	virtual void popTo(View &v) = 0;
	virtual void dismissView(View &v, bool refreshLayout = true) = 0;
	virtual void dismissView(int idx, bool refreshLayout = true) = 0;
	virtual bool inputEvent(const Input::Event &e);
	virtual bool moveFocusToNextView(const Input::Event &e, _2DOrigin direction);
};

class View
{
public:
	using DismissDelegate = DelegateFunc<bool (View &view)>;
	static constexpr auto imageCommonTextureSampler = ViewDefs::imageCommonTextureSampler;
	enum class Direction: uint8_t
	{
		TOP, RIGHT, BOTTOM, LEFT
	};

	constexpr View() = default;

	constexpr View(ViewAttachParams attach):
		win(&attach.window()),
		rendererTask_{&attach.rendererTask()},
		manager_{&attach.viewManager()} {}

	virtual ~View() = default;
	View &operator=(View &&) = delete;
	virtual void place() = 0;
	virtual void prepareDraw();
	virtual void draw(Gfx::RendererCommands &__restrict__) = 0;
	virtual bool inputEvent(const Input::Event &event) = 0;
	virtual void clearSelection(); // de-select any items from previous input
	virtual void onShow();
	virtual void onHide();
	virtual void onAddedToController(ViewController *c, const Input::Event &e);
	virtual void setFocus(bool focused);
	virtual std::u16string_view name() const;

	void setViewRect(WindowRect viewRect, WindowRect displayRect, Gfx::ProjectionPlane);
	void setViewRect(WindowRect viewRect, Gfx::ProjectionPlane);
	void setViewRect(Gfx::ProjectionPlane projP);
	void postDraw();
	Window &window() const;
	Gfx::Renderer &renderer() const;
	Gfx::RendererTask &rendererTask() const;
	ViewManager &manager();
	ViewAttachParams attachParams() const;
	Screen *screen() const;
	ApplicationContext appContext() const;
	static std::u16string nameString(const MenuItem &item);
	Gfx::GlyphTextureSet &defaultFace();
	Gfx::GlyphTextureSet &defaultBoldFace();
	static Gfx::Color menuTextColor(bool isSelected);
	void dismiss(bool refreshLayout = true);
	void dismissPrevious();
	void pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView = true, bool isModal = false);
	void pushAndShowModal(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView = false);
	void popTo(View &v);
	void show();
	bool moveFocusToNextView(const Input::Event &e, _2DOrigin direction);
	void setWindow(Window *w);
	void setOnDismiss(DismissDelegate del);
	void onDismiss();
	void setController(ViewController *c, const Input::Event &e);
	void setController(ViewController *c);
	ViewController *controller() const;
	WindowRect viewRect() const { return viewRect_; }
	WindowRect displayRect() const { return displayRect_; }
	WindowRect displayInsetRect(Direction) const;
	static WindowRect displayInsetRect(Direction, WindowRect viewRect, WindowRect displayRect);
	Gfx::ProjectionPlane projection() const { return projP; }
	bool pointIsInView(IG::WP pos);
	void waitForDrawFinished();

	template<class T>
	std::unique_ptr<T> makeView(auto &&...args)
	{
		return std::make_unique<T>(attachParams(), IG_forward(args)...);
	}

	template<class T>
	std::unique_ptr<T> makeViewWithName(IG::utf16String name, auto &&...args)
	{
		return std::make_unique<T>(std::move(name), attachParams(), IG_forward(args)...);
	}

	template<class T>
	std::unique_ptr<T> makeViewWithName(const MenuItem &item, auto &&...args)
	{
		return std::make_unique<T>(nameString(item), attachParams(), IG_forward(args)...);
	}

protected:
	Window *win{};
	Gfx::RendererTask *rendererTask_{};
	ViewManager *manager_{};
	ViewController *controller_{};
	DismissDelegate dismissDel{};
	IG::WindowRect viewRect_{};
	IG::WindowRect displayRect_{};
	Gfx::ProjectionPlane projP{};
};

}
