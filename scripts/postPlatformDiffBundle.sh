

if [ $# -ne 2 ]
then
	echo "Usage:"
	echo "postPlatformDiffBundle.sh  version_number platform_name"
	echo ""
	echo "Examples:"
	echo "postPlatformDiffBundle.sh 18 linux"
	echo "postPlatformDiffBundle.sh 17 mac"
	echo "postPlatformDiffBundle.sh 14 win"
	echo ""
	
	exit 1
fi


newVersion=$1
platform=$2



dbzFileName=${newVersion}_inc_${platform}.dbz


echo "New bundle name:  $dbzFileName"

echo ""


dbzFilePath=~/diffBundles/$dbzFileName

if [ ! -f $dbzFilePath ]
then
    echo "File doesn't exist at $dbzFilePath"
	exit 1
fi

urlFilePath=~/diffBundles/${newVersion}_inc_${platform}_urls.txt


if [ -f $urlFilePath ]
then
    echo "URL file already exists $urlFilePath"
	exit 1
fi



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

  echo "http://$server/downloads/$dbzFileName" > $urlFilePath

done <  <( grep "" ~/diffBundles/remoteServerList.ini )
