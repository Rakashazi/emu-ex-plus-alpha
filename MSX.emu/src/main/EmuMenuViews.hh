#pragma once
#include "OptionView.hh"

struct InstallMSXSystem
{
	static const char *installMessage()
	{
		return
		#if defined(CONFIG_BASE_ANDROID)
			"Install the C-BIOS BlueMSX machine files to your storage device?"
		#elif defined(CONFIG_ENV_WEBOS)
			"Install the C-BIOS BlueMSX machine files to internal storage? If using WebOS 1.4.5, make sure you have a version without the write permission bug."
		#elif defined(CONFIG_BASE_IOS_JB)
			"Install the C-BIOS BlueMSX machine files to /User/Media/MSX.emu?"
		#else
			"Install the C-BIOS BlueMSX machine files to Machines directory?"
		#endif
			;
	}

	static void confirmAlert(const Input::Event &e)
	{
		CallResult ret = FsSys::mkdir(machineBasePath);
		if(ret != OK && ret != ALREADY_EXISTS)
		{
			popup.printf(4, 1, "Can't create directory:\n%s", machineBasePath);
			return;
		}

		const char *dirsToCreate[] =
		{
			"Machines", "Machines/MSX - C-BIOS",
			"Machines/MSX2 - C-BIOS", "Machines/MSX2+ - C-BIOS"
		};

		forEachDInArray(dirsToCreate, e)
		{
			FsSys::cPath pathTemp;
			snprintf(pathTemp, sizeof(pathTemp), "%s/%s", machineBasePath, e);
			CallResult ret = FsSys::mkdir(pathTemp);
			if(ret != OK && ret != ALREADY_EXISTS)
			{
				popup.printf(4, 1, "Can't create directory:\n%s", pathTemp);
				return;
			}
		}

		const char *srcPath[] =
		{
			"cbios.txt", "cbios.txt", "cbios.txt",
			"cbios_logo_msx1.rom", "cbios_main_msx1.rom", "config1.ini",
			"cbios_logo_msx2.rom", "cbios_main_msx2.rom", "cbios_sub.rom", "config2.ini",
			"cbios_logo_msx2+.rom", "cbios_main_msx2+.rom", "cbios_sub.rom", "cbios_music.rom", "config3.ini"
		};
		const char *destDir[] =
		{
				"MSX - C-BIOS", "MSX2 - C-BIOS", "MSX2+ - C-BIOS",
				"MSX - C-BIOS", "MSX - C-BIOS", "MSX - C-BIOS",
				"MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS",
				"MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS"
		};

		forEachDInArray(srcPath, e)
		{
			Io *src = openAppAssetIo(e);
			if(!src)
			{
				popup.printf(4, 1, "Can't open source file:\n %s", e);
				return;
			}
			FsSys::cPath pathTemp;
			snprintf(pathTemp, sizeof(pathTemp), "%s/Machines/%s/%s",
					machineBasePath, destDir[e_i], strstr(e, "config") ? "config.ini" : e);
			CallResult ret = copyIoToPath(src, pathTemp);
			delete src;
			if(ret != OK)
			{
				popup.printf(4, 1, "Can't write file:\n%s", e);
				return;
			}
		}

		string_copy(optionMachineNameStr, "MSX2 - C-BIOS", sizeof(optionMachineNameStr));
		popup.post("Installation OK");
	}
};

class MsxMachineChoiceView : public BaseMultiChoiceView
{
public:
	constexpr MsxMachineChoiceView() { }
	MultiChoiceMenuItem *srcEntry = nullptr;
	TextMenuItem choiceEntry[256];
	MenuItem *choiceEntryItem[256] {nullptr};

	void init(MultiChoiceMenuItem *src, bool highlightFirst)
	{
		assert((uint)src->choices <= sizeofArray(choiceEntry));
		iterateTimes(src->choices, i)
		{
			choiceEntry[i].init(src->choiceStr[i], src->t2.face);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, src->choices, highlightFirst, C2DO);
		srcEntry = src;
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		assert((int)i < srcEntry->choices);
		logMsg("set choice %d", i);
		srcEntry->setVal(i);
		removeModalView();
	}
};

static MsxMachineChoiceView msxMachineChoiceView;

class SystemOptionView : public OptionView
{
private:

	struct MsxMachineItem : public MultiChoiceSelectMenuItem
	{
		constexpr MsxMachineItem() { }

		static int dirFsFilter(const char *name, int type)
		{
			return type == Fs::TYPE_DIR;
		}

		uint machines = 0;
		char *machineName[256] {nullptr};

		void init()
		{
			FsSys f;
			static const char *title = "Machine Type";
			static const char *noneStr[] = { "None" };
			FsSys::cPath machinePath;
			snprintf(machinePath, sizeof(machinePath), "%s/Machines", machineBasePath);
			if(f.openDir(machinePath, 0, dirFsFilter) != OK)
			{
				logMsg("couldn't open %s", machinePath);
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			int currentMachineIdx = -1;
			machines = 0;
			iterateTimes(IG::min(f.numEntries(), 256U), i)
			{
				FsSys::cPath configPath;
				snprintf(configPath, sizeof(configPath), "%s/%s/config.ini", machinePath, f.entryFilename(i));
				if(!FsSys::fileExists(configPath))
				{
					logMsg("%s doesn't exist", configPath);
					continue;
				}
				machineName[machines] = string_dup(f.entryFilename(i));
				logMsg("added machine %s", f.entryFilename(i));
				if(string_equal(machineName[machines], optionMachineName))
				{
					logMsg("current machine is idx %d", i);
					currentMachineIdx = machines;
				}
				machines++;
			}

			if(!machines)
			{
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			MultiChoiceSelectMenuItem::init(title, (const char **)machineName,
					currentMachineIdx, machines, 0, 1, currentMachineIdx == -1 ? "None" : 0);
		}

		void select(View *view, const Input::Event &e)
		{
			if(!machines)
			{
				popup.printf(4, 1, "Place machine directory in:\n%s", machineBasePath);
				return;
			}
			msxMachineChoiceView.init(this, !e.isPointer());
			msxMachineChoiceView.placeRect(Gfx::viewportRect());
			modalView = &msxMachineChoiceView;
		}

		void deinit()
		{
			logMsg("deinit MsxMachineItem");
			DualTextMenuItem::deinit();
			iterateTimes(machines, i)
			{
				mem_free(machineName[i]);
			}
		}

		void doSet(int val)
		{
			assert((uint)val < machines);
			string_copy(optionMachineName, machineName[val], sizeof(optionMachineName));
			logMsg("set machine type: %s", (char*)optionMachineName);
		}
	} msxMachine;

	#if !defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_IOS_JB)
	struct InstallCBIOSMenuItem : public TextMenuItem
	{
		constexpr InstallCBIOSMenuItem() { }
		void init() { TextMenuItem::init("Install MSX C-BIOS"); }

		void select(View *view, const Input::Event &e)
		{
			ynAlertView.init(InstallMSXSystem::installMessage(), !e.isPointer());
			ynAlertView.onYes().bind<&InstallMSXSystem::confirmAlert>();
			ynAlertView.placeRect(Gfx::viewportRect());
			modalView = &ynAlertView;
			Base::displayNeedsUpdate();
		}
	} installCBIOS;
	#endif

	static void skipFdcAccessHandler(BoolMenuItem &item, const Input::Event &e)
	{
		item.toggle();
		optionSkipFdcAccess = item.on;
	}

	BoolMenuItem skipFdcAccess {"Fast-forward disk IO"};

public:
	constexpr SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		msxMachine.init(); item[items++] = &msxMachine;
		skipFdcAccess.init(optionSkipFdcAccess); item[items++] = &skipFdcAccess;
		skipFdcAccess.selectDelegate().bind<&skipFdcAccessHandler>();
		#if !defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_IOS_JB)
		installCBIOS.init(); item[items++] = &installCBIOS;
		#endif
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

#include "MenuView.hh"

class MsxMediaChangeListener
{
public:
	constexpr MsxMediaChangeListener() { }
	virtual void onMediaChange(const char *name) = 0;
	virtual void onActionFromModalView(uint action, const Input::Event &e) = 0;
};

class MsxMediaFilePicker
{
public:
	constexpr MsxMediaFilePicker() { }
	enum { ROM, DISK, TAPE };
	uint type = ROM, slot = 0;
	MsxMediaChangeListener *listener = nullptr;

	void onSelectFile(const char* name, const Input::Event &e)
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

		View::removeModalView();
	}

	void onClose(const Input::Event &e)
	{
		View::removeModalView();
	}

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
	constexpr InsertEjectDiskMenu() { }
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2] {nullptr};
	MsxMediaChangeListener *listener = nullptr;

	void init(MsxMediaChangeListener *listener, bool highlightFirst)
	{
		var_selfSet(listener);
		choiceEntry[0].init("Insert File"); choiceEntryItem[0] = &choiceEntry[0];
		choiceEntry[1].init("Eject"); choiceEntryItem[1] = &choiceEntry[1];
		BaseMenuView::init(choiceEntryItem, sizeofArray(choiceEntry), highlightFirst, C2DO);
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		listener->onActionFromModalView(i, e);
	}
};

static InsertEjectDiskMenu insertEjectDiskMenu;

class InsertEjectRomMenu : public BaseMultiChoiceView
{
public:
	constexpr InsertEjectRomMenu() { }
	TextMenuItem choiceEntry[5];
	MenuItem *choiceEntryItem[5] {nullptr};
	MsxMediaChangeListener *listener = nullptr;

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

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
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

		void onActionFromModalView(uint action, const Input::Event &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::DISK, diskGetHdDriveId(slot / 2, slot % 2), this, !e.isPointer());
				fPicker.placeRect(Gfx::viewportRect());
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

		void select(View *view, const Input::Event &e)
		{
			if(!active) return;
			if(strlen(hdName[slot]))
			{
				insertEjectDiskMenu.init(this, !e.isPointer());
				insertEjectDiskMenu.placeRect(Gfx::viewportRect());
				modalView = &insertEjectDiskMenu;
			}
			else
			{
				picker.init(MsxMediaFilePicker::DISK, diskGetHdDriveId(slot / 2, slot % 2), this, !e.isPointer());
				fPicker.placeRect(Gfx::viewportRect());
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
		char strBuff[1024] {0};
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

		void onActionFromModalView(uint action, const Input::Event &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::ROM, slot, this, !e.isPointer());
				fPicker.placeRect(Gfx::viewportRect());
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

		void select(View *view, const Input::Event &e)
		{
			insertEjectRomMenu.init(this, !e.isPointer());
			insertEjectRomMenu.placeRect(Gfx::viewportRect());
			modalView = &insertEjectRomMenu;
		}
	} romSlot[2];

	struct DiskMenuItem : public TextMenuItem, public MsxMediaChangeListener
	{
		constexpr DiskMenuItem() { }
		int slot = 0;
		const char *prefix = nullptr;
		char strBuff[1024] {0};
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

		void onActionFromModalView(uint action, const Input::Event &e)
		{
			if(action == 0)
			{
				removeModalView();
				picker.init(MsxMediaFilePicker::DISK, slot, this, !e.isPointer());
				fPicker.placeRect(Gfx::viewportRect());
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

		void select(View *view, const Input::Event &e)
		{
			if(strlen(diskName[slot]))
			{
				insertEjectDiskMenu.init(this, !e.isPointer());
				insertEjectDiskMenu.placeRect(Gfx::viewportRect());
				modalView = &insertEjectDiskMenu;
			}
			else
			{
				picker.init(MsxMediaFilePicker::DISK, slot, this, !e.isPointer());
				fPicker.placeRect(Gfx::viewportRect());
				modalView = &fPicker;
				Base::displayNeedsUpdate();
			}
		}
	} diskSlot[2];

	MenuItem *item[9] {nullptr};
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

class SystemMenuView : public MenuView
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

		void select(View *view, const Input::Event &e)
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

public:
	constexpr SystemMenuView() { }

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
