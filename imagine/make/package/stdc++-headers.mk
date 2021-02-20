ifndef inc_pkg_stdcxx_headers
inc_pkg_stdcxx_headers := 1

ifneq ($(filter macosx ios, $(ENV)),)
 include $(buildSysPath)/package/stdc++.mk
endif

endif