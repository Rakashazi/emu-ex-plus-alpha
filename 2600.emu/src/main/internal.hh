#pragma once

#include <stella/emucore/Props.hxx>
#include <stella/emucore/Control.hxx>
#include <emuframework/Option.hh>
#include <optional>

class OSystem;
class Console;
class EmuApp;

static constexpr uint TV_PHOSPHOR_AUTO = 2;
extern Byte1Option optionTVPhosphor;
extern Byte1Option optionTVPhosphorBlend;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionAudioResampleQuality;
extern Byte1Option optionInputPort1;
extern Byte1Option optionPaddleDigitalSensitivity;
extern Properties defaultGameProps;
extern bool p1DiffB, p2DiffB, vcsColor;
extern std::optional<OSystem> osystem;
extern Controller::Type autoDetectedInput1;

const char *optionVideoSystemToStr();
void setRuntimeTVPhosphor(int val, int blend);
void setControllerType(EmuApp &, Console &console, Controller::Type type);
Controller::Type limitToSupportedControllerTypes(Controller::Type type);
const char *controllerTypeStr(Controller::Type type);
