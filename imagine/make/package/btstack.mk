ifndef inc_pkg_btstack
inc_pkg_btstack := 1

ifdef btstackIncludePath
 CPPFLAGS += -I$(btstackIncludePath)
else
 pkgConfigAddImagineSDKIncludePath := 1
endif

LDLIBS += -lBTstack

endif