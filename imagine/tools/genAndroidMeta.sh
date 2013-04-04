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
		--version-code=*)
			versionCode=$optarg
		;;
		--vendor=*)
			vendor=$optarg
		;;
		--min-sdk=*)
			minSDK=$optarg
		;;
		--activity-name=*)
			activityName=$optarg
		;;
		--permission-write-ext)
			writeExtStore=1
		;;
		--permission-bluetooth)
			bluetooth=1
		;;
		--permission-vibrate)
			vibrate=1
		;;
		--xperia-play-optimized)
			xperiaPlayOpt=1
		;;
		--no-icon)
			noIcon=1
		;;
		--ouya-build)
			ouyaBuild=1
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

if [ $xperiaPlayOpt ]
then
	if [ $noIcon ]
	then
		echo "error: must use an icon to be Xperia Play optimized"
		exit 1
	fi
fi

if [ ! "$minSDK" ]
then
	minSDK=9
fi

if [ ! $versionCode ]
then
	minSdkPad=`printf "%02d" $minSDK`
	set -- ${version//./ }
	v1=`printf "%02d" $1`
	v2=`printf "%02d" $2`
	v3=`printf "%02d" $3`
	versionCode=${minSdkPad}${v1}${v2}${v3}
fi

if [ ! "$activityName" ]
then
	activityName=com.imagine.BaseActivity
fi

uiChanges='mcc|mnc|locale|touchscreen|keyboard|keyboardHidden|navigation|screenLayout|fontScale|orientation'
if [ $minSDK -ge 5 ]
then
	uiChanges=${uiChanges}'|uiMode'
fi

if [ $minSDK -ge 9 ]
then
	uiChanges=${uiChanges}'|screenSize|smallestScreenSize'
fi

# start XML
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"
		package=\"$id\""  > $outPath

if [ $minSDK -ge 5 ]
then
	echo '		android:installLocation="auto"' >> $outPath
fi

echo "		android:versionCode=\"$versionCode\" android:versionName=\"$version\">" >> $outPath 

if [ $writeExtStore ]
then
	echo '	<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />' >> $outPath
fi

if [ $bluetooth ]
then
	echo '	<uses-permission android:name="android.permission.BLUETOOTH" />
	<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />' >> $outPath
	if [ $minSDK -ge 5 ]
	then
	echo '	<uses-feature android:name="android.hardware.bluetooth" android:required="false" />' >> $outPath
	fi
fi

if [ $vibrate ]
then
	echo '	<uses-permission android:name="android.permission.VIBRATE" />' >> $outPath
fi

if [ $minSDK -ge 9 ]
then
	echo '	<supports-screens android:xlargeScreens="true" />' >> $outPath
	echo '	<uses-feature android:name="android.hardware.touchscreen" android:required="false" />' >> $outPath
	targetSDKOutput='android:targetSdkVersion="17"'
fi

if [ $noIcon ]
then
	iconOutput=
else
	iconOutput="android:icon=\"@drawable/icon\""
fi

intentFilters="<action android:name=\"android.intent.action.MAIN\" />
				<category android:name=\"android.intent.category.LAUNCHER\" />"

if [ $ouyaBuild ]
then
	intentFilters="$intentFilters
				<category android:name=\"tv.ouya.intent.category.GAME\" />"
fi

echo "	<uses-sdk android:minSdkVersion=\"$minSDK\" ${targetSDKOutput} />
	<application android:label=\"@string/app_name\" $iconOutput>
		<activity android:name=\"$activityName\"
				android:label=\"@string/app_name\"
				android:theme=\"@android:style/Theme.NoTitleBar.Fullscreen\"
				android:screenOrientation=\"unspecified\"
				android:configChanges=\"$uiChanges\"
				android:launchMode=\"singleInstance\">
			<intent-filter>
				$intentFilters
			</intent-filter>
		</activity>" >> $outPath

if [ $xperiaPlayOpt ]
then
	if [ $minSDK -ge 9 ]
	then
		echo '	<meta-data android:name="xperiaplayoptimized_content" android:resource="@drawable/iconbig" />
	<meta-data android:name="game_icon" android:resource="@drawable/iconbig" />' >> $outPath
	fi
fi

echo '	</application>
</manifest>' >> $outPath
