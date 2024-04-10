ifndef inc_vmem
inc_vmem := 1

ifeq ($(ENV_KERNEL), linux)
 SRC += vmem/linux.cc
else ifeq ($(ENV_KERNEL), mach)
 SRC += vmem/mach.cc
else
 $(error unsupported ENV_KERNEL)
endif

endif
