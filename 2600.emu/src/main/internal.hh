#pragma once

class OSystem;
#include <stella/emucore/Props.hxx>
#include <emuframework/Option.hh>

static constexpr uint TV_PHOSPHOR_AUTO = 2;
extern Byte1Option optionTVPhosphor;
extern Byte1Option optionTVPhosphorBlend;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionAudioResampleQuality;
extern Properties defaultGameProps;
extern bool p1DiffB, p2DiffB, vcsColor;
extern std::unique_ptr<OSystem> osystem;

const char *optionVideoSystemToStr();
void setRuntimeTVPhosphor(int val, int blend);
