ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk

configDefs += CONFIG_BASE_IOS

ifdef iosJailbreak
 configDefs += CONFIG_BASE_IOS_JB
else
 iOSNoCodesign := 1
endif

LDLIBS += -framework UIKit -framework QuartzCore -framework Foundation -framework CoreFoundation -framework CoreGraphics -fobjc-link-runtime

ifdef iosMsgUI
 configDefs += IPHONE_MSG_COMPOSE
 LDLIBS += -framework MessageUI
endif

SRC += base/iphone/iphone.mm base/iphone/IOSWindow.mm \
 base/iphone/IOSScreen.mm base/iphone/EAGLView.mm \
 base/iphone/input.mm base/iphone/IOSGLContext.mm \
 base/common/timer/CFTimer.cc base/common/PosixPipe.cc \
 base/common/eventloop/CFEventLoop.cc util/string/apple.mm

endif
