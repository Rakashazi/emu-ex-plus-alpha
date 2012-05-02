ifndef inc_audio
inc_audio := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_PS3

SRC += audio/ps3/ps3.cc

LDLIBS += -laudio_stub

endif
