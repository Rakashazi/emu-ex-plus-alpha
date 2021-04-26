/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/TextEntry.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <fceu/driver.h>
extern unsigned fceuCheats;
void EncodeGG(char *str, int a, int v, int c);
static const int UNCHANGED_VAL = -2;

static bool isValidGGCodeLen(const char *str)
{
	return strlen(str) == 6 || strlen(str) == 8;
}

static const char *cheatName(unsigned idx)
{
	char *name{};
	int gotCheat = FCEUI_GetCheat(idx, &name, 0, 0, 0, 0, 0);
	assert(gotCheat);
	return name;
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		{},
		attach,
		{},
		[this](const TableView &)
		{
			return type ? 3 : 5;
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
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
			onCheatListChanged();
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	addr
	{
		"Address",
		nullptr,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 4-digit hex", addrStr,
				[this](EmuApp &app, auto str)
				{
					unsigned a = strtoul(str, nullptr, 16);
					if(a > 0xFFFF)
					{
						logMsg("addr 0x%X too large", a);
						app.postMessage(true, "Invalid input");
						postDraw();
						return false;
					}
					string_copy(addrStr, a ? str : "0");
					syncCheat();
					addr.set2ndName(addrStr);
					addr.compile(renderer(), projP);
					postDraw();
					return true;
				});
		}
	},
	value
	{
		"Value",
		nullptr,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 2-digit hex", valueStr,
				[this](EmuApp &app, auto str)
				{
					unsigned a = strtoul(str, nullptr, 16);
					if(a > 0xFF)
					{
						logMsg("val 0x%X too large", a);
						app.postMessage(true, "Invalid input");
						postDraw();
						return false;
					}
					string_copy(valueStr, a ? str : "0");
					syncCheat();
					value.set2ndName(valueStr);
					value.compile(renderer(), projP);
					postDraw();
					return true;
				});
		}
	},
	comp
	{
		"Compare",
		nullptr,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, "Input 2-digit hex or blank", compStr,
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(strlen(str))
						{
							unsigned a = strtoul(str, nullptr, 16);
							if(a > 0xFF)
							{
								logMsg("val 0x%X too large", a);
								app().postMessage(true, "Invalid input");
								return 1;
							}
							string_copy(compStr, str);
							comp.set2ndName(str);
						}
						else
						{
							compStr[0] = 0;
							comp.set2ndName(nullptr);
						}
						syncCheat();
						comp.compile(renderer(), projP);
						postDraw();
					}
					view.dismiss();
					return 0;
				});
		}
	},
	ggCode
	{
		"GG Code",
		nullptr,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input Game Genie code", ggCodeStr,
				[this](EmuApp &app, auto str)
				{
					if(!isValidGGCodeLen(str))
					{
						app.postMessage(true, "Invalid, must be 6 or 8 digits");
						return false;
					}
					string_copy(ggCodeStr, str);
					syncCheat();
					ggCode.set2ndName(str);
					ggCode.compile(renderer(), projP);
					postDraw();
					return true;
				});
		}
	},
	idx{cheatIdx}
{
	uint32 a;
	uint8 v;
	int compare;
	char *nameStr{};
	int gotCheat = FCEUI_GetCheat(cheatIdx, &nameStr, &a, &v, &compare, 0, &type);
	logMsg("got cheat with addr 0x%.4x val 0x%.2x comp %d", a, v, compare);
	name.setName(nameStr);
	if(type)
	{
		setName("Edit Code");
		if(a == 0 && v == 0 && compare == -1)
			ggCodeStr[0] = 0;
		else
			EncodeGG(ggCodeStr, a, v, compare);
		ggCode.set2ndName(ggCodeStr);
	}
	else
	{
		setName("Edit RAM Patch");
		snprintf(addrStr, sizeof(addrStr), "%x", a);
		addr.set2ndName(addrStr);
		snprintf(valueStr, sizeof(valueStr), "%x", v);
		value.set2ndName(valueStr);
		if(compare == -1)
			compStr[0] = 0;
		else
		{
			snprintf(compStr, sizeof(compStr), "%x", compare);
			comp.set2ndName(compStr);
		}
	}
}

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

const char *EmuEditCheatView::cheatNameString() const
{
	return cheatName(idx);
}

void EmuEditCheatView::renamed(const char *str)
{
	syncCheat(str);
}

void EmuEditCheatListView::loadCheatItems()
{
	unsigned cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		const char *name = cheatName(c);
		cheat.emplace_back(name ? name : "Corrupt Cheat", &defaultFace(),
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(c, [this](){ onCheatListChanged(); }), e);
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
		[this](const TableView &, unsigned idx) -> MenuItem&
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
		"Add Game Genie Code", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, "Input Game Genie code", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!isValidGGCodeLen(str))
						{
							app().postMessage(true, "Invalid, must be 6 or 8 digits");
							return 1;
						}
						{
							int a, v, c;
							if(!FCEUI_DecodeGG(str, &a, &v, &c))
							{
								app().postMessage(true, "Invalid code");
								return 1;
							}
							if(!FCEUI_AddCheat("Unnamed Cheat", a, v, c, 1))
							{
								app().postMessage(true, "Error adding cheat");
								view.dismiss();
								return 0;
							}
						}
						fceuCheats++;
						FCEUI_ToggleCheat(fceuCheats-1);
						logMsg("added new cheat, %d total", fceuCheats);
						view.dismiss();
						app().pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[this](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									FCEUI_SetCheat(fceuCheats-1, str, UNCHANGED_VAL, UNCHANGED_VAL, UNCHANGED_VAL, -1, 1);
									onCheatListChanged();
									view.dismiss();
								}
								else
								{
									view.dismiss();
								}
								return 0;
							});
					}
					else
					{
						view.dismiss();
					}
					return 0;
				});
		}
	},
	addRAM
	{
		"Add RAM Patch", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, "Input description", "",
				[this](CollectTextInputView &view, const char *str)
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
						onCheatListChanged();
						auto editCheatView = makeView<EmuEditCheatView>(fceuCheats-1, [this](){ onCheatListChanged(); });
						view.dismiss();
						pushAndShow(std::move(editCheatView), {});
					}
					else
					{
						view.dismiss();
					}
					return 0;
				});
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
	unsigned cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		char *name;
		int status = 0;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, &status, 0);
		assert(gotCheat);
		cheat.emplace_back(gotCheat ? name : "Corrupt Cheat", &defaultFace(), status,
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
					app().postMessage(true, "Game Genie code isn't set");
					return;
				}
				item.flipBoolValue(*this);
				FCEUI_ToggleCheat(c);
			});
	}
}
