makefilesToRun='
	src/btstack/ios-armv6.mk

	src/libogg/ios-armv6.mk
	
	src/tremor/ios-armv6.mk
	
	src/flac/ios-armv6.mk
	
	src/xz/ios-armv6.mk
	
	src/libarchive/ios-armv6.mk

	src/libcxx/ios-armv6.mk
'

source runMakefiles.sh

runMakefiles $@

