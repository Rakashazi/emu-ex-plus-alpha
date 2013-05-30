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
		--exec=*)
			exec=$optarg
		;;
		--vendor=*)
			vendor=$optarg
		;;
		--website=*)
			website=$optarg
		;;
		--description=*)
			description=$optarg
		;;
		--license=*)
			license=$optarg
		;;
		--license-url=*)
			licenseURL=$optarg
		;;
		--source-code-url=*)
			sourceCodeURL=$optarg
		;;
		--subcategory=*)
			subcategory=$optarg
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

versionArr=( ${version//./ } )
versionMajor=${versionArr[0]}
versionMinor=${versionArr[1]}
versionRelease=${versionArr[2]}

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

if [ ! "$description" ]
then
	description=$name
fi

echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<PXML xmlns=\"http://openpandora.org/namespaces/PXML\">
	<package id=\"${id}\">
		<version major=\"${versionMajor}\" minor=\"${versionMinor}\" release=\"${versionRelease}\" build=\"0\"/>
		<author name=\"${vendor}\" website=\"${website}\"/>
		<titles>
			<title lang=\"en_US\">${name}</title>
		</titles>
		<descriptions>
			<description lang=\"en_US\">${description}</description>
		</descriptions>
		<icon src=\"icon.png\"/>
	</package>

	<application id=\"${id}\">
		<exec command=\"${exec}\" x11=\"req\"/>
		<version major=\"${versionMajor}\" minor=\"${versionMinor}\" release=\"${versionRelease}\" build=\"0\"/>
		<author name=\"${vendor}\" website=\"${website}\"/>
		<titles>
			<title lang=\"en_US\">${name}</title>
		</titles>
		<descriptions>
			<description lang=\"en_US\">${description}</description>
		</descriptions>
		<icon src=\"icon.png\"/>
		<licenses>
			<license name=\"${license}\" url=\"${licenseURL}\" sourcecodeurl=\"${sourceCodeURL}\"/>
		</licenses>
		<previewpics>
			<pic src=\"preview.png\"/>
		</previewpics>
		<categories>
			<category name=\"Game\">
				<subcategory name=\"${subcategory}\"/>
			</category>
		</categories>
	</application>
</PXML>" > $outPath