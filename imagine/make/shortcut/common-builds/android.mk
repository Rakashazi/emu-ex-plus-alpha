include $(IMAGINE_PATH)/make/config.mk

ifndef android_arch
 ifdef android_ouyaBuild
  android_arch := armv7
 else
  android_arch := armv7 x86
 endif
endif

android_minSDK ?= 9
android_buildPrefix := android

ifdef android_ouyaBuild
 android_minSDK := 16
 android_buildPrefix := android-ouya
endif

ifneq ($(filter arm, $(android_arch)),)
 armTarget := $(android_buildPrefix)-$(android_minSDK)-armv6$(targetExt)
 targets += $(armTarget)
endif
ifneq ($(filter armv7, $(android_arch)),)
 armv7Target := $(android_buildPrefix)-$(android_minSDK)-armv7$(targetExt)
 targets += $(armv7Target)
endif
ifneq ($(filter x86, $(android_arch)),)
 x86Target := $(android_buildPrefix)-$(android_minSDK)-x86$(targetExt)
 targets += $(x86Target)
endif

installTargets := $(targets:=-install)
installLinksTargets := $(targets:=-install-links)
commonBuildPath := $(buildSysPath)/shortcut/common-builds

.PHONY: all $(targets) $(installTargets) $(installLinksTargets)

all : $(targets)
install : $(installTargets)
install-links : $(installLinksTargets)

$(targets) :
	@echo "Performing target $@"
	$(PRINT_CMD)$(MAKE) -f $(commonBuildPath)/$@.mk projectPath=$(projectPath)

$(installTargets) :
	@echo "Performing target $@"
	$(PRINT_CMD)$(MAKE) -f $(commonBuildPath)/$(@:-install=).mk projectPath=$(projectPath) install

$(installLinksTargets) :
	@echo "Performing target $@"
	$(PRINT_CMD)$(MAKE) -f $(commonBuildPath)/$(@:-install-links=).mk projectPath=$(projectPath) install-links
