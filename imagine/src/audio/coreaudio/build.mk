ifndef inc_audio
inc_audio := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_COREAUDIO

SRC +=  audio/coreaudio/coreaudio.cc

LDLIBS += -framework AudioToolbox -framework CoreAudio

endif
