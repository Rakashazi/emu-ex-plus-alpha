#pragma once

#include <base/Base.hh>
#include <input/Input.hh>
#include <input/DragPointer.hh>
#include <resource2/face/ResourceFace.hh>

class ViewAnimation
{
public:
	constexpr ViewAnimation() { }
	virtual void initShow() = 0;
	virtual void initActive() = 0;
	virtual void initDismiss() = 0;
	virtual bool update() = 0;
};

class View
{
public:
	constexpr View() { }
	constexpr View(const char *name) : name_(name) { }

	virtual void deinit() = 0;
	virtual Rect2<int> &viewRect() = 0;
	virtual void place() = 0;
	virtual void draw(Gfx::FrameTimeBase frameTime) = 0;
	virtual void inputEvent(const Input::Event &event) = 0;
	virtual void clearSelection() { } // de-select any items from previous input
	virtual void onShow() { }

	void placeRect(Rect2<int> rect)
	{
		this->viewRect() = rect;
		place();
	}

	static View *modalView;
	typedef Delegate<void ()> RemoveModalViewDelegate;
	static RemoveModalViewDelegate removeModalViewDel;
	static RemoveModalViewDelegate &removeModalViewDelegate() { return removeModalViewDel; }
	static void removeModalView()
	{
		assert(modalView);
		modalView->deinit();
		modalView = nullptr;
		removeModalViewDel.invokeSafe();
		Base::displayNeedsUpdate();
	}

	static ResourceFace *defaultFace;

	enum { SHOW, ACTIVE, HIDE };
	ViewAnimation *animation = nullptr;
	uint displayState = 0;

	const char *name_ = "";
	const char *name() { return name_; }

	// Does the platform need an on-screen/pointer-based control to move to a previous view?
	static bool needsBackControl;
	static const bool needsBackControlDefault = !(Config::envIsPS3 || Config::envIsAndroid || (Config::envIsWebOS && !Config::envIsWebOS3));
	static const bool needsBackControlIsConst = Config::envIsPS3 || Config::envIsIOS || Config::envIsWebOS3;

	static void setNeedsBackControl(bool on)
	{
		if(!needsBackControlIsConst) // only modify on environments that make sense
		{
			needsBackControl = on;
		}
	}

	void doDismiss()
	{
		//logMsg("dimiss view with hanlder %p", dismissHandler);
		deinit();
		/*if(dismissHandler)
			dismissHandler();*/
	}

	void dismiss()
	{
		if(!animation)
		{
			doDismiss();
		}
		else
		{
			animation->initDismiss();
			Base::displayNeedsUpdate();
		}
		displayState = HIDE;
	}

	void show(bool animated = 1)
	{
		if(animated && animation)
		{
			animation->initShow();
			displayState = SHOW;
		}
		else
		{
			if(animation)
				animation->initActive();
			displayState = ACTIVE;
		}
		onShow();
		//logMsg("showed view");
		Base::displayNeedsUpdate();
	}

	void init(ViewAnimation *animation = 0, bool animated = 0)
	{
		this->animation = animation;
		//show(animated);
	}

	bool updateAnimation()
	{
		if(animation && animation->update())
		{
			// still animating
			Base::displayNeedsUpdate();
			return 1;
		}

		if(displayState == HIDE)
		{
			// hide animation done, trigger view to dismiss
			doDismiss();
			return 0;
		}

		if(displayState == SHOW)
		{
			// show animation complete
			displayState = ACTIVE;
		}

		// not animating, view is active
		return 1;
	}
};
