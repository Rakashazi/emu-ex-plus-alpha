makefilesToRun='
	src/xz/linux-armv7-pandora.mk
	
	src/libarchive/linux-armv7-pandora.mk

	src/minizip/linux-armv7-pandora.mk
	
	src/boost/linux-armv7-pandora.mk
'
source runMakefiles.sh

runMakefiles $@

