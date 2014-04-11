CPPFLAGS += -I$(genPath)
VPATH += $(projectPath)/src

ifdef makeConfigH
 genConfigH = $(genPath)/config.h
endif

genMetaH = $(genPath)/meta.h

.SUFFIXES: 
.PHONY: all main config metadata-header
all : $(genConfigH) $(genMetaH) main

ifdef makeConfigH
# config.h is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)" "$(configIncNext)"
config : $(genConfigH)
endif

include $(projectPath)/metadata/conf.mk
$(genMetaH) :
	@echo "Generating Metadata Header $@"
	@mkdir -p $(@D)
	echo \#pragma once > $@
	echo \#define CONFIG_APP_NAME \"$(metadata_name)\" > $@
	echo \#define CONFIG_APP_ID \"$(metadata_id)\" >> $@
ifeq ($(ENV), ps3)
	echo \#define CONFIG_PS3_PRODUCT_ID \"$(ps3_productid)\" >> $@
endif
metadata-header : $(genMetaH)
