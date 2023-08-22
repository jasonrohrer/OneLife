#!/bin/sh


if [ "$2" = "" ]
then
	echo ""
	echo "Error: not enough arguments"
	echo ""
	echo "first argument needs to be the name of a folder"
	echo "second argument needs to be the os"
	echo ""
	echo "for example:"
	echo "$0 linux 1"
	echo "$0 windows 3"
	exit 1
fi

if [ "$2" != "1" ]
then
	if [ "$2" != "3" ]
	then
		echo "Invalid argument '$2'"
		echo "To get more information type $0"
		exit 1
	fi
fi

if [ ! -e OneLifeData7 ]
then
	git clone https://github.com/jasonrohrer/OneLifeData7	
fi

cd OneLifeData7
git fetch --tags
latestTaggedVersionB=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
git checkout -q OneLife_v$latestTaggedVersionB
cd ..


# SDL lib for linux-windows cross compile
if [ "$2" = "3" ]
then
	if [ ! -e SDL-1.2.15 ]
	then
		wget https://www.libsdl.org/release/SDL-devel-1.2.15-mingw32.tar.gz
		tar -xvzf SDL-devel-1.2.15-mingw32.tar.gz
		rm SDL-devel-1.2.15-mingw32.tar.gz
	fi
fi


if [ ! -e $1 ]
then
	mkdir $1
fi

cd $1


# create link to SDL headers (for windows)
if [ "$2" = "3" ]
then
	if [ ! -e SDL ]
	then
		ln -s ../SDL-1.2.15/include/SDL .
	fi
fi


# link Winsock.h to winsock.h (to circumvent case sensitive headers) (for windows)
if [ "$2" = "3" ]
then
	if [ ! -e Winsock.h ]
	then
		ln -s /usr/i686-w64-mingw32/include/winsock.h Winsock.h
	fi
fi


if [ ! -e minorGems ]
then
	git clone https://github.com/skps2010/minorGems
fi

if [ ! -e OneLife ]
then
	git clone https://github.com/skps2010/OneLife
fi


cd minorGems
git pull
#git fetch --tags
#latestTaggedVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
#git checkout -q OneLife_v$latestTaggedVersion
cd ..

cd OneLife
git pull
#git fetch --tags
#latestTaggedVersionA=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
#git checkout -q OneLife_v$latestTaggedVersionA

rm */cache.fcz

cd ..


latestVersion=$latestTaggedVersionB

#if [ $latestTaggedVersionA -gt $latestTaggedVersionB ]
#then
#	latestVersion=$latestTaggedVersionA
#fi


if [ ! -h OneLifeData7 ]
then
	ln -s ../OneLifeData7 .
fi

if [ ! -h animations ]
then
	ln -s ../OneLifeData7/animations .	
fi

if [ ! -h categories ]
then
	ln -s ../OneLifeData7/categories .	
fi

if [ ! -h ground ]
then
	ln -s ../OneLifeData7/ground .	
fi

if [ ! -h music ]
then
	ln -s ../OneLifeData7/music .	
fi

if [ ! -h objects ]
then
	ln -s ../OneLifeData7/objects .	
fi

if [ ! -h sounds ]
then
	ln -s ../OneLifeData7/sounds .	
fi

if [ ! -h sprites ]
then
	ln -s ../OneLifeData7/sprites .	
fi

if [ ! -h transitions ]
then
	ln -s ../OneLifeData7/transitions .	
fi

if [ ! -h dataVersionNumber.txt ]
then
	ln -s ../OneLifeData7/dataVersionNumber.txt .	
fi


cp OneLife/build/source/runToBuild .

echo
echo "Done downloading"
echo "In order to compile type the following:"
echo
echo "cd $1"

if [ "$2" = "1" ] # linux
then
	echo "./runToBuild 1"
fi

if [ "$2" = "3" ] # windows
then
	echo "./runToBuild 5"
fi

