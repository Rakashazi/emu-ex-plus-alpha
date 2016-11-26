// Customized minimal FrameBuffer class needed for 2600.emu

#pragma once

#include <stella/gui/Rect.hxx>
#include <imagine/pixmap/Pixmap.hh>

class TIA;

enum FBInitStatus {
	kSuccess,
	kFailComplete,
	kFailTooLarge,
	kFailNotSupported,
};

class FrameBuffer
{
public:
	bool myUsePhosphor = false;
	int myPhosphorBlend = 77;
	uInt16 tiaColorMap[256]{}, tiaPhosphorColorMap[256][256]{};

	FrameBuffer() {}

	void render(IG::Pixmap pix, TIA &tia);

	FrameBuffer &tiaSurface() { return *this; }

	// dummy value, not actually needed
	GUI::Size desktopSize() const { return GUI::Size{1024, 1024}; }

	// no-op, EmuFramework manages window
	FBInitStatus createDisplay(const string& title, uInt32 width, uInt32 height)
	{
		return kSuccess;
	}

	// no-op
	void showFrameStats(bool enable) {}

	/**
		Set up the TIA/emulation palette for a screen of any depth > 8.

		@param palette  The array of colors
	*/
	void setPalette(const uInt32* palette);

	/**
		Shows a message onscreen.

		@param message  The message to be shown
		@param position Onscreen position for the message
		@param force    Force showing this message, even if messages are disabled
		@param color    Color of text in the message
	*/
	void showMessage(const string& message,
										int position = 0,
										bool force = false,
										uInt32 color = 0);

	/**
		Enable/disable phosphor effect.
	*/
	void enablePhosphor(bool enable, int blend);

	/**
		Used to calculate an averaged color for the 'phosphor' effect.

		@param c1  Color 1
		@param c2  Color 2

		@return  Averaged value of the two colors
	*/
	uInt8 getPhosphor(uInt8 c1, uInt8 c2) const;
};
