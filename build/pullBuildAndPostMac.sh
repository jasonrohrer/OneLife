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
make

cd ../build


./makeDistributionMacOSX v$2 IntelMacOSX /System/Library/Frameworks/SDL.framework

cd mac

../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_mac.dbz


scp $2_inc_mac.dbz jcr15@onehouronelife.com:diffBundles/ 



cd ../../../OneLifeData7
git checkout master

