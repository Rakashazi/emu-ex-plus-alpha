#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/TextEntry.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <cheats.h>
extern CollectTextInputView textInputView;

void SystemEditCheatView::renamed(const char *str)
{
	string_copy(Cheat.c[idx].name, str);
	name.t.setString(Cheat.c[idx].name);
	name.compile(projP);
}

void SystemEditCheatView::removed()
{
	S9xDeleteCheat(idx);
	refreshCheatViews();
}

void SystemEditCheatView::init(bool highlightFirst, int cheatIdx)
{
	idx = cheatIdx;
	auto &cheat = Cheat.c[idx];
	logMsg("got cheat with addr 0x%.6x val 0x%.2x saved val 0x%.2x", cheat.address, cheat.byte, cheat.saved_byte);

	uint i = 0;
	loadNameItem(cheat.name, item, i);

	name_ = "Edit Address/Values";
	string_printf(addrStr, "%x", cheat.address);
	addr.init(addrStr); item[i++] = &addr;
	string_printf(valueStr, "%x", cheat.byte);
	value.init(valueStr); item[i++] = &value;
	if(!cheat.saved)
		savedStr[0] = 0;
	else
		string_printf(savedStr, "%x", cheat.saved_byte);
	saved.init(savedStr); item[i++] = &saved;

	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	TableView::init(item, i, highlightFirst);
}

SystemEditCheatView::SystemEditCheatView(Base::Window &win): EditCheatView("", win),
	addr
	{
		"Address",
		[this](DualTextMenuItem &item, View &, const Input::Event &e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 6-digit hex", addrStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = strtoul(str, nullptr, 16);
						if(a > 0xFFFFFF)
						{
							logMsg("addr 0x%X too large", a);
							popup.postError("Invalid input");
							window().postDraw();
							return 1;
						}
						string_copy(addrStr, a ? str : "0", sizeof(addrStr));
						auto wasEnabled = Cheat.c[idx].enabled;
						if(wasEnabled)
						{
							S9xDisableCheat(idx);
						}
						Cheat.c[idx].address = a;
						if(wasEnabled)
						{
							S9xEnableCheat(idx);
						}
						addr.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView);
		}
	},
	value
	{
		"Value",
		[this](DualTextMenuItem &item, View &, const Input::Event &e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 2-digit hex", valueStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = strtoul(str, nullptr, 16);
						if(a > 0xFF)
						{
							popup.postError("value must be <= FF");
							window().postDraw();
							return 1;
						}
						string_copy(valueStr, a ? str : "0", sizeof(valueStr));
						auto wasEnabled = Cheat.c[idx].enabled;
						if(wasEnabled)
						{
							S9xDisableCheat(idx);
						}
						Cheat.c[idx].byte = a;
						if(wasEnabled)
						{
							S9xEnableCheat(idx);
						}
						value.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView);
		}
	},
	saved
	{
		"Saved Value",
		[this](DualTextMenuItem &item, View &, const Input::Event &e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 2-digit hex or blank", savedStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = 0x100;
						if(strlen(str))
						{
							uint a = strtoul(str, nullptr, 16);
							if(a > 0xFF)
							{
								popup.postError("value must be <= FF");
								window().postDraw();
								return 1;
							}
							string_copy(savedStr, str, sizeof(savedStr));
						}
						else
						{
							savedStr[0] = 0;
						}
						auto wasEnabled = Cheat.c[idx].enabled;
						if(wasEnabled)
						{
							S9xDisableCheat(idx);
						}
						if(a <= 0xFF)
						{
							Cheat.c[idx].saved = 1;
							Cheat.c[idx].saved_byte = a;
						}
						else
						{
							Cheat.c[idx].saved = 0;
							Cheat.c[idx].saved_byte = 0;
						}
						if(wasEnabled)
						{
							S9xEnableCheat(idx);
						}
						saved.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView);
		}
	}
{}

void EditCheatListView::loadAddCheatItems(MenuItem *item[], uint &items)
{
	addCode.init(); item[items++] = &addCode;
}

void EditCheatListView::loadCheatItems(MenuItem *item[], uint &items)
{
	int cheats = std::min(Cheat.num_cheats, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		cheat[c].init(Cheat.c[c].name); item[items++] = &cheat[c];
		cheat[c].onSelect() =
			[this, c](TextMenuItem &, View &, const Input::Event &e)
			{
				auto &editCheatView = *new SystemEditCheatView{window()};
				editCheatView.init(!e.isPointer(), c);
				pushAndShow(editCheatView);
			};
	}
}

EditCheatListView::EditCheatListView(Base::Window &win):
	BaseEditCheatListView(win),
	addCode
	{
		"Add Game Genie/Action Replay/Gold Finger Code",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(Cheat.num_cheats == EmuCheats::MAX)
			{
				popup.postError("Too many cheats, delete some first");
				return;
			}
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input xxxx-xxxx (GG), xxxxxxxx (AR), or GF code", getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint8 byte;
						uint32 address;
						uint8 bytes[3];
						bool8 sram;
						uint8 numBytes;
						if(!S9xGameGenieToRaw(str, address, byte))
							S9xAddCheat(false, true, address, byte);
						else if(!S9xProActionReplayToRaw (str, address, byte))
							S9xAddCheat(false, true, address, byte);
						else if(!S9xGoldFingerToRaw(str, address, sram, numBytes, bytes))
						{
							iterateTimes(numBytes, i)
								S9xAddCheat(false, true, address + i, bytes[i]);
							// TODO: handle cheat names for multiple codes added at once
						}
						else
						{
							popup.postError("Invalid format");
							return 1;
						}
						string_copy(Cheat.c[Cheat.num_cheats - 1].name, "Unnamed Cheat");
						logMsg("added new cheat, %d total", Cheat.num_cheats);
						view.dismiss();
						auto &textInputView = *new CollectTextInputView{window()};
						textInputView.init("Input description", getCollectTextCloseAsset());
						textInputView.onText() =
							[](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									string_copy(Cheat.c[Cheat.num_cheats - 1].name, str);
									view.dismiss();
									refreshCheatViews();
								}
								else
								{
									view.dismiss();
								}
								return 0;
							};
						refreshCheatViews();
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
	}
{}

void CheatsView::loadCheatItems(MenuItem *item[], uint &i)
{
	int cheats = std::min(Cheat.num_cheats, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		cheat[c].init(Cheat.c[c].name, Cheat.c[c].enabled); item[i++] = &cheat[c];
		cheat[c].onSelect() =
			[this, c](BoolMenuItem &item, View &, const Input::Event &e)
			{
				item.toggle(*this);
				if(item.on)
					S9xEnableCheat(c);
				else
					S9xDisableCheat(c);
			};
	}
}
