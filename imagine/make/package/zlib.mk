ifndef inc_pkg_zlib
inc_pkg_zlib := 1

ifneq ($(filter android ios webos,$(ENV)),)
 LDLIBS += -lz
else
 pkgConfigDeps += zlib
endif

endif