makefilesToRun='
	src/libogg/android-arm64.mk
	
	src/libvorbis/android-arm64.mk
	
	src/libsndfile/android-arm64.mk
		
	src/xz/android-arm64.mk
	
	src/libarchive/android-arm64.mk
	
	src/boost/android-arm64.mk
'

source runMakefiles.sh

runMakefiles $@

