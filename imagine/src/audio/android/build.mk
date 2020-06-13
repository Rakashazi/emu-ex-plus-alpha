ifndef inc_audio_aaudio
inc_audio_aaudio := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_AAUDIO

SRC += audio/OutputStream.cc audio/android/aaudio.cc

endif
