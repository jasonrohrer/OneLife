#!/bin/sh

cd /c/cpp/minorGems
git pull --tags

cd /c/cpp/OneLife
git pull --tags


cd /c/cpp/OneLifeData7
git checkout OneLife_v20



echo ""
echo ""
echo "After building, will automatically check out master in Data7."
echo ""
echo -n "Hit [ENTER] when ready: "
read


git checkout master