#!/bin/sh

if [ ! -e minorGems ]
then
	git clone https://github.com/skps2010/minorGems.git	
fi

if [ ! -e OneLife ]
then
	git clone https://github.com/skps2010/OneLife.git
fi

if [ ! -e OneLifeData7 ]
then
	git clone https://github.com/jasonrohrer/OneLifeData7.git	
fi


# cd minorGems
# git fetch --tags
# latestTaggedVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
# git checkout -q OneLife_v$latestTaggedVersion


# cd OneLife
# git fetch --tags
# latestTaggedVersionA=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
# git checkout -q OneLife_v$latestTaggedVersionA


cd OneLifeData7
git fetch --tags
# remove translated objects
git checkout .
latestTaggedVersionB=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
git checkout -q OneLife_v$latestTaggedVersionB

rm */cache.fcz


latestVersion=$latestTaggedVersionB

# if [ "$latestTaggedVersionA" -gt "$latestTaggedVersionB" ]
# then
# 	latestVersion=$latestTaggedVersionA
# fi



cd ..


if [ ! -h animations ]
then
	ln -s OneLifeData7/animations .	
fi


if [ ! -h categories ]
then
	ln -s OneLifeData7/categories .	
fi


if [ ! -h ground ]
then
	ln -s OneLifeData7/ground .	
fi


if [ ! -h music ]
then
	ln -s OneLifeData7/music .	
fi


if [ ! -h objects ]
then
	ln -s OneLifeData7/objects .	
fi


if [ ! -h sounds ]
then
	ln -s OneLifeData7/sounds .	
fi


if [ ! -h sprites ]
then
	ln -s OneLifeData7/sprites .	
fi


if [ ! -h transitions ]
then
	ln -s OneLifeData7/transitions .	
fi


if [ ! -h dataVersionNumber.txt ]
then
	ln -s OneLifeData7/dataVersionNumber.txt .	
fi




cp OneLife/build/source/runToBuild .
cp OneLife/scripts/skps2010Scripts/cleanOldBuilds.sh .
cp OneLife/scripts/skps2010Scripts/makeWindows.sh .
cp OneLife/scripts/skps2010Scripts/translator.py .


if [ "$(uname)" = "Darwin" ]; then
    ./runToBuild 2
else
    ./runToBuild 1
fi


echo
echo
echo "Done building v$latestVersion"
