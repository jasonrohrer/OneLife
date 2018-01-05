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


cd minorGems
git pull

cd ../OneLife
git pull



cd server
./makeStressTestClient
