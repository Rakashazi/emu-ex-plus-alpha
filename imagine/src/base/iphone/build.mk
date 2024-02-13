ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk
include $(imagineSrcDir)/util/fdUtils.mk

LDLIBS += -framework UIKit -framework QuartzCore -framework Foundation -framework CoreFoundation -framework CoreGraphics

ifdef iosMsgUI
 configEnable += IPHONE_MSG_COMPOSE
 LDLIBS += -framework MessageUI
endif

SRC += base/iphone/iphone.mm \
 base/iphone/IOSWindow.mm \
 base/iphone/IOSScreen.mm \
 base/iphone/EAGLView.mm \
 base/iphone/input.mm \
 base/iphone/IOSGLContext.mm \
 base/common/timer/CFTimer.cc \
 base/common/PosixPipe.cc \
 base/common/eventloop/CFEventLoop.cc \
 base/common/eventloop/FDCustomEvent.cc \
 util/string/apple.mm

endif
