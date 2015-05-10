makefilesToRun='
	src/libogg/android-arm.mk
	
	src/tremor/android-arm.mk
	
	src/libsndfile/android-arm.mk
	
	src/minizip/android-arm.mk
	
	src/boost/android-arm.mk
'

source runMakefiles.sh

runMakefiles $@

