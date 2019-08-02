#!/bin/sh

apt-get -o Acquire::ForceIPv4=true update
apt-get -y install emacs-nox mercurial git g++ expect gdb make valgrind


mkdir checkout

cd checkout



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
git pull

cd ../OneLife
git pull

cd ../OneLifeData7
git pull

rm */cache.fcz
rm */bin_*cache.fcz


cd ../OneLife



cd server
./configure 1
make


ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .
ln -s ../../OneLifeData7/tutorialMaps .


echo 0 > settings/requireTicketServerCheck.ini
