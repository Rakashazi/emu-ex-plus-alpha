NEW=
for def in $2; do
	NEW="${NEW}#define ${def/=/ }
"
done

for inc in $3; do
	NEW="${NEW}#include $inc
"
done

for inc in $4; do
	NEW="${NEW}#include_next $inc
"
done

if [ -f $1 ]; then
	OLD=`cat $1`"
"
	#echo -n "$NEW"
	#echo -n "$OLD"
	if [[ "$NEW" == "$OLD" ]]; then
		exit 0
	fi
fi

#echo "writing config to $1"
echo -n "$NEW" > $1
