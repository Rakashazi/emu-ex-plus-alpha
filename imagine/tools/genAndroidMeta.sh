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
		--intent-mimetypes=*)
			intentMimeTypes="$optarg"
		;;
		--intent-file-extensions=*)
			intentFileExtensions="$optarg"
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
		--permission-install-shortcut)
			installShortcut=1
		;;
		--xperia-play-optimized)
			xperiaPlayOpt=1
		;;
		--tv)
			isTV=1
		;;
		--game)
			isGame=1
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
	isGame=1
fi

if [ ! $noIcon ]
then
	applicationOutput="$applicationOutput android:icon=\"@drawable/icon\""
fi

if [ $isTV ]
then
	applicationOutput="$applicationOutput android:banner=\"@drawable/banner\""
fi

if [ $isGame ]
then
	applicationOutput="$applicationOutput android:isGame=\"true\""
fi

if [ ! "$minSDK" ]
then
	minSDK=9
fi

if [ $minSDK -lt 9 ]
then
	echo "error: --min-sdk must be at least 9"
	exit 1
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

uiChanges='mcc|mnc|locale|touchscreen|keyboard|keyboardHidden|navigation|screenLayout|fontScale|orientation|uiMode|screenSize|smallestScreenSize'

# start XML
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"
		package=\"$id\""  > $outPath

echo '		android:installLocation="auto"' >> $outPath

echo "		android:versionCode=\"$versionCode\" android:versionName=\"$version\">" >> $outPath 

if [ $writeExtStore ]
then
	echo '	<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />' >> $outPath
	echo '	<uses-permission android:name="android.permission.WRITE_MEDIA_STORAGE" />' >> $outPath
fi

if [ $bluetooth ]
then
	echo '	<uses-permission android:name="android.permission.BLUETOOTH" />
	<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
	<uses-feature android:name="android.hardware.bluetooth" android:required="false" />' >> $outPath
fi

if [ $isGame ]
then
	echo '	<uses-feature android:name="android.hardware.gamepad" android:required="false" />' >> $outPath
fi

if [ $vibrate ]
then
	echo '	<uses-permission android:name="android.permission.VIBRATE" />' >> $outPath
fi

if [ $installShortcut ]
then
	echo '	<uses-permission android:name="com.android.launcher.permission.INSTALL_SHORTCUT" />' >> $outPath
	#echo '	<uses-permission android:name="com.android.launcher.permission.UNINSTALL_SHORTCUT" />' >> $outPath
fi

echo '	<supports-screens android:largeScreens="true" android:xlargeScreens="true" />' >> $outPath
echo '	<uses-feature android:name="android.hardware.touchscreen" android:required="false" />' >> $outPath
targetSDKOutput='android:targetSdkVersion="21"'

intentFilters="<action android:name=\"android.intent.action.MAIN\" />
				<category android:name=\"android.intent.category.LAUNCHER\" />
				<category android:name=\"android.intent.category.LEANBACK_LAUNCHER\" />"

if [ $ouyaBuild ]
then
	intentFilters="$intentFilters
				<category android:name=\"tv.ouya.intent.category.GAME\" />"
fi

fileIntentFilters=

if [ "$intentMimeTypes" ]
then
	fileIntentFilters="$fileIntentFilters"'
			<intent-filter>
				<action android:name="android.intent.action.VIEW" />
				<category android:name="android.intent.category.DEFAULT" />
				<category android:name="android.intent.category.BROWSABLE" />
'
	for type in $intentMimeTypes
	do
		fileIntentFilters="$fileIntentFilters				<data android:mimeType=\"$type\"/>
"
	done
	fileIntentFilters="$fileIntentFilters"'			</intent-filter>'
fi

if [ "$intentFileExtensions" ]
then
	fileIntentFilters="$fileIntentFilters"'
			<intent-filter>
				<action android:name="android.intent.action.VIEW" />
				<category android:name="android.intent.category.DEFAULT" />
				<category android:name="android.intent.category.BROWSABLE" />
				<data android:scheme="file" />
				<data android:mimeType="*/*" />
				<data android:host="*" />
'
	for extension in $intentFileExtensions
	do
		fileIntentFilters="$fileIntentFilters				<data android:pathPattern=\".*\\\\.$extension\" />
"
	done
	fileIntentFilters="$fileIntentFilters"'			</intent-filter>
'
fi

echo "	<uses-sdk android:minSdkVersion=\"$minSDK\" ${targetSDKOutput} />
	<application android:label=\"@string/app_name\" $applicationOutput>
		<activity android:name=\"$activityName\"
				android:label=\"@string/app_name\"
				android:theme=\"@android:style/Theme.NoTitleBar\"
				android:configChanges=\"$uiChanges\"
				android:launchMode=\"singleInstance\">
			<intent-filter>
				$intentFilters
			</intent-filter>
$fileIntentFilters
		</activity>" >> $outPath

if [ $xperiaPlayOpt ]
then
	echo '		<meta-data android:name="xperiaplayoptimized_content" android:resource="@drawable/iconbig" />
		<meta-data android:name="game_icon" android:resource="@drawable/iconbig" />' >> $outPath
fi

echo '	</application>
</manifest>' >> $outPath
