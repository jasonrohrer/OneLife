#!/bin/bash

git clone ../../../minorGems/.git
git clone ../../../OneLife/.git
git clone ../../../OneLifeData7/.git

cd OneLife/server
./configure 1
make

ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .
ln -s ../../OneLifeData7/categories .
ln -s ../../OneLifeData7/tutorialMaps .
ln -s ../../OneLifeData7/dataVersionNumber.txt .

git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=2 refs/tags | grep "OneLife_v" | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt

serverVersion=`cat serverCodeVersionNumber.txt`

echo -e "\n\nDone building server with version v$serverVersion\n"
echo -e "To run your server, do this:\n"
echo "cd OneLife/server"
echo -e "./OneLifeServer\n"


