#!/bin/sh

if [ $# -lt 2 ] ; then
   echo "Usage: $0  last_release_name  new_release_name"
   echo "Example: $0  v37  v39"
   echo 
   echo "NOTE:  The old release folder must exist in 'mac' for diffing."
   exit 1
fi

if [ ! -f "mac/OneLife_$1" ]
then
    echo "$0: Folder 'mac/OneLife_$1' not found."
	exit 1
fi



cd ../../minorGems
git pull

cd game/diffBundle
./diffBundleCompile



cd ../../../OneLife
git pull


./configure 2
cd gameSource
make

cd ../build


./makeDistributionMacOSX $2 IntelMacOSX /System/Library/Frameworks/SDL.framework

cd mac

../../../minorGems/game/diffBundle/diffBundle OneLife_$1 OneLife_$2 $2_inc_mac.dbz


scp $2_inc_mac.dbz jcr15@onehouronelife.com:diffBundles/ 




