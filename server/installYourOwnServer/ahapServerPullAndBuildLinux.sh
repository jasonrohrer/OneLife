#!/bin/bash


git clone https://github.com/jasonrohrer/minorGems.git	
git clone https://github.com/jasonrohrer/OneLife.git
git clone https://github.com/jasonrohrer/AnotherPlanetData.git	




cd minorGems
git fetch --tags
latestTaggedVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
git checkout -q OneLife_v$latestTaggedVersion


cd ../OneLife
git fetch --tags
latestTaggedVersionA=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`
git checkout -q OneLife_v$latestTaggedVersionA


cd ../AnotherPlanetData
git fetch --tags
latestTaggedVersionB=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/AnotherPlanet_v* | sed -e 's/AnotherPlanet_v//'`
git checkout -q AnotherPlanet_v$latestTaggedVersionB


cd ..

cd OneLife
git pull --tags
git checkout OneLife_liveServer
cd ..


cd OneLife/server
./configure 1
make
ln -s ../../AnotherPlanetData/objects .
ln -s ../../AnotherPlanetData/transitions .
ln -s ../../AnotherPlanetData/categories .
ln -s ../../AnotherPlanetData/tutorialMaps .
ln -s ../../AnotherPlanetData/dataVersionNumber.txt .
ln -s ../../AnotherPlanetData/contentSettings .

git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=2 refs/tags | grep "OneLife_v" | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt


serverVersion=`cat serverCodeVersionNumber.txt`

echo
echo
echo "Done building server with version v$serverVersion"


echo
echo "To run your server, do this:"
echo
echo "cd OneLife/server"
echo "./OneLifeServer"
echo


