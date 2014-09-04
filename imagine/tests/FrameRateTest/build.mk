ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/main.cc main/tests.cc main/TestPicker.cc

include $(IMAGINE_PATH)/make/package/imagine.mk

ifndef target
target := FrameRateTest
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
