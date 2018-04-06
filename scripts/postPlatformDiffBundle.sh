

if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "postPlatformDiffBundle.sh  version_number [servers_too]"
	echo ""
	echo "servers_too is an optional parameter.  Set to 1 to interleave"
	echo "server updates with binary bundle updates (for protocol changes)."
	echo ""
	echo "Example:"
	echo "postPlatformDiffBundle.sh 18"
	echo ""
	echo "Example:"
	echo "postPlatformDiffBundle.sh 18 1"
	echo ""
	
	exit 1
fi



serversToo=0


if [ $# -gt 1 ]
then
	serversToo=1
fi

echo "ServersToo = $serversToo"

echo -n "Hit [ENTER] when ready: "
read


newVersion=$1



# first, check that existing tagged versions make sense

cd ~/checkout/OneLifeWorking
git pull --tags

lastOneLifeVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent OneLife version is:  $lastOneLifeVersion"
echo ""


cd ~/checkout/minorGems
git pull --tags

lastMGVersion=`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'`


echo "" 
echo "Most recent minorGems version is:  $lastMGVersion"
echo ""


if [ $lastMGVersion -ne $lastOneLifeVersion ]
then
	echo "Most recent OneLife and minorGems versions differ, exiting."
	exit 1
fi


if [ $lastOneLifeVersion -ge $newVersion ]
then
	echo "Requested version $newVersion not newer than most recent version."
	exit 1
fi


echo ""
echo "Most recent version $lastOneLifeVersion"
echo "About to post and tag $newVersion"
echo ""
echo -n "Hit [ENTER] when ready: "
read







# next, check that incremental bundles exist for all platforms
# don't proceed unless we can proceed for all
for platform in linux mac win; do
	dbzFileName=${newVersion}_inc_${platform}.dbz

	dbzFilePath=~/diffBundles/$dbzFileName

	if [ ! -e $dbzFilePath ]
	then
		echo "File doesn't exist at $dbzFilePath"
		exit 1
	fi
	urlFilePath=~/diffBundles/${newVersion}_inc_${platform}_urls.txt

	if [ -e $urlFilePath ]
	then
		echo "URL file already exists $urlFilePath"
		exit 1
	fi
done 





for platform in linux mac win; do


	dbzFileName=${newVersion}_inc_${platform}.dbz
	echo "New bundle name:  $dbzFileName"

	dbzFilePath=~/diffBundles/$dbzFileName


	urlFilePath=~/diffBundles/${newVersion}_inc_${platform}_urls.txt



	echo -n "" > $urlFilePath


    # feed file through grep to add newlines at the end of each line
    # otherwise, read skips the last line if it doesn't end with newline

    # send this new .dbz to all the download servers
	while read user server
	do
		echo ""
		echo "Sending $dbzFileName to $server"
		scp $dbzFilePath $user@$server:downloads/
		
		echo "Adding url for $server to mirror list for this .dbz"
		
		echo "http://$server/downloads/$dbzFileName" >> $urlFilePath
		
	done <  <( grep "" ~/diffBundles/remoteServerList.ini )
	
done




echo ""
echo "Using rsync to push non-diff binary bundles too."


~/checkout/OneLife/scripts/pushDownloadsAndDiffsToMirrors.sh


echo ""
echo "Tagging OneLife and minorGems with OneLife_v$newVersion"

cd ~/checkout/OneLifeWorking
git pull

git tag -a -m "Tagging for binary version $newVersion" OneLife_v$newVersion

git push --tags


cd ~/checkout/minorGems
git pull


git tag -a -m "Tagging for OneLife version $newVersion" OneLife_v$newVersion

git push --tags



if [ $serversToo -gt 0 ]
then
	~/checkout/OneLifeWorking/scripts/updateServerCodeStep1.sh
fi



echo ""
echo "Telling update server and reflector about latest version."

echo -n "$newVersion" > ~/diffBundles/latest.txt

echo -n "<?php \$version=$newVersion; ?>" > ~/www/reflector/requiredVersion.php



if [ $serversToo -gt 0 ]
then
	~/checkout/OneLifeWorking/scripts/updateServerCodeStep2.sh
fi
