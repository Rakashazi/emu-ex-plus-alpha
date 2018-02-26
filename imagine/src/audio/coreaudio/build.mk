ifndef inc_audio_ca
inc_audio_ca := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_COREAUDIO

SRC +=  audio/OutputStream.cc audio/coreaudio/coreaudio.cc

LDLIBS += -framework AudioToolbox

endif
