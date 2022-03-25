#pragma once

#include <stella/emucore/Props.hxx>
#include <stella/emucore/Control.hxx>
#include <emuframework/Option.hh>
#include <optional>

class OSystem;
class Console;

namespace EmuEx
{

class EmuApp;
class EmuSystem;

enum class PaddleRegionMode : uint8_t
{
	OFF = 0,
	LEFT = 1,
	RIGHT = 2,
	FULL = 3,
};

static constexpr uint TV_PHOSPHOR_AUTO = 2;
extern Byte1Option optionTVPhosphor;
extern Byte1Option optionTVPhosphorBlend;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionAudioResampleQuality;
extern Byte1Option optionInputPort1;
extern Byte1Option optionPaddleDigitalSensitivity;
extern Byte1Option optionPaddleAnalogRegion;
extern Properties defaultGameProps;
extern bool p1DiffB, p2DiffB, vcsColor;
extern std::optional<OSystem> osystem;
extern Controller::Type autoDetectedInput1;

const char *optionVideoSystemToStr();
void setRuntimeTVPhosphor(EmuSystem &, int val, int blend);
void setControllerType(EmuApp &, Console &console, Controller::Type type);
Controller::Type limitToSupportedControllerTypes(Controller::Type type);
const char *controllerTypeStr(Controller::Type type);
void updatePaddlesRegionMode(EmuApp &, PaddleRegionMode);

}
