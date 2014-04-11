ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/stdc++.mk

configDefs += CONFIG_BASE_IOS CONFIG_INPUT

ifdef iosJailbreak
 configDefs += CONFIG_BASE_IOS_JB
else
 iOSNoCodesign := 1
endif

LDLIBS += -framework UIKit -framework QuartzCore -framework Foundation -framework CoreFoundation -framework CoreGraphics -ObjC

ifdef iosMsgUI
 configDefs += IPHONE_MSG_COMPOSE
 LDLIBS += -framework MessageUI
endif

SRC += base/iphone/iphone.mm base/iphone/IOSWindow.mm base/iphone/EAGLView.mm \
 base/common/timer/CFTimer.cc base/common/PosixPipe.cc base/common/eventloop/CFEventLoop.cc util/string/apple.mm

endif
