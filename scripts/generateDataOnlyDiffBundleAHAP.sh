
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
	echo "Pause to verify build along the way?"
	echo ""
	echo "Enter YES to pause, or press [ENTER] to skip."
	echo ""
	echo -n "Pause later: "
	read pauseWord

	if [ "$pauseWord" = "YES" ]
	then
		echo
		echo "Pausing later to verify build."
		echo
		pauseToVerify=1
	else
		echo
		echo "NOT pausing later."
		echo
	fi
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
echo "Updating AnotherPlanetDataLatest"
echo ""

cd ~/checkout/AnotherPlanetDataLatest
git pull --tags



lastTaggedDataVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/AnotherPlanet_v* | sed -e 's/AnotherPlanet_v//'`


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




numNewChangsets=`git log AnotherPlanet_v$baseDataVersion..HEAD | grep commit | wc -l`


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




echo "" 
echo "Ignoring OHOL latest tagged code version for AHAP data build."
echo ""


lastTaggedVersion=$lastTaggedDataVersion



echo "" 
echo "Most recent AHAP version is:  $lastTaggedVersion"
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
	echo "Most recent data version $lastTaggedDataVersion"
	echo "Base data version for diff bundle $baseDataVersion"
	echo ""
	echo "About to post and tag data with $newVersion"
	echo ""
	echo -n "Hit [ENTER] when ready: "
	read
fi



cd ~/checkout/AnotherPlanetDataLatest


# remove any old, stale directories
# this is what got us into trouble with the v152 update
# (the ahapDiffWorking/dataLatest directory created manually to test Steam
#  builds was in the way)
rm -rf ~/checkout/ahapDiffWorking



mkdir ~/checkout/ahapDiffWorking


echo "" 
echo "Exporting latest data for diffing"
echo ""


git clone . ~/checkout/ahapDiffWorking/dataLatest
rm -rf ~/checkout/ahapDiffWorking/dataLatest/.git*
rm ~/checkout/ahapDiffWorking/dataLatest/.hg*
rm -rf ~/checkout/ahapDiffWorking/dataLatest/soundsRaw
rm -rf ~/checkout/ahapDiffWorking/dataLatest/faces
rm -rf ~/checkout/ahapDiffWorking/dataLatest/scenes
rm -r ~/checkout/ahapDiffWorking/dataLatest/*.sh ~/checkout/ahapDiffWorking/dataLatest/working ~/checkout/ahapDiffWorking/dataLatest/overlays
echo -n "$newVersion" > ~/checkout/ahapDiffWorking/dataLatest/dataVersionNumber.txt


echo "" 
echo "Exporting base data for diffing"
echo ""

git checkout -q AnotherPlanet_v$baseDataVersion

git clone . ~/checkout/ahapDiffWorking/dataLast
rm -rf ~/checkout/ahapDiffWorking/dataLast/.git*
rm ~/checkout/ahapDiffWorking/dataLast/.hg*
rm -rf ~/checkout/ahapDiffWorking/dataLast/soundsRaw
rm -rf ~/checkout/ahapDiffWorking/dataLast/faces
rm -rf ~/checkout/ahapDiffWorking/dataLast/scenes
rm -r ~/checkout/ahapDiffWorking/dataLast/*.sh ~/checkout/ahapDiffWorking/dataLast/working ~/checkout/ahapDiffWorking/dataLast/overlays


# restore repository to latest, to avoid confusion later
git checkout -q master


echo "" 
echo "Copying last bundle's reverb cache to save work"
echo ""


cp -r ~/checkout/reverbCacheLastBundleAHAP ~/checkout/ahapDiffWorking/dataLast/reverbCache
cp -r ~/checkout/reverbCacheLastBundleAHAP ~/checkout/ahapDiffWorking/dataLatest/reverbCache



echo "" 
echo "Generating caches"
echo ""


cd ~/checkout/ahapDiffWorking/dataLast
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
# don't include bin_cache files in downloads, because they change
# with each update, and are too big
# cache.fcz files are full of compressed text files, so they're much smaller
# and fine to included when they change
rm */bin_*cache.fcz

cd ~/checkout/ahapDiffWorking/dataLatest
cp ~/checkout/OneLifeWorking/gameSource/reverbImpulseResponse.aiff .
~/checkout/OneLifeWorking/gameSource/regenerateCaches
rm reverbImpulseResponse.aiff
rm */bin_*cache.fcz


echo "" 
echo "Saving latest reverb cache for next time"
echo ""


rm -r ~/checkout/reverbCacheLastBundleAHAP
cp -r ~/checkout/ahapDiffWorking/dataLatest/reverbCache ~/checkout/reverbCacheLastBundleAHAP





echo "" 
echo "Generating diff bundle"
echo ""

cd ~/checkout/ahapDiffWorking

dbzFileName=${newVersion}_inc_all.dbz

~/checkout/minorGems/game/diffBundle/diffBundle dataLast dataLatest $dbzFileName

cp $dbzFileName ~/ahapDiffBundles/
mv $dbzFileName ~/www/ahapUpdateBundles/


# start with an empty URL list
echo -n "" > ~/ahapDiffBundles/${newVersion}_inc_all_urls.txt


# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline

# send this new .dbz to all the download servers
while read user server
do
  echo ""
  echo "Sending $dbzFileName to $server"
  scp ~/ahapDiffBundles/$dbzFileName $user@$server:ahapDownloads/

  echo "Adding url for $server to mirror list for this .dbz"

  echo "http://$server/ahapDownloads/$dbzFileName" >> ~/ahapDiffBundles/${newVersion}_inc_all_urls.txt

done <  <( grep "" ~/ahapDiffBundles/remoteServerList.ini )


# DO NOT add the main server to the mirror list
# farm this out to conserve bandwidth on the main server

# echo -n "http://onehouronelife.com/ahapUpdateBundles/$dbzFileName" > ~/ahapDiffBundles/${newVersion}_inc_all_urls.txt


# don't post new version requirement to reflector just yet
# need to shutdown all servers first


#
# do NOT remove ahapDiffWorking dir just yet
















echo "" 
echo "Updating live server data"
echo ""


cd ~/checkout/AnotherPlanetData
git pull
rm */cache.fcz
rm */bin_cache.fcz



newTag=AnotherPlanet_v$newVersion

echo "" 
echo "Updating live server data dataVersionNumber.txt to $newVersion"
echo ""

echo -n "$newVersion" > dataVersionNumber.txt

git add dataVersionNumber.txt
git commit -m "Updatated dataVersionNumber to $newVersion"


echo "" 
echo "Tagging live server data with new git tag $newTag"
echo ""

git tag -a $newTag -m "Tag automatically generated by AHAP data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push origin $newTag




echo "" 
echo "Tagging OHOL live server code with AnotherPlanet_liveServer"
echo ""

cd ~/checkout/OneLifeWorking


git tag -fa AnotherPlanet_liveServer -m "Tag automatically generated by AHAP data-only diffBundle script."


echo "" 
echo "Pushing tag change to central git server"
echo ""

git push
git push origin -f --tags



echo "" 
echo "Tagging live minorGems code with AnotherPlanet_liveServer"
echo ""

cd ~/checkout/minorGems


git tag -fa AnotherPlanet_liveServer -m "Tag automatically generated by AHAP data-only diffBundle script."


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

~/checkout/OneLifeWorking/scripts/generateUpdatePostsAHAP.sh



echo "" 
echo "Generating AHAP object report for website."
echo ""

~/checkout/OneLifeWorking/scripts/generateObjectReportAHAP.sh






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
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )







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
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdateAHAP.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )



echo "" 
echo "No Steam Content depot build for AHAP."
echo ""




echo "" 
echo "Keeping temporary ahapDiffWorking directory around for future reference"
echo ""

# don't delete this.  If something goes wrong, we'll want to look at it
#
# rm -r ~/checkout/ahapDiffWorking



if [ $pauseToVerify -eq 1 ]
then
	echo "" 
	echo "PAUSING now."
	echo ""
	echo "Even servers are shut down, about to push update to Odd servers."
	echo ""
	echo "This is a good time for any manual sanity checks."
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
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )





echo "" 
echo "Setting required version number to $newVersion for all future connections."
echo ""



# now that servers are no longer accepting new connections,
# update server that an upgrade is available
echo -n "$newVersion" > ~/ahapDiffBundles/latest.txt

# also updated required ahapVersion.txt file
echo -n "$newVersion http://onehouronelife.com/ahapUpdateServer/server.php OK" > ~/www/ahapVersion.txt

# note that we do NOT update reflector required version number, not even
# for the AHAP-specific reflector, since that version number tracks required
# code version only.  AHAP-specific clients check ahapVersion.txt when
# they start up to download any new content.




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
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )





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
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerUpdateAHAP.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )



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
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )




echo "" 
echo "Entire update process is done."
echo ""






