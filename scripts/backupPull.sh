# run as a cron job on backup server to pull from listed servers


while read user server port
do

  echo ""
  echo "Using rsync to sync all backups from $server"
  echo ""

  if [ ! -f ~/backups/$server ]
  then
    mkdir ~/backups/$server
  fi

  rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress $user@$server:checkout/OneLife/server/backups/* ~/backups/$server

  
  echo ""
  echo "Deleting local files that are more than 14 days old"
  echo ""

  # delete backup files older than two weeks
  find ~/backups/$server -mtime +14 -delete


done <  <( grep "" ~/remoteServerList.ini )