makefilesToRun='
	src/libogg/android-armv7.mk
	
	src/tremor/android-armv7.mk
	
	src/libsndfile/android-armv7.mk
	
	src/minizip/android-armv7.mk
	
	src/boost/android-armv7.mk
'

source runMakefiles.sh

runMakefiles $@

