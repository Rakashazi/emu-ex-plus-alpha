makefilesToRun='
	src/btstack/ios-armv6.mk

	src/libogg/ios-armv6.mk
	
	src/tremor/ios-armv6.mk
	
	src/libsndfile/ios-armv6.mk
	
	src/minizip/ios-armv6.mk
	
	src/boost/ios-armv6.mk
	
	src/libcxxabi/ios-armv6.mk
	src/libcxx/ios-armv6.mk
'

source runMakefiles.sh

runMakefiles $@

