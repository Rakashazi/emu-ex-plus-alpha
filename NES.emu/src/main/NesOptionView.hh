#pragma once
#include "OptionView.hh"

static bool isFDSBIOSExtension(const char *name);
static void setupNESInputPorts();
static void setupNESFourScore();

class DiskSystemBIOSFilePicker
{
public:
	static void onSelectFile(const char* name, const InputEvent &e)
	{
		snprintf(fdsBiosPath, sizeof(fdsBiosPath), "%s/%s", FsSys::workDir(), name);
		logMsg("set fds bios %s", fdsBiosPath);
		View::removeModalView();
	}

	static void onClose(const InputEvent &e)
	{
		View::removeModalView();
	}

	static void init(bool highlightFirst)
	{
		fPicker.init(highlightFirst, biosFsFilter);
		fPicker.onSelectFileDelegate().bind<&DiskSystemBIOSFilePicker::onSelectFile>();
		fPicker.onCloseDelegate().bind<&DiskSystemBIOSFilePicker::onClose>();
	}
};

class NesOptionView : public OptionView
{
private:
	TextMenuItem fdsBiosPath;

	static void fdsBiosPathHandler(TextMenuItem &, const InputEvent &e)
	{
		DiskSystemBIOSFilePicker::init(!e.isPointer());
		fPicker.place(Gfx::viewportRect());
		modalView = &fPicker;
		Base::displayNeedsUpdate();
	}

	BoolMenuItem fourScore;

	static void fourScoreHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionFourScore = item.on;
		setupNESFourScore();
	}

	MultiChoiceSelectMenuItem inputPorts;

	void inputPortsInit()
	{
		static const char *str[] =
		{
			"Auto", "Gamepads", "Gun (2P, NES)", "Gun (1P, VS)"
		};
		int setting = 0;
		if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_GAMEPAD)
			setting = 1;
		else if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_ZAPPER)
			setting = 2;
		else if(nesInputPortDev[0] == SI_ZAPPER && nesInputPortDev[1] == SI_GAMEPAD)
			setting = 3;

		inputPorts.init("Input Ports", str, setting, sizeofArray(str));
		inputPorts.valueDelegate().bind<&inputPortsSet>();
	}

	static void inputPortsSet(MultiChoiceMenuItem &, int val)
	{
		if(val == 0)
		{
			nesInputPortDev[0] = nesInputPortDev[1] = SI_UNSET;
		}
		else if(val == 1)
		{
			nesInputPortDev[0] = nesInputPortDev[1] = SI_GAMEPAD;
		}
		else if(val == 2)
		{
			nesInputPortDev[0] = SI_GAMEPAD; nesInputPortDev[1] = SI_ZAPPER;
		}
		else if(val == 3)
		{
			nesInputPortDev[0] = SI_ZAPPER; nesInputPortDev[1] = SI_GAMEPAD;
		}

		setupNESInputPorts();
	}

	MenuItem *item[24];

public:

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		fourScore.init("4-Player Adapter", optionFourScore); item[items++] = &fourScore;
		fourScore.selectDelegate().bind<&fourScoreHandler>();
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		fdsBiosPath.init("Select Disk System BIOS"); item[items++] = &fdsBiosPath;
		fdsBiosPath.selectDelegate().bind<&fdsBiosPathHandler>();
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
