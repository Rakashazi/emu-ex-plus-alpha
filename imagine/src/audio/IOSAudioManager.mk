ifndef inc_iosAudioManager
inc_iosAudioManager := 1

SRC += audio/IOSAudioManager.mm

LDLIBS += -framework AVFoundation

endif
