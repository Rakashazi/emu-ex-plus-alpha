#include <imagine/gui/TextEntry.hh>
#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <fceu/driver.h>
extern uint fceuCheats;
void EncodeGG(char *str, int a, int v, int c);
static const int UNCHANGED_VAL = -2;

void EmuEditCheatView::syncCheat(const char *newName)
{
	if(type)
	{
		int a, v, c;
		if(!FCEUI_DecodeGG(ggCodeStr, &a, &v, &c))
		{
			logWarn("error decoding GG code %s", ggCodeStr);
			a = 0; v = 0; c = -1;
		}
		if(!FCEUI_SetCheat(idx, newName, a, v, c, -1, 1))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
	else
	{
		logMsg("setting comp %d", strlen(compStr) ? (int)strtoul(compStr, nullptr, 16) : -1);
		if(!FCEUI_SetCheat(idx,
				newName, strtoul(addrStr, nullptr, 16), strtoul(valueStr, nullptr, 16),
				strlen(compStr) ? strtoul(compStr, nullptr, 16) : -1, -1, 0))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
}

void EmuEditCheatView::renamed(const char *str)
{
	syncCheat(str);
	FCEUI_GetCheat(idx, &nameStr, nullptr, nullptr, nullptr, nullptr, nullptr);
	name.t.setString(nameStr);
	name.compile(renderer(), projP);
	refreshCheatViews();
}

static bool isValidGGCodeLen(const char *str)
{
	return strlen(str) == 6 || strlen(str) == 8;
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, uint cheatIdx):
	BaseEditCheatView
	{
		"",
		attach,
		"",
		[this](const TableView &)
		{
			return type ? 3 : 5;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			if(type)
			{
				switch(idx)
				{
					case 0: return name;
					case 1: return ggCode;
					default: return remove;
				}
			}
			else
			{
				switch(idx)
				{
					case 0: return name;
					case 1: return addr;
					case 2: return value;
					case 3: return comp;
					default: return remove;
				}
			}
		},
		[this](TextMenuItem &, View &, Input::Event)
		{
			assert(fceuCheats != 0);
			FCEUI_DelCheat(idx);
			fceuCheats--;
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
			textInputView.init("Input 4-digit hex", addrStr, getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = strtoul(str, nullptr, 16);
						if(a > 0xFFFF)
						{
							logMsg("addr 0x%X too large", a);
							popup.postError("Invalid input");
							window().postDraw();
							return 1;
						}
						string_copy(addrStr, a ? str : "0");
						syncCheat();
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
							logMsg("val 0x%X too large", a);
							popup.postError("Invalid input");
							window().postDraw();
							return 1;
						}
						string_copy(valueStr, a ? str : "0");
						syncCheat();
						value.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	comp
	{
		"Compare",
		compStr,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input 2-digit hex or blank", compStr, getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(strlen(str))
						{
							uint a = strtoul(str, nullptr, 16);
							if(a > 0xFF)
							{
								logMsg("val 0x%X too large", a);
								popup.postError("Invalid input");
								window().postDraw();
								return 1;
							}
							string_copy(compStr, str);
						}
						else
						{
							compStr[0] = 0;
						}
						syncCheat();
						comp.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	ggCode
	{
		"GG Code",
		ggCodeStr,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input Game Genie code", ggCodeStr, getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!isValidGGCodeLen(str))
						{
							popup.postError("Invalid, must be 6 or 8 digits");
							window().postDraw();
							return 1;
						}
						string_copy(ggCodeStr, str);
						syncCheat();
						ggCode.compile(renderer(), projP);
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
	uint32 a;
	uint8 v;
	int compare;
	int gotCheat = FCEUI_GetCheat(cheatIdx, &nameStr, &a, &v, &compare, 0, &type);
	logMsg("got cheat with addr 0x%.4x val 0x%.2x comp %d", a, v, compare);
	name.t.setString(nameStr);
	if(type)
	{
		name_ = "Edit Code";
		if(a == 0 && v == 0 && compare == -1)
			ggCodeStr[0] = 0;
		else
			EncodeGG(ggCodeStr, a, v, compare);
	}
	else
	{
		name_ = "Edit RAM Patch";
		snprintf(addrStr, sizeof(addrStr), "%x", a);
		snprintf(valueStr, sizeof(valueStr), "%x", v);
		if(compare == -1)
			compStr[0] = 0;
		else
			snprintf(compStr, sizeof(compStr), "%x", compare);
	}
}

void EmuEditCheatListView::loadCheatItems()
{
	uint cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		char *name;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, 0, 0);
		assert(gotCheat);
		cheat.emplace_back(gotCheat ? name : "Corrupt Cheat",
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
			return 2 + cheat.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addGG;
				case 1: return addRAM;
				default: return cheat[idx - 2];
			}
		}
	},
	addGG
	{
		"Add Game Genie Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input Game Genie code", getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!isValidGGCodeLen(str))
						{
							popup.postError("Invalid, must be 6 or 8 digits");
							return 1;
						}
						{
							int a, v, c;
							if(!FCEUI_DecodeGG(str, &a, &v, &c))
							{
								popup.postError("Invalid code");
								return 1;
							}
							if(!FCEUI_AddCheat("Unnamed Cheat", a, v, c, 1))
							{
								popup.postError("Error adding cheat");
								view.dismiss();
								return 0;
							}
						}
						fceuCheats++;
						FCEUI_ToggleCheat(fceuCheats-1);
						logMsg("added new cheat, %d total", fceuCheats);
						view.dismiss();
						auto &textInputView = *new CollectTextInputView{attachParams()};
						textInputView.init("Input description", getCollectTextCloseAsset(renderer()));
						textInputView.onText() =
							[](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									FCEUI_SetCheat(fceuCheats-1, str, UNCHANGED_VAL, UNCHANGED_VAL, UNCHANGED_VAL, -1, 1);
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
	},
	addRAM
	{
		"Add RAM Patch",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{attachParams()};
			textInputView.init("Input description", getCollectTextCloseAsset(renderer()));
			textInputView.onText() =
			[](CollectTextInputView &view, const char *str)
			{
				if(str)
				{
					if(!FCEUI_AddCheat(str, 0, 0, -1, 0))
					{
						logErr("error adding new cheat");
						view.dismiss();
						return 0;
					}
					fceuCheats++;
					FCEUI_ToggleCheat(fceuCheats-1);
					logMsg("added new cheat, %d total", fceuCheats);
					auto &editCheatView = *new EmuEditCheatView{view.attachParams(), fceuCheats-1};
					view.dismiss();
					refreshCheatViews();
					viewStack.pushAndShow(editCheatView, {});
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
	uint cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		char *name;
		int status = 0;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, &status, 0);
		assert(gotCheat);
		cheat.emplace_back(gotCheat ? name : "Corrupt Cheat", status,
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				uint32 a;
				uint8 v;
				int compare, type;
				int gotCheat = FCEUI_GetCheat(c, nullptr, &a, &v, &compare, 0, &type);
				if(!gotCheat)
					return;
				if(!item.boolValue() && type && a == 0 && v == 0 && compare == -1)
				{
					// Don't turn on null Game Genie codes
					popup.postError("Game Genie code isn't set", 2);
					return;
				}
				item.flipBoolValue(*this);
				FCEUI_ToggleCheat(c);
			});
	}
}
