#pragma once

class BaseCheatsView : public BaseMenuView
{
private:
	TextMenuItem edit {"Add/Edit", TextMenuItem::SelectDelegate::create<&editHandler>()};

	static void editHandler(TextMenuItem &item, const Input::Event &e)
	{
		editCheatListView.init(!e.isPointer());
		viewStack.pushAndShow(&editCheatListView);
	}

	MenuItem *item[maxCheats + 1] = {nullptr};
public:
	constexpr BaseCheatsView(): BaseMenuView("Cheats") { }

	virtual void loadCheatItems(MenuItem *item[], uint &items) = 0;

	void init(bool highlightFirst)
	{
		uint i = 0;
		edit.init(); item[i++] = &edit;
		loadCheatItems(item, i);
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};
