# script for generating Steam content depot separately
# from normal generateDataOnlyDiffBundle script

# this runs the whole process from scratch

# still assumes that there is a jcr15 home directory


# NOTE:
# Also assumes that OneLifeData7 has been updated and pushed
# with latest data and data version number
#
# Essentially, we will only run this script if generateDataOnlyDiffBundle
# ran fully but failed at Steam depot step



echo "" 
echo "Logging in to steamcmd to make sure credentials are cached"
echo ""

steamcmd +login "jasonrohrergames" +quit



echo "" 
echo "Updating minorGems"
echo ""

cd ~/checkout/minorGems
~/checkout/OneLifeWorking/scripts/gitPullComplete.sh



echo "" 
echo "Updating OneLifeData7Latest"
echo ""

cd ~/checkout/OneLifeData7Latest
~/checkout/OneLifeWorking/scripts/gitPullComplete.sh



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`




echo "" 
echo "Updating OneLifeWorking"
echo ""

cd ~/checkout/OneLifeWorking
~/checkout/OneLifeWorking/scripts/gitPullComplete.sh



echo "" 
echo "Building headless cache generation tool"
echo ""

cd gameSource
sh ./makeRegenerateCaches

cd ~/checkout/OneLifeWorking



newVersion=$lastTaggedDataVersion

echo "" 
echo "Using new version number:  $newVersion"
echo ""



echo ""
echo ""
echo "Most recent data version $lastTaggedDataVersion"
echo ""
echo "Posting Steam content depot with $newVersion"
echo ""




cd ~/checkout/OneLifeData7Latest


# remove any old, stale directories
# this is what got us into trouble with the v152 update
# (the diffWorking/dataLatest directory created manually to test Steam
#  builds was in the way)
rm -rf ~/checkout/diffWorking



mkdir ~/checkout/diffWorking


echo "" 
echo "Exporting latest data for bundling"
echo ""


git clone . ~/checkout/diffWorking/dataLatest
rm -rf ~/checkout/diffWorking/dataLatest/.git*
rm ~/checkout/diffWorking/dataLatest/.hg*
rm -rf ~/checkout/diffWorking/dataLatest/soundsRaw
rm -rf ~/checkout/diffWorking/dataLatest/faces
rm -rf ~/checkout/diffWorking/dataLatest/scenes
rm -r ~/checkout/diffWorking/dataLatest/*.sh ~/checkout/diffWorking/dataLatest/working ~/checkout/diffWorking/dataLatest/overlays



echo "" 
echo "Generating caches"
echo ""


cd ~/checkout/diffWorking/dataLatest
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
rm */bin_*cache.fcz



echo "" 
echo "Building Steam Depot and making it public/live"
echo ""


oldBuildID=`~/checkout/OneLifeWorking/scripts/getLatestSteamBuildID.sh`


steamcmd +login "jasonrohrergames" +run_app_build -desc OneLifeContent_v$newVersion ~/checkout/OneLifeWorking/build/steam/app_build_content_595690.vdf +quit | tee /tmp/steamBuildLog.txt


newBuildID=`grep BuildID /tmp/steamBuildLog.txt | sed "s/.*(BuildID //" | sed "s/).*//"`


echo ""
echo "Old Steam build ID:  $oldBuildID"
echo "New Steam build ID:  $newBuildID"
echo ""



if [[ $newBuildID = "" || $newBuildID -le $oldBuildID ]]
then
	echo ""
	echo ""
	echo "Steam buld failed!"
	echo ""
	echo "You must build and post depot manually using another method."
	echo ""
	echo -n "Hit [ENTER] when ready: "
	read
fi


echo "" 
echo "Deleting temporary diffWorking directory"
echo ""

rm -r ~/checkout/diffWorking
