// Customized minimal FrameBuffer class needed for 2600.emu

#pragma once

#include <stella/gui/Rect.hxx>
#include <stella/emucore/tia/TIAConstants.hxx>
#include <stella/emucore/FrameBufferConstants.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>
#include <imagine/pixmap/Pixmap.hh>
#include <array>

class Console;
class OSystem;
class TIA;

class FrameBuffer
{
public:
	uInt16 tiaColorMap16[256]{};
	uInt32 tiaColorMap32[256]{};
	uInt8 myPhosphorPalette[256][256]{};
	std::array<uInt8, 160 * TIAConstants::frameBufferHeight> prevFramebuffer;
	float myPhosphorPercent = 0.80f;
	bool myUsePhosphor = false;

	FrameBuffer() {}

	void render(IG::Pixmap pix, TIA &tia);

	FrameBuffer &tiaSurface() { return *this; }

	// dummy value, not actually needed
	GUI::Size desktopSize() const { return GUI::Size{1024, 1024}; }

	// no-op, EmuFramework manages window
	FBInitStatus createDisplay(const string& title, uInt32 width, uInt32 height)
	{
		return FBInitStatus::Success;
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
		Enable/disable/query phosphor effect.
	*/
	void enablePhosphor(bool enable, int blend = -1);
	bool phosphorEnabled() const { return myUsePhosphor; }

	/**
		Used to calculate an averaged color for the 'phosphor' effect.

		@param c1  Color 1
		@param c2  Color 2

		@return  Averaged value of the two colors
	*/
	uInt8 getPhosphor(const uInt8 c1, uInt8 c2) const;

	uInt32 getRGBPhosphor(const uInt32 c, const uInt32 p) const;

	void clear() {}
};
