if [ "$#" -lt "2" ] ; then
	echo "Usage:  videoToGif.sh  video.mp4  video.gif"
	exit 1
fi

mkdir frames

ffmpeg -i $1 -vf scale=480:-1:flags=lanczos,fps=10 frames/ffout%03d.png

# crop to size that is visible on steam store page (update summary area)
mogrify -crop 480x240+0+0 +repage frames/*

convert -loop 0 -delay 10 -layers Optimize -fuzz "5%" frames/ffout*.png $2

