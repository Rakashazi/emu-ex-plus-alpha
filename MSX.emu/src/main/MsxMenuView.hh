#pragma once

#include "MenuView.hh"

class MsxMediaChangeListener
{
public:
	constexpr MsxMediaChangeListener() { }
	virtual void onMediaChange(const char *name) = 0;
	virtual void onActionFromModalView(uint action, const InputEvent &e) = 0;
};

class MsxMediaFilePicker// : public BaseFilePicker
{
public:
	enum { ROM, DISK, TAPE };
	uint type, slot;
	MsxMediaChangeListener *listener;

	void onSelectFile(const char* name, const InputEvent &e)
	{
		switch(type)
		{
			bcase ROM:
				if(insertROM(name, slot))
				{
					listener->onMediaChange(name);
				}
			bcase DISK:
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					listener->onMediaChange(name);
				}
			/*bcase TAPE:
				/*if(insertTape(name, slot))
				{
					strcpy(tapeName, name);
					listener->onMediaChange();
				}*/
		}

		removeModalView();
	}

	void onClose(const InputEvent &e)
	{
		removeModalView();
	}

	/*void inputEvent(const InputEvent &e)
	{
		if(e.state == INPUT_PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				onClose();
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					removeModalView();
					startGameFromMenu();
					return;
				}
			}
		}

		FSPicker::inputEvent(e);
	}*/

	void init(uint type, uint slot, MsxMediaChangeListener *listener, bool highlightFirst)
	{
		this->type = type;
		this->slot = slot;
		this->listener = listener;
		FsDirFilterFunc filter = isMSXROMExtension;
		if(type == DISK)
			filter = isMSXDiskExtension;
		else if(type == TAPE)
			filter = isMSXTapeExtension;
		//BaseFilePicker::init(highlightFirst, filter, 1);
		fPicker.init(highlightFirst, filter, 1);
		fPicker.onSelectFileDelegate().bind<MsxMediaFilePicker, &MsxMediaFilePicker::onSelectFile>(this);
		fPicker.onCloseDelegate().bind<MsxMediaFilePicker, &MsxMediaFilePicker::onClose>(this);
	}
};

class InsertEjectDiskMenu : public BaseMultiChoiceView
{
public:
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2];
	MsxMediaChangeListener *listener;

	void init(MsxMediaChangeListener *listener, bool highlightFirst)
	{
		var_selfSet(listener);
		choiceEntry[0].init("Insert File"); choiceEntryItem[0] = &choiceEntry[0];
		choiceEntry[1].init("Eject"); choiceEntryItem[1] = &choiceEntry[1];
		BaseMenuView::init(choiceEntryItem, sizeofArray(choiceEntry), highlightFirst, C2DO);
	}

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
	{
		listener->onActionFromModalView(i, e);
	}
};

static InsertEjectDiskMenu insertEjectDiskMenu;

class InsertEjectRomMenu : public BaseMultiChoiceView
{
public:
	TextMenuItem choiceEntry[5];
	MenuItem *choiceEntryItem[5];
	MsxMediaChangeListener *listener;

	void init(MsxMediaChangeListener *listener, bool highlightFirst)
	{
		var_selfSet(listener);
		choiceEntry[0].init("Insert File"); choiceEntryItem[0] = &choiceEntry[0];
		choiceEntry[1].init("Eject"); choiceEntryItem[1] = &choiceEntry[1];
		choiceEntry[2].init("Insert SCC"); choiceEntryItem[2] = &choiceEntry[2];
		choiceEntry[3].init("Insert SCC+"); choiceEntryItem[3] = &choiceEntry[3];
		choiceEntry[4].init("Insert Sunrise IDE"); choiceEntryItem[4] = &choiceEntry[4];
		BaseMenuView::init(choiceEntryItem, sizeofArray(choiceEntry), highlightFirst, C2DO);
	}

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
	{
		listener->onActionFromModalView(i, e);
	}
};

static InsertEjectRomMenu insertEjectRomMenu;

class MsxIOControlView : public BaseMenuView
{
private:
	static MsxMediaFilePicker picker;
	#if CONFIG_USE_IN_TABLE_NAV
	MenuView::BackMenuItem back;
	#endif

	struct HDMenuItem : public TextMenuItem, public MsxMediaChangeListener
	{
		constexpr HDMenuItem() { }
		int slot = 0;
		const char *prefix = nullptr;
		char strBuff[1024] = {0};
		void init(const char *str, int slot)
		{
			prefix = str;
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, hdName[slot]);
			TextMenuItem::init(strBuff, boardGetHdType(slot/2) == HD_SUNRISEIDE);
			this->slot = slot;
		}

		void refreshLabel()
		{
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, hdName[slot]);
			TextMenuItem::compile();
		}

		void onCartUpdate()
		{
			active = boardGetHdType(slot/2) == HD_SUNRISEIDE;
			refreshLabel();
		}

		void onMediaChange(const char *name)
		{
			strcpy(hdName[slot], name);
			refreshLabel();
		}

		void onActionFromModalView(uint action, const InputEvent &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::DISK, diskGetHdDriveId(slot / 2, slot % 2), this, !e.isPointer());
				fPicker.place(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
			else
			{
				diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
				onMediaChange("");
				removeModalView();
			}
		}

		void select(View *view, const InputEvent &e)
		{
			if(!active) return;
			if(strlen(hdName[slot]))
			{
				insertEjectDiskMenu.init(this, !e.isPointer());
				insertEjectDiskMenu.place(Gfx::viewportRect());
				modalView = &insertEjectDiskMenu;
			}
			else
			{
				picker.init(MsxMediaFilePicker::DISK, diskGetHdDriveId(slot / 2, slot % 2), this, !e.isPointer());
				fPicker.place(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
		}
	} hdSlot[4];

	struct RomMenuItem : public TextMenuItem, public MsxMediaChangeListener
	{
		constexpr RomMenuItem() { }
		int slot = 0;
		const char *prefix = nullptr;
		char strBuff[1024] = {0};
		HDMenuItem *hdMenuItem = nullptr;
		void init(const char *str, int slot, HDMenuItem *hdMenuItem)
		{
			prefix = str;
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, cartName[slot]);
			TextMenuItem::init(strBuff, slot < boardInfo.cartridgeCount);
			var_selfs(slot);
			var_selfs(hdMenuItem);
		}

		void onMediaChange(const char *name)
		{
			strcpy(cartName[slot], name);
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, cartName[slot]);
			TextMenuItem::compile();
			iterateTimes(2, i)
			{
				hdMenuItem[i].onCartUpdate();
			}
		}

		void onActionFromModalView(uint action, const InputEvent &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::ROM, slot, this, !e.isPointer());
				fPicker.place(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
			else if(action == 1)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onMediaChange("");
				removeModalView();
			}
			else if(action == 2)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onMediaChange("SCC");
				removeModalView();
			}
			else if(action == 3)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onMediaChange("SCC+");
				removeModalView();
			}
			else if(action == 4)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					popup.postError("Error loading Sunrise IDE device");
				}
				else
					onMediaChange("Sunrise IDE");
				removeModalView();
			}
		}

		void select(View *view, const InputEvent &e)
		{
			insertEjectRomMenu.init(this, !e.isPointer());
			insertEjectRomMenu.place(Gfx::viewportRect());
			modalView = &insertEjectRomMenu;
		}
	} romSlot[2];

	struct DiskMenuItem : public TextMenuItem, public MsxMediaChangeListener
	{
		constexpr DiskMenuItem() { }
		int slot = 0;
		const char *prefix = nullptr;
		char strBuff[1024] = {0};
		void init(const char *str, int slot)
		{
			prefix = str;
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, diskName[slot]);
			TextMenuItem::init(strBuff, slot < boardInfo.diskdriveCount);
			this->slot = slot;
		}

		void onMediaChange(const char *name)
		{
			strcpy(diskName[slot], name);
			snprintf(strBuff, sizeof(strBuff), "%s %s", prefix, diskName[slot]);
			TextMenuItem::compile();
		}

		void onActionFromModalView(uint action, const InputEvent &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::DISK, slot, this, !e.isPointer());
				fPicker.place(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
			else
			{
				diskChange(slot, 0, 0);
				onMediaChange("");
				removeModalView();
			}
		}

		void select(View *view, const InputEvent &e)
		{
			if(strlen(diskName[slot]))
			{
				insertEjectDiskMenu.init(this, !e.isPointer());
				insertEjectDiskMenu.place(Gfx::viewportRect());
				modalView = &insertEjectDiskMenu;
			}
			else
			{
				picker.init(MsxMediaFilePicker::DISK, slot, this, !e.isPointer());
				fPicker.place(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
		}
	} diskSlot[2];

	MenuItem *item[9] = {nullptr};
public:
	constexpr MsxIOControlView(): BaseMenuView("IO Control") { }

	void init(bool highlightFirst)
	{
		uint i = 0;
		#if CONFIG_USE_IN_TABLE_NAV
		back.init(); item[i++] = &back;
		#endif
		romSlot[0].init("ROM1:", 0, &hdSlot[0]); item[i++] = &romSlot[0];
		romSlot[1].init("ROM2:", 1, &hdSlot[2]); item[i++] = &romSlot[1];
		diskSlot[0].init("Disk1:", 0); item[i++] = &diskSlot[0];
		diskSlot[1].init("Disk2:", 1); item[i++] = &diskSlot[1];
		hdSlot[0].init("IDE1-M:", 0); item[i++] = &hdSlot[0];
		hdSlot[1].init("IDE1-S:", 1); item[i++] = &hdSlot[1];
		hdSlot[2].init("IDE2-M:", 2); item[i++] = &hdSlot[2];
		hdSlot[3].init("IDE2-S:", 3); item[i++] = &hdSlot[3];
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static MsxIOControlView msxIoMenu;

MsxMediaFilePicker MsxIOControlView::picker;

class MsxMenuView : public MenuView
{
private:
	struct MsxIOControlMenuItem : public TextMenuItem
	{
		void init()
		{
			TextMenuItem::init("ROM/Disk Control");
		}

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning() && activeBoardType == BOARD_MSX;
		}

		void select(View *view, const InputEvent &e)
		{
			if(active)
			{
				FsSys::chdir(EmuSystem::gamePath);// Stay in active media's directory
				msxIoMenu.init(!e.isPointer());
				viewStack.pushAndShow(&msxIoMenu);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				popup.post("Only used in MSX mode", 2);
			}
		}
	} msxIOControl;

	MenuItem *item[STANDARD_ITEMS + 1];

public:

	void gameStopped()
	{
		#if CONFIG_USE_IN_TABLE_NAV
		resumeGame.active = 0;
		#endif
		msxIOControl.active = 0;
		reset.active = 0;
		loadState.active = 0;
		saveState.active = 0;
	}

	void onShow()
	{
		MenuView::onShow();
		msxIOControl.refreshActive();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		msxIOControl.init(); item[items++] = &msxIOControl;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
