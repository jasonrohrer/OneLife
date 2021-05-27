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

cppFileVer=$(grep versionNumber game.cpp | head -1 | sed -e 's/[^0-9]*//g' );


if [ $2 -ne $cppFileVer ] ; then
   echo "game.cpp version number mismatch (found '$cppFileVer', expecting $2)."
   exit 1
fi


make

cd ../build


./makeDistributionWindows v$2

cd windows

../../../minorGems/game/diffBundle/diffBundle OneLife_v$1 OneLife_v$2 $2_inc_win.dbz


echo
echo -n "Press ENTER to scp diff bundle to web server."

read userIn



scp $2_inc_win.dbz jcr15@onehouronelife.com:diffBundles/ 



echo
echo
echo "Done with diff bundle build."
echo "Next:  Steam depot build."


rm -r steamLatest
mkdir steamLatest
cp -r OneLife_v$2/* steamLatest/

cp ../steam/windows/steam_api.dll steamLatest/
cp ../steam/windows/steamGateClient.exe steamLatest/

cd steamLatest
rm -r animations categories ground groundTileCache music objects reverbCache sounds sprites transitions dataVersionNumber.txt

# Steam users cannot access files to report wild bugs easily
echo -n 0 > settings/reportWildBugToUser.ini

# don't use internal updater
echo -n 1 > settings/useSteamUpdate.ini

cd ..

rm steamLatest.tar.gz
tar czf steamLatest.tar.gz steamLatest


echo
echo -n "Press ENTER to scp steamLatest bundle to web server."

read userIn



scp steamLatest.tar.gz jcr15@onehouronelife.com: 


# new, run remotely

echo "Please ssh to server and run:"
echo "~/checkout/OneLifeWorking/scripts/runWindowsSteamDepotBuild.sh $2"
echo ""
echo -n "Press ENTER when done"

read userIn



# Old, run locally
# /c/steamSDK/tools/ContentBuilder/builder/steamcmd.exe +login "jasonrohrergames" +run_app_build -desc OneLifeClient_Windows_v$2 /c/cpp/OneLife/build/steam/app_build_windows_595690.vdf +quit

echo
echo "Steam depot build is done."



echo
echo
echo "Done."
echo "Don't forget to run  'git checkout master'   in OneLifeData7."




