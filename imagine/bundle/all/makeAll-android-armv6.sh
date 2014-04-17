makefilesToRun='
	src/libogg/android-armv6.mk
	
	src/tremor/android-armv6.mk
	
	src/libsndfile/android-armv6.mk
	
	src/minizip/android-armv6.mk
	
	src/boost/android-armv6.mk
'

source runMakefiles.sh

runMakefiles $@

