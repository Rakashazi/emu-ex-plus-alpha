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

	static void confirmAlert(const InputEvent &e)
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
	MultiChoiceMenuItem *srcEntry;
	TextMenuItem choiceEntry[256];
	MenuItem *choiceEntryItem[256];

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

	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
	{
		assert((int)i < srcEntry->choices);
		logMsg("set choice %d", i);
		srcEntry->setVal(i);
		removeModalView();
	}
};

static MsxMachineChoiceView msxMachineChoiceView;

class MsxOptionView : public OptionView
{
private:

	struct MsxMachineItem : public MultiChoiceSelectMenuItem
	{
		static int dirFsFilter(const char *name, int type)
		{
			return type == Fs::TYPE_DIR;
		}

		uint machines;
		char *machineName[256];

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

		void select(View *view, const InputEvent &e)
		{
			if(!machines)
			{
				popup.printf(4, 1, "Place machine directory in:\n%s", machineBasePath);
				return;
			}
			msxMachineChoiceView.init(this, !e.isPointer());
			msxMachineChoiceView.place(Gfx::viewportRect());
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
		void init() { TextMenuItem::init("Install MSX C-BIOS"); }

		void select(View *view, const InputEvent &e)
		{
			ynAlertView.init(InstallMSXSystem::installMessage(), !e.isPointer());
			ynAlertView.onYesDelegate().bind<&InstallMSXSystem::confirmAlert>();
			ynAlertView.place(Gfx::viewportRect());
			modalView = &ynAlertView;
			Base::displayNeedsUpdate();
		}
	} installCBIOS;
	#endif

	MenuItem *item[24];

public:

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		msxMachine.init(); item[items++] = &msxMachine;
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
