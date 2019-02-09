if [ "$#" -lt "1" ] ; then
	echo "Usage:  makeTwoGifs.sh  name"
	exit 1
fi

rm -rf frames

nameLong=$1
nameShort=$1

nameLong+="Long"
nameShort+="Short"



~/cpp/OneLife/documentation/press/videoToGif.sh ${1}Long.mp4 ${1}Long.gif

rm -rf frames

~/cpp/OneLife/documentation/press/videoToGif.sh ${1}Short.mp4 ${1}Short.gif

