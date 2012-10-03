#pragma once

#include <gfx/GfxText.hh>
#include <gui/View.hh>

class TextEntry
{
public:
	constexpr TextEntry() { }
	Rect2<int> b;
	GfxText t;
	char str[128] = {0};
	bool acceptingInput = 0;
	bool multiLine = 0;

	CallResult init(const char *initText, ResourceFace *face);
	void deinit();
	void setAcceptingInput(bool on);
	void inputEvent(const InputEvent &e);
	void draw();
	void place();
	void place(Rect2<int> rect);
};

class CollectTextInputView : public View
{
public:
	constexpr CollectTextInputView(): View("Text Entry") { }

	Rect2<int> cancelBtn;
	GfxSprite cancelSpr;
	GfxText message;
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	TextEntry textEntry;
	#endif

	// returning non-zero keeps text entry active on Android
	typedef Delegate<uint (const char *str)> OnTextDelegate;
	OnTextDelegate onTextDel;
	OnTextDelegate &onTextDelegate() { return onTextDel; }

	Rect2<int> rect;
	Rect2<int> &viewRect() { return rect; }

	void init(const char *msgText, const char *initialContent = "");
	void deinit();
	void place();
	void inputEvent(const InputEvent &e);
	void draw();

	#ifdef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	void gotText(const char *str);
	#endif
};
