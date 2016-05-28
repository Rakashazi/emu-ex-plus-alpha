include $(EMUFRAMEWORK_PATH)/metadata/conf.mk
metadata_name = NEO.emu
metadata_exec = neoemu
metadata_pkgName = NeoEmu
metadata_supportedMIMETypes = application/zip
metadata_id = com.explusalpha.NeoEmu
metadata_vendor = Robert Broglia
webOS_metadata_requiredMemory = 120
pnd_metadata_description = Neo Geo emulator using components from Gngeo
ifeq ($(android_arch), armv7)
 # TODO: TEXRELs in ARM assembly code
 android_metadata_target_sdk = 22
endif
