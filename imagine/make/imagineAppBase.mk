CPPFLAGS += -I$(genPath) -Isrc -I$(imagineSrcDir)
VPATH += src $(imagineSrcDir)

genConfigH = $(genPath)/config.h
genMetaH = $(genPath)/meta.h

.SUFFIXES: 
.PHONY: all main config metadata-header
all : $(genConfigH) $(genMetaH) main

# config.h is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)" "$(configIncNext)"
config : $(genConfigH)

include metadata/conf.mk
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
