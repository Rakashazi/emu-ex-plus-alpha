ifndef inc_audio
inc_audio := 1

include $(IMAGINE_PATH)/make/package/sdl.mk

configDefs += CONFIG_AUDIO CONFIG_AUDIO_SDL

SRC += audio/sdl/sdl.cc

endif
