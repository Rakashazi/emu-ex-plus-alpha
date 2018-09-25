include $(IMAGINE_PATH)/make/config.mk

prefix ?= $(IMAGINE_SDK_PATH)/android-java

imagineV9SrcPath := $(projectPath)/src/base/android/imagine-v9
imagineV9BuildJarPath := $(imagineV9SrcPath)/build/outputs/aar/imagine-v9-release.aar
imagineV9InstallJarPath := $(prefix)/imagine-v9.aar

.PHONY: all build clean install install-links

all : build

build :
	cd $(imagineV9SrcPath) && bash gradlew assembleRelease

clean :
	rm -r $(imagineV9SrcPath)/build

install : build
	@echo "Installing aar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)cp $(imagineV9BuildJarPath) $(imagineV9InstallJarPath)

install-links : build
	@echo "Installing symlink aar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)$(LN) -srf $(imagineV9BuildJarPath) $(imagineV9InstallJarPath)
