include $(IMAGINE_PATH)/make/config.mk

ios_arch ?= armv7 arm64

ifneq ($(filter armv7, $(ios_arch)),)
 armv7Target := ios-armv7$(targetExt)
 targets += $(armv7Target)
endif
ifneq ($(filter armv7s, $(ios_arch)),)
 armv7sTarget := ios-armv7s$(targetExt)
 targets += $(armv7sTarget)
endif
ifneq ($(filter arm64, $(ios_arch)),)
 arm64Target := ios-arm64$(targetExt)
 targets += $(arm64Target)
endif
ifneq ($(filter x86, $(ios_arch)),)
 x86Target := ios-x86$(targetExt)
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