#!/bin/sh

if [ $# -lt 2 ] ; then
   echo "Usage: $0  last_release_number  new_release_number"
   echo "Example: $0  37  39"
   echo 
   echo "NOTE:  The old release folder must exist in 'mac' for diffing."
   exit 1
fi

if [ ! -d "mac/OneLife_v$1" ]
then
    echo "$0: Folder 'mac/OneLife_v$1' not found."
	exit 1
fi



cd ../../minorGems
git pull

cd game/diffBundle
./diffBundleCompile


cd ../../../OneLifeData7
git checkout OneLife_v25


cd ../OneLife
git pull


./configure 2
cd gameSource


cppFileVer=$(grep versionNumber game.cpp | head -1 | sed -e 's/[^0-9]*//g' );


if [ $2 -ne $cppFileVer ] ; then
   echo "game.cpp version number mismatch (found '$cppFileVer', expecting $2)."
   exit 1
fi



make

cd ../build


./makeDistributionMacOSX v$2 IntelMacOSX /System/Library/Frameworks/SDL.framework

cd mac

../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_mac.dbz


echo
echo -n "Press ENTER to scp diff bundle to web server."

read userIn


scp $2_inc_mac.dbz jcr15@onehouronelife.com:diffBundles/ 



cd ../../../OneLifeData7
git checkout master

