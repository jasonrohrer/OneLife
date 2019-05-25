#!/bin/sh

if [ $# -lt 2 ] ; then
   echo "Usage: $0  last_release_number  new_release_number"
   echo "Example: $0  37  39"
   echo 
   echo "NOTE:  The old release folder must exist in this directory diffing."
   echo 
   echo "NOTE:  Assumes old release built with v25 data."
   exit 1
fi


if [ ! -d "OneLife_v$1" ]
then
    echo "$0: Folder 'OneLife_v$1' not found."
	exit 1
fi


cd ../../gameSource

cppFileVer=$(grep versionNumber game.cpp | head -1 | sed -e 's/[^0-9]*//g' );


if [ $2 -ne $cppFileVer ] ; then
   echo "game.cpp version number mismatch (found '$cppFileVer', expecting $2)."
   exit 1
fi



cd ../../minorGems

cd game/diffBundle
./diffBundleCompile


cd ../../../OneLifeData7
git checkout OneLife_v25



cd ../OneLife/build/source

./makeLinuxBuild v$2


../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_linux.dbz


echo
echo -n "Press ENTER to scp diff bundle to web server."

read userIn



scp $2_inc_linux.dbz jcr15@onehouronelife.com:diffBundles/ 


cd ../../../OneLifeData7
git checkout master
