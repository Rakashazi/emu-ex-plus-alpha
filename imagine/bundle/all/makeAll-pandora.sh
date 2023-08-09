makefilesToRun='
	src/xz/linux-armv7-pandora.mk
	
	src/libarchive/linux-armv7-pandora.mk
'
source runMakefiles.sh

runMakefiles $@

