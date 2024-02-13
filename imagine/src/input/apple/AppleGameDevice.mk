ifndef inc_input_apple
inc_input_apple := 1

include $(imagineSrcDir)/input/build.mk

LDLIBS += -weak_framework GameController

SRC += input/apple/AppleGameDevice.mm

endif
