ifndef inc_iosAudioManager
inc_iosAudioManager := 1

SRC += audio/coreaudio/AvfManager.mm

LDLIBS += -framework AVFoundation

endif
