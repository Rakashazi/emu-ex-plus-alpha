include $(IMAGINE_PATH)/make/config.mk
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/pnd-metadata.mk

.PHONY: all
all : pandora-pnd

pandora_buildName ?= $(firstMakefileName:.mk=)
pandora_targetPath ?= target/$(pandora_buildName)
pandora_targetPNDPath := $(pandora_targetPath)/$(pnd_metadata_pndName)
pandora_exec := $(pandora_targetPNDPath)/$(pnd_metadata_exec)
pandora_icon := res/icons/iOS/icon-114.png
pandora_pxml := $(pandora_targetPNDPath)/PXML.xml
pandora_pnd := $(pandora_targetPath)/$(pnd_metadata_pndName).pnd
pandora_installUser := robert
pandora_installHost := pandora
pandora_sdcardPath := /media/mmcblk0p1/pandora
pandora_deviceExecInstallPath := $(pandora_sdcardPath)/$(pnd_metadata_pndName)
pandora_deviceExecPath := $(pandora_deviceExecInstallPath)/$(pnd_metadata_exec)
pandora_devicePNDInstallPath := $(pandora_sdcardPath)/apps
pandora_makefile ?= $(IMAGINE_PATH)/make/shortcut/common-builds/linux-armv7-pandora.mk
pandora_execPath := $(pandora_targetPNDPath)/$(pnd_metadata_exec)
pandora_pndDeps = $(pandora_exec)

ifdef LTO_MODE
 pandora_makefileOpts += LTO_MODE=$(LTO_MODE)
endif

.PHONY: pandora-build
pandora-build :
	@echo "Building Executable"
	$(PRINT_CMD)$(MAKE) -f $(pandora_makefile) $(pandora_makefileOpts) targetDir=$(pandora_targetPNDPath) targetFile=$(pnd_metadata_exec) \
	buildName=$(pandora_buildName) projectPath=$(projectPath)
$(pandora_execPath) : pandora-build

.PHONY: pandora-exec-install
pandora-exec-install : $(pandora_exec)
	ssh $(pandora_installUser)@$(pandora_installHost) mkdir -p $(pandora_deviceExecInstallPath)
	scp $^ $(pandora_installUser)@$(pandora_installHost):$(pandora_deviceExecPath)

$(pandora_pxml) : $(projectPath)/metadata/conf.mk $(metadata_confDeps)
	bash $(IMAGINE_PATH)/tools/genPNDMeta.sh $(pnd_gen_metadata_args) $@
.PHONY: pandora-metadata
pandora-metadata : $(pandora_pxml)

ifneq ($(wildcard $(pandora_icon)),)
pandora_iconPNDPath := $(pandora_targetPNDPath)/icon.png
$(pandora_iconPNDPath) :
	@mkdir -p $(@D)
	ln $(pandora_icon) $@
endif

ifneq ($(wildcard $(projectPath)/res/pandora),)
pandora_resourcePath := res/pandora

.PHONY: pandora-resources
pandora-resources : $(pandora_resourcePath) $(pandora_iconPNDPath) $(pandora_pxml)
	@mkdir -p $(pandora_targetPNDPath)
	@echo linking resource files
	cp -alLu $(pandora_resourcePath)/* $(pandora_targetPNDPath)/

.PHONY: pandora-resources-install
pandora-resources-install : $(pandora_pxml)
	ssh $(pandora_installUser)@$(pandora_installHost) mkdir -p $(pandora_deviceExecInstallPath)
	scp $(pandora_resourcePath)/* $(pandora_installUser)@$(pandora_installHost):$(pandora_deviceExecInstallPath)/
	scp $(pandora_icon) $(pandora_installUser)@$(pandora_installHost):$(pandora_deviceExecInstallPath)/icon.png
	scp $(pandora_pxml) $(pandora_installUser)@$(pandora_installHost):$(pandora_deviceExecInstallPath)/PXML.xml

pandora_pndDeps += pandora-resources
endif

$(pandora_pnd) : $(pandora_pndDeps)
	pnd_make.sh -c -d $(<D) -i $(<D)/icon.png -x $(<D)/PXML.xml -p $@
.PHONY: pandora-pnd
pandora-pnd : $(pandora_pnd)

.PHONY: pandora-pnd-install
pandora-pnd-install : $(pandora_pnd)
	ssh $(pandora_installUser)@$(pandora_installHost) mkdir -p $(pandora_devicePNDInstallPath)
	scp $^ $(pandora_installUser)@$(pandora_installHost):$(pandora_devicePNDInstallPath)/

.PHONY: pandora-pnd-install-only
pandora-pnd-install-only :
	ssh $(pandora_installUser)@$(pandora_installHost) mkdir -p $(pandora_devicePNDInstallPath)
	scp $(pandora_pnd) $(pandora_installUser)@$(pandora_installHost):$(pandora_devicePNDInstallPath)/

.PHONY: pandora-ready
pandora-ready : 
	cp $(pandora_pnd) $(IMAGINE_PATH)/../releases-bin/pandora/$(pnd_metadata_pndName)-$(metadata_version).pnd