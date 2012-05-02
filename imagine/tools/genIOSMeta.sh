for arg in "$@"
do
	case $arg in
		*=*) optarg=`expr "X$arg" : '[^=]*=\(.*\)'` ;;
	esac
	
	case "$arg" in
		--name=*)
			name=$optarg
		;;
		--id=*)
			id=$optarg
		;;
		--version=*)
			version=$optarg
		;;
		--bundle-name=*)
			bundleName=$optarg
		;;
		--exec=*)
			exec=$optarg
		;;
		--vendor=*)
			vendor=$optarg
		;;
		--file-share)
			fileShare=1
		;;
		--setuid)
			setuid=1
		;;
		# special actions
		-v | --verbose)
			verbose=1
		;;
		-*)
			echo "warning: unknown parameter $arg"
		;;
		*)
			outPath=$arg
		;;
	esac
done

if [ ! "$name" ]
then
	echo "error: no name specified"
	exit 1
fi

if [ ! $id ]
then
	echo "error: no id specified"
	exit 1
fi

if [ ! $version ]
then
	echo "error: no version specified"
	exit 1
fi

if [ ! $bundleName ]
then
	echo "error: no bundle name specified"
	exit 1
fi

if [ ! $exec ]
then
	echo "error: no executable name specified"
	exit 1
fi

if [ ! "$vendor" ]
then
	echo "error: no vendor specified"
	exit 1
fi

if [ ! "$outPath" ]
then
	echo "error: no output file specified"
	exit 1
fi

if [ $setuid ]
then
	exec=${exec}_
fi

echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>

	<key>CFBundleName</key>
	<string>$bundleName</string>
	
	<key>CFBundleIdentifier</key>
	<string>$id</string>
	
	<key>CFBundleExecutable</key>
	<string>$exec</string>
	
	<key>CFBundleDisplayName</key>
	<string>$name</string>
	
	<key>CFBundleVersion</key>
	<string>2</string>

	<key>CFBundleShortVersionString</key>
	<string>$version</string>

	<key>UIStatusBarStyle</key>
	<string>UIStatusBarStyleBlackOpaque</string>

	<key>UIStatusBarHidden</key>
	<true/>
	
<!--
	Common app keys
-->
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>

	<key>CFBundlePackageType</key>
	<string>APPL</string>

	<key>CFBundleIconFile</key>
	<string>icon-57.png</string>

	<key>DTPlatformName</key>
	<string>iphoneos</string>

	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>iPhoneOS</string>
	</array>

	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>

	<key>MinimumOSVersion</key>
	<string>3.1</string>

	<key>CFBundleIconFiles</key>
	<array>
		<string>icon-57.png</string>
		<string>icon-72.png</string>
		<string>icon-114.png</string>
	</array>

	<key>LSRequiresIPhoneOS</key>
	<true/>

	<key>UISupportedInterfaceOrientations</key>
	<array>
		<string>UIInterfaceOrientationPortrait</string>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>

	<key>CFBundleSignature</key>
	<string>????</string>

	<key>UIDeviceFamily</key>
	<array>
		<integer>1</integer>
		<integer>2</integer>
	</array>

" > $outPath 

if [ $fileShare ]
then
	echo "<key>UIFileSharingEnabled</key>
		<true/>
" >> $outPath
fi

echo "	</dict>
</plist>" >> $outPath