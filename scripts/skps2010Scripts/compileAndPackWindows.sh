#!/bin/sh

if [ $# -lt 1 ] ; then
   echo "Usage: $0 new_release_number"
   echo "Example: $0 39"
   exit 1
fi

echo
echo "Make sure to git pull all the components"
#echo "Also pull OneLifeData7, but then run:   git checkout OneLife_v20"
echo
echo -n "Press ENTER when done."

read userIn


cd ../../minorGems

cd game/diffBundle
#./diffBundleCompile


cd ../../../OneLife

./configure 5
cd gameSource
make

cd ../build

./makeDistributionWindows v$1

cd windows

#../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_win.dbz


#scp $2_inc_win.dbz jcr15@onehouronelife.com:diffBundles/ 

echo
echo "Done."
#echo "Don't forget to run  'git checkout master'   in OneLifeData7."




