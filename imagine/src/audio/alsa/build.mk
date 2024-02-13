ifndef inc_audio_alsa
inc_audio_alsa := 1

include $(IMAGINE_PATH)/make/package/alsa.mk

SRC += audio/OutputStream.cc audio/alsa/alsa.cc

endif
