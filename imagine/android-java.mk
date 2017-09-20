include $(IMAGINE_PATH)/make/config.mk

prefix ?= $(IMAGINE_SDK_PATH)/android-java

imagineV9SrcPath := $(projectPath)/src/base/android/imagine-v9
imagineV9BuildJarPath := $(imagineV9SrcPath)/build/intermediates/bundles/default/classes.jar
imagineV9InstallJarPath := $(prefix)/imagine-v9.jar

.PHONY: all build clean install install-links

all : build

build :
	cd $(imagineV9SrcPath) && bash gradlew assembleRelease

clean :
	rm -r $(imagineV9SrcPath)/build

install : build
	@echo "Installing jar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)cp $(imagineV9BuildJarPath) $(imagineV9InstallJarPath)

install-links : build
	@echo "Installing symlink jar to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)
	$(PRINT_CMD)$(LN) -srf $(imagineV9BuildJarPath) $(imagineV9InstallJarPath)
