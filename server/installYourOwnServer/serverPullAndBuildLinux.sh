#!/bin/bash

git clone https://github.com/twohoursonelife/minorGems.git
git clone https://github.com/twohoursonelife/OneLife.git
git clone https://github.com/twohoursonelife/OneLifeData7.git

cd minorGems
git fetch --tags
latestTaggedVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/2HOL_v* | sed -e 's/2HOL_v//'`
git checkout -q 2HOL_v$latestTaggedVersion

cd ../OneLife
git fetch --tags
latestTaggedVersionA=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/2HOL_v* | sed -e 's/2HOL_v//'`
git checkout -q 2HOL_v$latestTaggedVersionA

cd ../OneLifeData7
git fetch --tags
latestTaggedVersionB=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/2HOL_v* | sed -e 's/2HOL_v//'`
git checkout -q 2HOL_v$latestTaggedVersionB


cd ..

cd OneLife
git pull --tags
git checkout 2HOL_liveServer
cd ..


cd OneLife/server
./configure 1
make
ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .
ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/tutorialMaps .
ln -s ../../OneLifeData7/dataVersionNumber.txt .

git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=2 refs/tags | grep "2HOL_v" | sed -e 's/2HOL_v//' > serverCodeVersionNumber.txt


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
