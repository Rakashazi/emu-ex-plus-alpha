ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk
include $(IMAGINE_PATH)/make/package/egl.mk

configDefs += CONFIG_BASE_ANDROID CONFIG_INPUT_ANDROID

SRC += base/android/android.cc base/android/AndroidWindow.cc \
 base/android/AndroidScreen.cc base/android/ALooperEventLoop.cc \
 base/common/timer/TimerFD.cc base/common/PosixPipe.cc \
 base/android/AndroidGLContext.cc base/common/EGLContextBase.cc \
 base/android/FrameTimer.cc base/android/intent.cc \
 base/android/inputConfig.cc base/android/textInput.cc \
 base/android/input.cc base/android/system.cc \
 base/android/surfaceTexture.cc \
 base/android/privateApi/libhardware.c \
 base/android/privateApi/GraphicBuffer.cc
LDLIBS += -landroid

ifeq (,$(findstring $(configDefs),CONFIG_MACHINE_OUYA))
 configDefs += CONFIG_INPUT_ANDROID_MOGA
 SRC += base/android/moga.cc
endif

endif
