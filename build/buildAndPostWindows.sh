#!/bin/sh

if [ $# -lt 2 ] ; then
   echo "Usage: $0  last_release_number  new_release_number"
   echo "Example: $0  37  39"
   echo 
   echo "NOTE:  The old release folder must exist in 'windows' for diffing."
   exit 1
fi

if [ ! -d "windows/OneLife_v$1" ]
then
    echo "$0: Folder 'windows/OneLife_v$1' not found."
	exit 1
fi

echo
echo "Make sure to git pull both minorGems and OneLife"
echo "Also pull OneLifeData7, but then run:   git checkout OneLife_v20"
echo
echo -n "Press ENTER when done."

read userIn


cd ../../minorGems

cd game/diffBundle
./diffBundleCompile



cd ../../../OneLife


./configure 3
cd gameSource
make

cd ../build


./makeDistributionWindows v$2

cd windows

../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_win.dbz


scp $2_inc_win.dbz jcr15@onehouronelife.com:diffBundles/ 


echo
echo
echo "Done."
echo "Don't forget to run  'git checkout master'   in OneLifeData7."




