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
#include <imagine/gui/ViewAttachParams.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/NonCopyable.hh>
#include <utility>
#include <memory>
#include <optional>

namespace Base
{
class Window;
class Screen;
class ApplicationContext;
class Application;
}

namespace Input
{
class Event;
}

namespace Gfx
{
class Renderer;
class RendererTask;
}

class View;
class BaseTextMenuItem;

class ViewController : private NonCopyable
{
public:
	constexpr ViewController() {}
	virtual void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal) = 0;
	void pushAndShow(std::unique_ptr<View> v, Input::Event e);
	void pushAndShow(std::unique_ptr<View> v);
	virtual void pop();
	virtual void popAndShow();
	virtual void popTo(View &v) = 0;
	virtual void dismissView(View &v, bool refreshLayout = true) = 0;
	virtual void dismissView(int idx, bool refreshLayout = true) = 0;
	virtual bool inputEvent(Input::Event e);
	virtual bool moveFocusToNextView(Input::Event e, _2DOrigin direction);
};

class ViewManager
{
public:
	static constexpr bool needsBackControlDefault = !Config::envIsAndroid;
	static constexpr bool needsBackControlIsMutable = !Config::envIsIOS;
	static constexpr auto imageCommonTextureSampler = Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP;

	constexpr ViewManager() {}
	ViewManager(Gfx::Renderer &);
	void setDefaultFace(Gfx::GlyphTextureSet);
	void setDefaultBoldFace(Gfx::GlyphTextureSet);
	Gfx::GlyphTextureSet &defaultFace();
	Gfx::GlyphTextureSet &defaultBoldFace();
	constexpr bool needsBackControl() const { return needsBackControl_; }
	void setNeedsBackControl(std::optional<bool>);
	std::optional<bool> needsBackControlOption() const;
	Gfx::GC tableXIndent() const;
	void setTableXIndentMM(float indentMM, Gfx::ProjectionPlane);
	float defaultTableXIndentMM(const Base::Window &);
	void setTableXIndentToDefault(const Base::Window &, Gfx::ProjectionPlane);

protected:
	Gfx::GlyphTextureSet defaultFace_{};
	Gfx::GlyphTextureSet defaultBoldFace_{};
	Gfx::GC tableXIndent_{};
	// True if the platform needs an on-screen/pointer-based control to move to a previous view
	IG_enableMemberIfOrConstant(needsBackControlIsMutable,
		bool, needsBackControlDefault, needsBackControl_){needsBackControlDefault};
};

class View : private NonCopyable
{
public:
	using NameString = Gfx::TextString;
	using NameStringView = Gfx::TextStringView;
	using DismissDelegate = DelegateFunc<bool (View &view)>;
	static constexpr bool needsBackControlDefault = ViewManager::needsBackControlDefault;
	static constexpr bool needsBackControlIsMutable = ViewManager::needsBackControlIsMutable;
	static constexpr auto imageCommonTextureSampler = ViewManager::imageCommonTextureSampler;

	View();
	View(ViewAttachParams attach);
	View(NameString name, ViewAttachParams attach);
	View(const char *name, ViewAttachParams attach);
	virtual ~View();

	virtual void place() = 0;
	virtual void prepareDraw();
	virtual void draw(Gfx::RendererCommands &cmds) = 0;
	virtual bool inputEvent(Input::Event event) = 0;
	virtual void clearSelection(); // de-select any items from previous input
	virtual void onShow();
	virtual void onHide();
	virtual void onAddedToController(ViewController *c, Input::Event e);
	virtual void setFocus(bool focused);

	void setViewRect(IG::WindowRect rect, Gfx::ProjectionPlane projP);
	void setViewRect(Gfx::ProjectionPlane projP);
	void postDraw();
	Base::Window &window() const;
	Gfx::Renderer &renderer() const;
	Gfx::RendererTask &rendererTask() const;
	ViewManager &manager();
	ViewAttachParams attachParams() const;
	Base::Screen *screen() const;
	Base::ApplicationContext appContext() const;
	NameStringView name() const;
	void setName(const char *name);
	void setName(NameString name);
	static NameString makeNameString(const char *name);
	static NameString makeNameString(const BaseTextMenuItem &item);
	Gfx::GlyphTextureSet &defaultFace();
	Gfx::GlyphTextureSet &defaultBoldFace();
	static Gfx::Color menuTextColor(bool isSelected);
	void dismiss(bool refreshLayout = true);
	void dismissPrevious();
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView = true, bool isModal = false);
	void pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView = false);
	void popTo(View &v);
	void show();
	bool moveFocusToNextView(Input::Event e, _2DOrigin direction);
	void setWindow(Base::Window *w);
	void setOnDismiss(DismissDelegate del);
	void onDismiss();
	void setController(ViewController *c, Input::Event e);
	void setController(ViewController *c);
	ViewController *controller() const;
	IG::WindowRect viewRect() const;
	Gfx::ProjectionPlane projection() const;
	bool pointIsInView(IG::WP pos);
	void waitForDrawFinished();

	template<class T, class... Args>
	std::unique_ptr<T> makeView(Args&&... args)
	{
		return std::make_unique<T>(attachParams(), std::forward<Args>(args)...);
	}

	template<class T, class... Args>
	std::unique_ptr<T> makeViewWithName(NameString name, Args&&... args)
	{
		return std::make_unique<T>(std::move(name), attachParams(), std::forward<Args>(args)...);
	}

	template<class T, class... Args>
	std::unique_ptr<T> makeViewWithName(const char *name, Args&&... args)
	{
		return std::make_unique<T>(makeNameString(name), attachParams(), std::forward<Args>(args)...);
	}

	template<class T, class... Args>
	std::unique_ptr<T> makeViewWithName(const BaseTextMenuItem &item, Args&&... args)
	{
		return std::make_unique<T>(makeNameString(item), attachParams(), std::forward<Args>(args)...);
	}

protected:
	Base::Window *win{};
	Gfx::RendererTask *rendererTask_{};
	ViewManager *manager_{};
	ViewController *controller_{};
	NameString nameStr{};
	DismissDelegate dismissDel{};
	IG::WindowRect viewRect_{};
	Gfx::ProjectionPlane projP{};
};
