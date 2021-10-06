ifndef inc_audio_alsa
inc_audio_alsa := 1

include $(IMAGINE_PATH)/make/package/alsa.mk

configDefs += CONFIG_AUDIO_ALSA

SRC += audio/OutputStream.cc audio/alsa/alsa.cc

endif
