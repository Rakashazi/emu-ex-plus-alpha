#include <Cheats.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <EmuApp.hh>
#include "EmuCheatViews.hh"
#include <gba/Cheats.h>
static bool cheatsModified = false;

void SystemEditCheatView::renamed(const char *str)
{
	cheatsModified = true;
	auto &cheat = cheatsList[idx];
	string_copy(cheat.desc, str);
	name.t.setString(cheat.desc);
	name.compile();
}

void SystemEditCheatView::removed()
{
	cheatsModified = true;
	cheatsDelete(gGba.cpu, idx, true);
	refreshCheatViews();
}

void SystemEditCheatView::init(bool highlightFirst, int cheatIdx)
{
	idx = cheatIdx;
	auto &cheat = cheatsList[idx];

	uint i = 0;
	loadNameItem(cheat.desc, item, i);

	name_ = "Edit Code";
	code.init(cheat.codestring); item[i++] = &code;

	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

SystemEditCheatView::SystemEditCheatView(Base::Window &win): EditCheatView("", win),
	code
	{
		"Code",
		[this](DualTextMenuItem &item, const Input::Event &e)
		{
			popup.post("To change this cheat, please delete and re-add it");
		}
	}
{}

void EditCheatListView::loadAddCheatItems(MenuItem *item[], uint &items)
{
	addGS12CBCode.init(); item[items++] = &addGS12CBCode;
	addGS3Code.init(); item[items++] = &addGS3Code;
}

void EditCheatListView::loadCheatItems(MenuItem *item[], uint &items)
{
	uint cheats = std::min((uint)cheatsNumber, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		cheat[c].init(cheatsList[c].desc); item[items++] = &cheat[c];
		cheat[c].onSelect() =
			[this, c](TextMenuItem &, const Input::Event &e)
			{
				auto &editCheatView = *menuAllocator.allocNew<SystemEditCheatView>(window());
				editCheatView.init(!e.isPointer(), c);
				viewStack.pushAndShow(editCheatView, &menuAllocator);
			};
	}
}

void EditCheatListView::addNewCheat(int isGSv3)
{
	if(cheatsNumber == EmuCheats::MAX)
	{
		popup.postError("Too many cheats, delete some first");
		window().postDraw();
		return;
	}
	auto &textInputView = *allocModalView<CollectTextInputView>(window());
	textInputView.init(isGSv3 ? "Input xxxxxxxx yyyyyyyy" : "Input xxxxxxxx yyyyyyyy (GS) or xxxxxxxx yyyy (AR)", getCollectTextCloseAsset());
	textInputView.onText() =
		[this, isGSv3](CollectTextInputView &view, const char *str)
		{
			if(str)
			{
				char tempStr[20];
				string_copy(tempStr, str);
				string_toUpper(tempStr);
				if(strlen(tempStr) == 17 && tempStr[8] == ' ')
				{
					logMsg("removing middle space in text");
					memmove(&tempStr[8], &tempStr[9], 9); // 8 chars + null byte
				}
				if(isGSv3 ?
					cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", true) :
					((strlen(tempStr) == 16 && cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", false))
					|| cheatsAddCBACode(gGba.cpu, tempStr, "Unnamed Cheat")))
				{
					logMsg("added new cheat, %d total", cheatsNumber);
				}
				else
				{
					popup.postError("Invalid format");
					window().postDraw();
					return 1;
				}
				cheatsModified = true;
				cheatsDisable(gGba.cpu, cheatsNumber-1);
				view.dismiss();
				refreshCheatViews();

				auto &textInputView = *allocModalView<CollectTextInputView>(window());
				textInputView.init("Input description", getCollectTextCloseAsset());
				textInputView.onText() =
					[this](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							string_copy(cheatsList[cheatsNumber-1].desc, str);
							view.dismiss();
							refreshCheatViews();
						}
						else
						{
							view.dismiss();
						}
						return 0;
					};
				modalViewController.pushAndShow(textInputView);
			}
			else
			{
				view.dismiss();
			}
			return 0;
		};
	modalViewController.pushAndShow(textInputView);
}

EditCheatListView::EditCheatListView(Base::Window &win):
	BaseEditCheatListView(win),
	addGS12CBCode
	{
		"Add Game Shark v1-2/Code Breaker Code",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			addNewCheat(false);
		}
	},
	addGS3Code
	{
		"Add Game Shark v3 Code",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			addNewCheat(true);
		}
	}
{}

void CheatsView::loadCheatItems(MenuItem *item[], uint &i)
{
	uint cheats = std::min((uint)cheatsNumber, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		auto &cheatEntry = cheatsList[c];
		cheat[c].init(cheatEntry.desc, cheatEntry.enabled); item[i++] = &cheat[c];
		cheat[c].onSelect() =
			[this, c](BoolMenuItem &item, const Input::Event &e)
			{
				cheatsModified = true;
				item.toggle(*this);
				if(item.on)
					cheatsEnable(c);
				else
					cheatsDisable(gGba.cpu, c);
			};
	}
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	FsSys::cPath filename;
	string_printf(filename, "%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName);

	if(!cheatsNumber)
	{
		logMsg("deleting cheats file %s", filename);
		FsSys::remove(filename);
		cheatsModified = false;
		return;
	}
	cheatsSaveCheatList(filename);
	cheatsModified = false;
}

void readCheatFile()
{
	FsSys::cPath filename;
	sprintf(filename, "%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName);
	if(cheatsLoadCheatList(filename))
	{
		logMsg("loaded cheat file: %s", filename);
	}
}
