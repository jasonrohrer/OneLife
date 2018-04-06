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
	git clone https://github.com/jasonrohrer/OneLifeData7.git	
fi



cd minorGems
git pull --tags

cd ../OneLife
git pull --tags

cd ../OneLifeData7
git pull --tags

rm */cache.fcz


cd ../OneLife

./configure 1

cd gameSource

make

echo 1 > settings/useCustomServer.ini

sh ./makeEditor.sh

ln -s ../../OneLifeData7/animations .
ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/ground .
ln -s ../../OneLifeData7/music .
ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/overlays .
ln -s ../../OneLifeData7/scenes .
ln -s ../../OneLifeData7/sounds .
ln -s ../../OneLifeData7/sprites .
ln -s ../../OneLifeData7/transitions .


cd ../server
./configure 1
make


ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .
ln -s ../../OneLifeData7/dataVersionNumber.txt .


git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt


echo 0 > settings/requireTicketServerCheck.ini
echo 1 > settings/forceEveLocation.ini