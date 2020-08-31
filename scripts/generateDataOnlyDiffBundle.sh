
wipeMaps=0

# two arguments means automation
if [ $# -ne 2 ]
then
	echo ""
	echo ""
	echo "Auto wipe maps as part of process?"
	echo ""
	echo "Enter WIPE to wipe, or press [ENTER] to skip."
	echo ""
	echo -n "Wipe maps: "
	read wipeMapsWord

	if [ "$wipeMapsWord" = "WIPE" ]
	then
		echo
		echo "Auto-wiping maps."
		echo
		wipeMaps=1
	else
		echo
		echo "NOT Auto-wiping maps."
		echo
	fi
fi




pauseToVerify=0

# two arguments means automation
if [ $# -ne 2 ]
then
	echo ""
	echo ""
	echo "Pause to verify Steam build along the way?"
	echo ""
	echo "Enter YES to pause, or press [ENTER] to skip."
	echo ""
	echo -n "Pause later: "
	read pauseWord

	if [ "$pauseWord" = "YES" ]
	then
		echo
		echo "Pausing later to verify Steam build."
		echo
		pauseToVerify=1
	else
		echo
		echo "NOT pausing later."
		echo
	fi
fi



# note that if we're running on cron-job automation, this might not work
# in genral, we have never done cron-job automation for midnight updates
# (because we are updating servers in batches) so it probably doesn't matter
echo "" 
echo "Logging in to steamcmd to make sure credentials are cached"
echo ""

steamcmd +login "jasonrohrergames" +quit



# two arguments means automation
if [ $# -ne 2 ]
then
	echo ""
	echo ""
	lastBuildID=`~/checkout/OneLifeWorking/scripts/getLatestSteamBuildID.sh`
	
	echo "Seeing last Steam build ID of $lastBuildID"
	echo ""
	echo "Check Steamworks and verify that this is correct."
	echo ""
	echo -n "Hit [ENTER] when ready: "
	read
fi



echo "" 
echo "Updating minorGems"
echo ""

cd ~/checkout/minorGems
git pull --tags



echo "" 
echo "Building diffBundle tool"
echo ""

cd game/diffBundle
./diffBundleCompile



echo "" 
echo "Updating OneLifeData7Latest"
echo ""

cd ~/checkout/OneLifeData7Latest
git pull --tags



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Data git version is:  $lastTaggedDataVersion"
echo ""

baseDataVersion=$lastTaggedDataVersion


if [ $# -eq 1 ]
then
	echo "" 
	echo "Overriding base data version using command line argument:  $1"
	echo "" 
	baseDataVersion=$1
fi




numNewChangsets=`git log OneLife_v$baseDataVersion..HEAD | grep commit | wc -l`


echo "" 
echo "Num git revisions since base data version:  $numNewChangsets"
echo ""



if [ $numNewChangsets -lt 1 ]
then
	echo "No new revisions to bundle, exiting."
	exit 1
fi



echo "" 
echo "Updating OneLifeWorking"
echo ""

cd ~/checkout/OneLifeWorking
git pull --tags


echo "" 
echo "Building headless cache generation tool"
echo ""

cd gameSource
sh ./makeRegenerateCaches

cd ~/checkout/OneLifeWorking



lastTaggedCodeVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/OneLife_v* | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent Code git version is:  $lastTaggedCodeVersion"
echo ""


lastTaggedVersion=$lastTaggedDataVersion

if [ $lastTaggedCodeVersion -gt $lastTaggedDataVersion ]
then
	lastTaggedVersion=$lastTaggedCodeVersion
fi


echo "" 
echo "Most recent Data or Code git version is:  $lastTaggedVersion"
echo ""



newVersion=$((lastTaggedVersion + 1))

echo "" 
echo "Using new version number:  $newVersion"
echo ""



# two arguments means automation
if [ $# -ne 2 ]
then
	echo ""
	echo ""
	echo "Most recent code version $lastTaggedCodeVersion"
	echo "Most recent data version $lastTaggedDataVersion"
	echo "Base data version for diff bundle $baseDataVersion"
	echo ""
	echo "About to post and tag data with $newVersion"
	echo ""
	echo -n "Hit [ENTER] when ready: "
	read
fi



cd ~/checkout/OneLifeData7Latest


# remove any old, stale directories
# this is what got us into trouble with the v152 update
# (the diffWorking/dataLatest directory created manually to test Steam
#  builds was in the way)
rm -rf ~/checkout/diffWorking



mkdir ~/checkout/diffWorking


echo "" 
echo "Exporting latest data for diffing"
echo ""


git clone . ~/checkout/diffWorking/dataLatest
rm -rf ~/checkout/diffWorking/dataLatest/.git*
rm ~/checkout/diffWorking/dataLatest/.hg*
rm -rf ~/checkout/diffWorking/dataLatest/soundsRaw
rm -rf ~/checkout/diffWorking/dataLatest/faces
rm -rf ~/checkout/diffWorking/dataLatest/scenes
rm -r ~/checkout/diffWorking/dataLatest/*.sh ~/checkout/diffWorking/dataLatest/working ~/checkout/diffWorking/dataLatest/overlays
echo -n "$newVersion" > ~/checkout/diffWorking/dataLatest/dataVersionNumber.txt


echo "" 
echo "Exporting base data for diffing"
echo ""

git checkout -q OneLife_v$baseDataVersion

git clone . ~/checkout/diffWorking/dataLast
rm -rf ~/checkout/diffWorking/dataLast/.git*
rm ~/checkout/diffWorking/dataLast/.hg*
rm -rf ~/checkout/diffWorking/dataLast/soundsRaw
rm -rf ~/checkout/diffWorking/dataLast/faces
rm -rf ~/checkout/diffWorking/dataLast/scenes
rm -r ~/checkout/diffWorking/dataLast/*.sh ~/checkout/diffWorking/dataLast/working ~/checkout/diffWorking/dataLast/overlays


# restore repository to latest, to avoid confusion later
git checkout -q master


echo "" 
echo "Copying last bundle's reverb cache to save work"
echo ""


cp -r ~/checkout/reverbCacheLastBundle ~/checkout/diffWorking/dataLast/reverbCache
cp -r ~/checkout/reverbCacheLastBundle ~/checkout/diffWorking/dataLatest/reverbCache



echo "" 
echo "Generating caches"
echo ""


cd ~/checkout/diffWorking/dataLast
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
# don't include bin_cache files in downloads, because they change
# with each update, and are too big
# cache.fcz files are full of compressed text files, so they're much smaller
# and fine to included when they change
rm */bin_*cache.fcz

cd ~/checkout/diffWorking/dataLatest
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
rm */bin_*cache.fcz


echo "" 
echo "Saving latest reverb cache for next time"
echo ""


rm -r ~/checkout/reverbCacheLastBundle
cp -r ~/checkout/diffWorking/dataLatest/reverbCache ~/checkout/reverbCacheLastBundle





echo "" 
echo "Generating diff bundle"
echo ""

cd ~/checkout/diffWorking

dbzFileName=${newVersion}_inc_all.dbz

~/checkout/minorGems/game/diffBundle/diffBundle dataLast dataLatest $dbzFileName

cp $dbzFileName ~/diffBundles/
mv $dbzFileName ~/www/updateBundles/


# start with an empty URL list
echo -n "" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline

# send this new .dbz to all the download servers
while read user server
do
  echo ""
  echo "Sending $dbzFileName to $server"
  scp ~/diffBundles/$dbzFileName $user@$server:downloads/

  echo "Adding url for $server to mirror list for this .dbz"

  echo "http://$server/downloads/$dbzFileName" >> ~/diffBundles/${newVersion}_inc_all_urls.txt

done <  <( grep "" ~/diffBundles/remoteServerList.ini )


# DO NOT add the main server to the mirror list
# farm this out to conserve bandwidth on the main server

# echo -n "http://onehouronelife.com/updateBundles/$dbzFileName" > ~/diffBundles/${newVersion}_inc_all_urls.txt


# don't post new version requirement to reflector just yet
# need to shutdown all servers first


# do NOT remove diffWorking dir just yet
# need it to build steam depot below
















echo "" 
echo "Updating live server data"
echo ""


cd ~/checkout/OneLifeData7
git pull
rm */cache.fcz
rm */bin_cache.fcz



newTag=OneLife_v$newVersion

echo "" 
echo "Updating live server data dataVersionNumber.txt to $newVersion"
echo ""

echo -n "$newVersion" > dataVersionNumber.txt

git add dataVersionNumber.txt
git commit -m "Updatated dataVersionNumber to $newVersion"


echo "" 
echo "Tagging live server data with new git tag $newTag"
echo ""

git tag -a $newTag -m "Tag automatically generated by data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push origin $newTag




echo "" 
echo "Tagging live server code with OneLife_liveServer"
echo ""

cd ~/checkout/OneLifeWorking


git tag -fa OneLife_liveServer -m "Tag automatically generated by data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push origin -f --tags



echo "" 
echo "Tagging live minorGems code with OneLife_liveServer"
echo ""

cd ~/checkout/minorGems


git tag -fa OneLife_liveServer -m "Tag automatically generated by data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push origin -f --tags




echo "" 
echo "Re-compiling non-running local server code base as a sanity check"
echo ""

cd ~/checkout/OneLife/server
git pull

./configure 1
make



echo "" 
echo "Posting to website Update log."
echo ""

~/checkout/OneLifeWorking/scripts/generateUpdatePosts.sh



echo "" 
echo "Generating object report for website."
echo ""

~/checkout/OneLifeWorking/scripts/generateObjectReport.sh






echo "" 
echo "Shutting down EVEN remote servers, setting server shutdownMode flags."
echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )







echo "" 
echo "Triggering EVEN remote server updates."
echo ""


# this will wait for even servers to finish shutting down
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




# Decision about when to build depot and make it live is tough
# This depot build process could take a bit of time.
# At this moment, Even servers are down, but ready to go
# and Odd servers are still running the old version and accepting new players
#
# There's no way to do this without the possibility of someone slipping through
# the cracks and connecting to a mis-matched-version server.
#
# Client should catch this and display an error message when it happens.
#
# So, all we can do is minimize the amount of time during which this can happen.
#
# Since the depot build can take a while, we do it here, while Odd servers are
# still running, so everyone can keep playing during the build.
#
# The shutdown operation of Odd servers should be quick, and the startup
# operation of Even servers should be quick as well.
#
# During that moment, some Steam users, who already have the new version,
# will see a SERVERS UPDATING message, because they will connect to outdated
# Odd servers that are still running.
#
# The worst thing that can ever happen to Non-Steam users is that they find
# all servers shutdown (in the moment between Odd shutdown and Even startup).
#


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
	echo "Steam build failed, trying remote build as backup"
	echo ""
	
	echo
	echo "Remote Steam build starting"
	echo

    # two arguments means automation
	if [ $# -ne 2 ]
	then
		# run ssh interactively so we can pause at error
		ssh build.onehouronelife.com 'cd ~/checkout/OneLifeWorking; git pull; ~/checkout/OneLifeWorking/scripts/generateSteamContentDepot.sh'
	else 
		# automation, do not run ssh interactively
		ssh -n build.onehouronelife.com 'cd ~/checkout/OneLifeWorking; git pull; ~/checkout/OneLifeWorking/scripts/generateSteamContentDepot.sh'
	fi
	echo
	echo "Remote Steam build done"
	echo
fi




echo "" 
echo "Keeping temporary diffWorking directory around for future reference"
echo ""

# don't delete this.  If something goes wrong, we'll want to look at it
#
# rm -r ~/checkout/diffWorking



if [ $pauseToVerify -eq 1 ]
then
	echo "" 
	echo "As requested, PAUSING now that Steam build is done."
	echo ""

	echo "Press [ENTER] when ready."
	echo ""
	echo -n "Ready? "
	read goWord
fi



echo "" 
echo "Shutting down ODD remote servers, setting server shutdownMode flags."
echo ""



# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )





echo "" 
echo "Setting required version number to $newVersion for all future connections."
echo ""



# now that servers are no longer accepting new connections, tell reflector
# and update server that an upgrade is available
echo -n "$newVersion" > ~/diffBundles/latest.txt

echo -n "<?php \$version=$newVersion; ?>" > ~/www/reflector/requiredVersion.php




echo "" 
echo "Triggering EVEN remote server startups."
echo ""


withMapWipeNote=""
serverStartupCommand="~/checkout/OneLife/scripts/remoteServerStartup.sh"

if [ $wipeMaps -eq 1 ]
then
	withMapWipeNote=" (with map wipe)"
	serverStartupCommand="~/checkout/OneLife/scripts/remoteServerWipeMap.sh"
fi



i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Triggering server startup$withMapWipeNote on $server"
		ssh -n $user@$server "$serverStartupCommand"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )





echo "" 
echo "Triggering ODD remote server updates."
echo ""


# this will wait for ODD servers to finish shutting down
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering ODD remote server startups."
echo ""

i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Triggering server startup$withMapWipeNote on $server"
		ssh -n $user@$server "$serverStartupCommand"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




echo "" 
echo "Entire update process is done."
echo ""






