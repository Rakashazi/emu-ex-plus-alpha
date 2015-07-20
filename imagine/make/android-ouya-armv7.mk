include $(IMAGINE_PATH)/make/config.mk

configDefs += CONFIG_MACHINE_OUYA

# using cortex-a9 for -mcpu or -mtune can cause crashes from bad alignment (SIGBUS & BUS_ADRALN)
armv7CPUFlags := -march=armv7-a -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon

include $(buildSysPath)/android-armv7-gcc.mk
