include $(IMAGINE_PATH)/make/config.mk
LTO_MODE ?= lto
pandora_makefileOpts ?= O_RELEASE=1
pandora_imagineLibPath ?= $(IMAGINE_PATH)/lib/pandora-release
pandora_imagineIncludePath ?= $(IMAGINE_PATH)/build/pandora-release/gen
include $(buildSysPath)/shortcut/meta-builds/pandora.mk