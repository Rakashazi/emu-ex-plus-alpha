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
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/utility.h>
#include <imagine/util/string/utf16.hh>
#include <memory>
#include <string_view>

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
struct DocumentPickerEvent;

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
	virtual View* parentView(View&);
};

class View: public ViewI
{
public:
	static constexpr auto imageSamplerConfig = ViewDefs::imageSamplerConfig;
	enum class Direction: uint8_t
	{
		TOP, RIGHT, BOTTOM, LEFT
	};

	constexpr View() = default;

	constexpr View(ViewAttachParams attach):
		win(&attach.window),
		rendererTask_{&attach.rendererTask},
		manager_{&attach.viewManager} {}

	View &operator=(View &&) = delete;
	bool onDocumentPicked(const DocumentPickerEvent&) override;
	void setViewRect(WindowRect viewRect, WindowRect displayRect);
	void setViewRect(WindowRect viewRect);
	void postDraw();
	Window &window() const;
	Gfx::Renderer &renderer() const;
	Gfx::RendererTask &rendererTask() const;
	auto &manager(this auto&& self) { return *self.manager_; }
	ViewAttachParams attachParams() const;
	Screen *screen() const;
	ApplicationContext appContext() const;
	template<class T> T &applicationAs() const { return appContext().applicationAs<T>(); }
	static std::u16string nameString(const MenuItem &item);
	Gfx::GlyphTextureSet &defaultFace();
	Gfx::GlyphTextureSet &defaultBoldFace();
	static Gfx::Color menuTextColor(bool isSelected);
	static int navBarHeight(const Gfx::GlyphTextureSet &face);
	void dismiss(bool refreshLayout = true);
	void dismissPrevious();
	void pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView = true, bool isModal = false);
	void pushAndShowModal(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView = false);
	void popTo(View &v);
	void popTo();
	void show();
	bool moveFocusToNextView(const Input::Event &e, _2DOrigin direction);
	View* parentView();
	void setWindow(Window *w);
	void onDismiss();
	void setController(ViewController *c, const Input::Event &e);
	void setController(ViewController *c);
	ViewController *controller() const;
	WindowRect viewRect() const { return viewRect_; }
	WindowRect displayRect() const { return displayRect_; }
	WindowRect displayInsetRect(Direction) const;
	static WindowRect displayInsetRect(Direction, WindowRect viewRect, WindowRect displayRect);
	bool pointIsInView(WPt pos);

	template<class T>
	std::unique_ptr<T> makeView(auto &&...args)
	{
		return std::make_unique<T>(attachParams(), IG_forward(args)...);
	}

	template<class T>
	std::unique_ptr<T> makeViewWithName(UTF16Convertible auto &&name, auto &&...args)
	{
		return std::make_unique<T>(IG_forward(name), attachParams(), IG_forward(args)...);
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
	WRect viewRect_{};
	WRect displayRect_{};
};

}
