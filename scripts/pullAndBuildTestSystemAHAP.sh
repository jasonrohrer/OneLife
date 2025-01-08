#!/bin/sh

if [ ! -e minorGems ]
then
	git clone https://github.com/jasonrohrer/minorGems.git	
fi

if [ ! -e OneLife ]
then
	git clone https://github.com/jasonrohrer/OneLife.git
fi

if [ ! -e OneLifeData7 ]
then
	git clone https://github.com/jasonrohrer/AnotherPlanetData.git	
fi



cd minorGems
git pull --tags

cd ../OneLife
git pull --tags

cd ../AnotherPlanetData
git pull --tags

rm */cache.fcz
rm */bin_*cache.fcz


cd ../OneLife

./configure 1

cd gameSource

make

echo 1 > settings/useCustomServer.ini

sh ./makeEditor.sh

ln -s ../../AnotherPlanetData/animations .
ln -s ../../AnotherPlanetData/categories .
ln -s ../../AnotherPlanetData/ground .
ln -s ../../AnotherPlanetData/music .
ln -s ../../AnotherPlanetData/objects .
ln -s ../../AnotherPlanetData/overlays .
ln -s ../../AnotherPlanetData/scenes .
ln -s ../../AnotherPlanetData/sounds .
ln -s ../../AnotherPlanetData/sprites .
ln -s ../../AnotherPlanetData/transitions .
ln -s ../../AnotherPlanetData/contentSettings .
ln -s ../../AnotherPlanetData/dataVersionNumber.txt .


cd ../server
./configure 1
make


ln -s ../../AnotherPlanetData/categories .
ln -s ../../AnotherPlanetData/objects .
ln -s ../../AnotherPlanetData/transitions .
ln -s ../../AnotherPlanetData/tutorialMaps .
ln -s ../../AnotherPlanetData/contentSettings .
ln -s ../../AnotherPlanetData/dataVersionNumber.txt .


git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt


echo 0 > settings/requireTicketServerCheck.ini
echo 1 > settings/forceEveLocation.ini
