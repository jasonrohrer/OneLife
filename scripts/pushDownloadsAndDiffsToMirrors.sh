while read user server
do
  echo ""
  echo "Using rsync to sync all .dbz files to $server"
  echo ""

  rsync -avz -e ssh --progress ~/diffBundles/*.dbz $user@$server:downloads/

  echo ""
  echo "Using rsync to sync all oneLifeDownloads to $server"
  echo ""

  rsync -avz --exclude='*.ini' --exclude="*~" -e ssh --progress ~/oneLifeDownloads/* $user@$server:downloads/

done <  <( grep "" ~/diffBundles/remoteServerList.ini )