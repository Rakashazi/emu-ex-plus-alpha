#pragma once

// Customized minimal FrameBuffer class needed for 2600.emu

#include <stella/common/Rect.hxx>
#include <stella/emucore/tia/TIAConstants.hxx>
#include <stella/emucore/FrameBufferConstants.hxx>
#include <stella/emucore/EventHandlerConstants.hxx>
#include <stella/common/PaletteHandler.hxx>
#include <stella/common/VideoModeHandler.hxx>
#include <array>

class Console;
class OSystem;
class TIA;

namespace IG
{
template <class PixData>
class PixmapViewBase;
using MutablePixmapView = PixmapViewBase<char>;
enum class PixelFormatId : uint8_t;
}

namespace EmuEx
{
class EmuApp;
}

class FrameBuffer
{
public:
	struct VideoMode
	{
		enum class Stretch { Preserve, Fill, None };

		Common::Rect image;
		Common::Size screen;
		Stretch stretch{VideoMode::Stretch::None};
		string description;
		float zoom{1.F};
		Int32 fsIndex{-1};

		VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
							Stretch smode, float overscan = 1.F,
							const string& desc = "", float zoomLevel = 1, Int32 fsindex = -1);

		friend ostream& operator<<(ostream& os, const VideoMode& vm)
		{
			os << "image=" << vm.image << "  screen=" << vm.screen
				 << "  stretch=" << (vm.stretch == Stretch::Preserve ? "preserve" :
														 vm.stretch == Stretch::Fill ? "fill" : "none")
				 << "  desc=" << vm.description << "  zoom=" << vm.zoom
				 << "  fsIndex= " << vm.fsIndex;
			return os;
		}
	};

	FrameBuffer(OSystem& osystem);

	void render(IG::MutablePixmapView pix, TIA &tia);

	FrameBuffer &tiaSurface() { return *this; }

	// dummy value, not actually needed
	Common::Size desktopSize() const { return Common::Size{1024, 1024}; }

	// no-op, EmuFramework manages window
	FBInitStatus createDisplay(const string& title, BufferType, Common::Size, bool honourHiDPI = true)
	{
		myPaletteHandler.setPalette();
		return FBInitStatus::Success;
	}

	// no-op
	void showFrameStats(bool enable) {}

	void setTIAPalette(const PaletteArray& rgb_palette);

	void setPixelFormat(IG::PixelFormatId);
	IG::PixelFormatId pixelFormat() const;

	void showTextMessage(const string& message,
		MessagePosition position = MessagePosition::BottomCenter,
		bool force = false) const;

	void showGaugeMessage(const string& message, const string& valueText,
		float value, float minValue = 0.F, float maxValue = 100.F) {}

	void enablePhosphor(bool enable, int blend = -1);

	bool phosphorEnabled() const { return myUsePhosphor; }

	uInt8 getPhosphor(const uInt8 c1, uInt8 c2) const;

	uInt16 getRGBPhosphor16(const uInt32 c, const uInt32 p) const;
	uInt32 getRGBPhosphor32(const uInt32 c, const uInt32 p) const;

	void clear() {}

	void updateSurfaceSettings() {}

	const Common::Rect& imageRect() const { return myImageRect; }

	PaletteHandler& paletteHandler() { return myPaletteHandler; }

private:
	EmuEx::EmuApp *appPtr{};
	PaletteHandler myPaletteHandler;
	uInt16 tiaColorMap16[256]{};
	uInt32 tiaColorMap32[256]{};
	uInt8 myPhosphorPalette[256][256]{};
	std::array<uInt8, 160 * TIAConstants::frameBufferHeight> prevFramebuffer{};
	Common::Rect myImageRect{};
	float myPhosphorPercent = 0.80f;
	bool myUsePhosphor{};
	IG::PixelFormatId format;

	std::array<uInt8, 3> getRGBPhosphorTriple(uInt32 c, uInt32 p) const;
	template <int outputBits>
	void renderOutput(IG::MutablePixmapView pix, TIA &tia);
};
