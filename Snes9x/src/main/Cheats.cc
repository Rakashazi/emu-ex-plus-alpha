#include <imagine/gui/TextEntry.hh>
#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <cheats.h>
extern CollectTextInputView textInputView;

void EmuEditCheatView::renamed(const char *str)
{
	string_copy(Cheat.c[idx].name, str);
	name.t.setString(Cheat.c[idx].name);
	name.compile(renderer(), projP);
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, uint cheatIdx):
	BaseEditCheatView
	{
		"Edit Address/Values",
		attach,
		Cheat.c[idx].name,
		[this](const TableView &)
		{
			return 5;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return name;
				case 1: return addr;
				case 2: return value;
				case 3: return saved;
				default: return remove;
			}
		},
		[this](TextMenuItem &, View &, Input::Event)
		{
			S9xDeleteCheat(idx);
			refreshCheatViews();
			dismiss();
			return true;
		}
	},
	addr
	{
		"Address",
		addrStr,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input 6-digit hex", addrStr, getCollectTextCloseAsset(renderer()));
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
						string_copy(addrStr, a ? str : "0");
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
						addr.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	value
	{
		"Value",
		valueStr,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input 2-digit hex", valueStr, getCollectTextCloseAsset(renderer()));
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
						string_copy(valueStr, a ? str : "0");
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
						value.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	saved
	{
		"Saved Value",
		savedStr,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input 2-digit hex or blank", savedStr, getCollectTextCloseAsset(renderer()));
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
							string_copy(savedStr, str);
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
						saved.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	idx{cheatIdx}
{
	auto &cheat = Cheat.c[idx];
	logMsg("got cheat with addr 0x%.6x val 0x%.2x saved val 0x%.2x", cheat.address, cheat.byte, cheat.saved_byte);
	string_printf(addrStr, "%x", cheat.address);
	string_printf(valueStr, "%x", cheat.byte);
	if(!cheat.saved)
		savedStr[0] = 0;
	else
		string_printf(savedStr, "%x", cheat.saved_byte);
}

void EmuEditCheatListView::loadCheatItems()
{
	uint cheats = Cheat.num_cheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back(Cheat.c[c].name,
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				auto &editCheatView = *new EmuEditCheatView{attachParams(), c};
				viewStack.pushAndShow(editCheatView, e);
			});
	}
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addCode;
				default: return cheat[idx - 1];
			}
		}
	},
	addCode
	{
		"Add Game Genie/Action Replay/Gold Finger Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(Cheat.num_cheats == EmuCheats::MAX)
			{
				popup.postError("Too many cheats, delete some first");
				return;
			}
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input xxxx-xxxx (GG), xxxxxxxx (AR), or GF code", getCollectTextCloseAsset(renderer()));
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
						auto &textInputView = *new CollectTextInputView{attachParams()};
						textInputView.init("Input description", getCollectTextCloseAsset(renderer()));
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
						modalViewController.pushAndShow(textInputView, {});
					}
					else
					{
						view.dismiss();
					}
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	}
{
	loadCheatItems();
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void EmuCheatsView::loadCheatItems()
{
	uint cheats = Cheat.num_cheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back(Cheat.c[c].name, Cheat.c[c].enabled,
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				bool on = item.flipBoolValue(*this);
				if(on)
					S9xEnableCheat(c);
				else
					S9xDisableCheat(c);
			});
	}
}
