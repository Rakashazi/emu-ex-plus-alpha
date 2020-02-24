include $(IMAGINE_PATH)/make/config.mk

prefix ?= $(IMAGINE_SDK_PATH)/android-java

imagineV9SrcPath := $(projectPath)/src/base/android/imagine-v9
imagineV9BuildLibBasePath := $(imagineV9SrcPath)/build/outputs/aar
imagineV9BuildLibPath = $(firstword $(wildcard $(imagineV9BuildLibBasePath)/imagine-v9*.aar))
imagineV9InstallLibPath := $(prefix)/imagine-v9.aar

.PHONY: all build clean install install-links

all : build

build :
	cd $(imagineV9SrcPath) && bash gradlew assembleRelease

clean :
	rm -r $(imagineV9SrcPath)/build

install : build
	@echo "Installing aar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)cp $(imagineV9BuildLibPath) $(imagineV9InstallLibPath)

install-links : build
	@echo "Installing symlink aar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)$(LN) -srf $(imagineV9BuildLibPath) $(imagineV9InstallLibPath)
