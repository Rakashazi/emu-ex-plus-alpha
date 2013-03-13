#include <Cheats.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <util/gui/ViewStack.hh>
#include <main/EmuCheatViews.hh>

extern MsgPopup popup;
extern CollectTextInputView textInputView;
extern ViewStack viewStack;
SystemEditCheatView editCheatView;
EditCheatListView editCheatListView;
CheatsView cheatsMenu;

void BaseCheatsView::editHandler(TextMenuItem &item, const Input::Event &e)
{
	editCheatListView.init(!e.isPointer());
	viewStack.pushAndShow(&editCheatListView);
}

void BaseCheatsView::init(bool highlightFirst)
{
	uint i = 0;
	edit.init(); item[i++] = &edit;
	loadCheatItems(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

uint EditCheatView::handleNameFromTextInput(const char *str)
{
	if(str)
	{
		logMsg("setting cheat name %s", str);
		renamed(str);
		name.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void EditCheatView::nameHandler(TextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input description", name.t.str);
	textInputView.onTextDelegate().bind<template_mfunc(EditCheatView, handleNameFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void EditCheatView::removeHandler(TextMenuItem &item, const Input::Event &e)
{
	removed();
	viewStack.popAndShow();
}

void EditCheatView::loadNameItem(const char *nameStr, MenuItem *item[], uint &items)
{
	name.init(nameStr); item[items++] = &name;
}

void EditCheatView::loadRemoveItem(MenuItem *item[], uint &items)
{
	remove.init(); item[items++] = &remove;
}

void refreshCheatViews()
{
	editCheatListView.deinit();
	editCheatListView.init(0);
	editCheatListView.place();
	cheatsMenu.deinit();
	cheatsMenu.init(0);
	cheatsMenu.place();
}
