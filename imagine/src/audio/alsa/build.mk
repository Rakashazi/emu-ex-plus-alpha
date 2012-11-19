ifndef inc_audio
inc_audio := 1

LDLIBS += -lasound

configDefs += CONFIG_AUDIO CONFIG_AUDIO_ALSA

SRC += audio/alsa/alsa.cc

endif
