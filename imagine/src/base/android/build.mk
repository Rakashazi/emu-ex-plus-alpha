ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk
include $(IMAGINE_PATH)/make/package/egl.mk
include $(imagineSrcDir)/util/fdUtils.mk

SRC += base/android/android.cc \
base/android/AndroidWindow.cc \
base/android/AndroidScreen.cc \
base/android/ALooperEventLoop.cc \
base/android/AndroidGLContext.cc \
base/android/FrameTimer.cc \
base/android/HardwareBuffer.cc \
base/android/intent.cc \
base/android/inputConfig.cc \
base/android/textInput.cc \
base/android/input.cc \
base/android/moga.cc \
base/android/Sensor.cc \
base/android/system.cc \
base/android/surfaceTexture.cc \
base/android/RootCpufreqParamSetter.cc \
base/android/privateApi/libhardware.c \
base/android/privateApi/GraphicBuffer.cc \
base/common/timer/TimerFD.cc \
base/common/eventloop/FDCustomEvent.cc \
base/common/PosixPipe.cc \
base/common/EGLContextBase.cc \
base/common/SimpleFrameTimer.cc \
util/jni.cc

LDLIBS += -landroid

endif
