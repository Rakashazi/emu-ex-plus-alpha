ifndef inc_base
inc_base := 1

configDefs += CONFIG_BASE_IOS CONFIG_INPUT

ifdef config_ios_jb
 configDefs += CONFIG_BASE_IOS_JB
else
 iOSNoCodesign := 1
endif

LDLIBS += -framework UIKit -framework QuartzCore -framework Foundation -framework CoreFoundation -framework CoreGraphics -lobjc
#-multiply_defined suppress

ifdef iOSMsgUI
	configDefs += IPHONE_MSG_COMPOSE
	LDLIBS += -framework MessageUI
endif

SRC += base/iphone/iphone.mm util/string/apple.mm

endif
