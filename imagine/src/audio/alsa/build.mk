ifndef inc_audio
inc_audio := 1

include $(IMAGINE_PATH)/make/package/alsa.mk

configDefs += CONFIG_AUDIO CONFIG_AUDIO_ALSA

SRC += audio/alsa/alsa.cc

endif
