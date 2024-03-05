while read user server
do
  echo ""
  echo "Using rsync to sync all OHOL .dbz files to $server"
  echo ""

  rsync -avz -e ssh --progress ~/diffBundles/*.dbz $user@$server:downloads/

  echo ""
  echo "Using rsync to sync all AHAP .dbz files to $server"
  echo ""

  rsync -avz -e ssh --progress ~/ahapDiffBundles/*.dbz $user@$server:ahapDownloads/


  echo ""
  echo "Using rsync to sync all oneLifeDownloads to $server"
  echo ""

  rsync -avz --exclude='*.ini' --exclude="*~" -e ssh --progress ~/oneLifeDownloads/* $user@$server:downloads/


  echo ""
  echo "Using rsync to sync all AHAP Downloads to $server"
  echo ""

  rsync -avz --exclude='*.ini' --exclude="*~" -e ssh --progress ~/ahapDownloads/* $user@$server:ahapDownloads/

done <  <( grep "" ~/diffBundles/remoteServerList.ini )