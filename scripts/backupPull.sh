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


user=jcr13
server=lineage.onehouronelife.com


echo ""
echo "Using rsync to sync all backups from $server"
echo ""


if [ ! -f ~/backups/$server ]
then
    mkdir ~/backups/$server
fi

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress $user@$server:backups/* ~/backups/$server


echo ""
echo "Deleting local files that are more than 14 days old"
echo ""

# delete backup files older than two weeks
find ~/backups/$server -mtime +14 -delete






user=jcr13
server=projectdecember.onehouronelife.com


echo ""
echo "Using rsync to sync all backups from $server"
echo ""


if [ ! -f ~/backups/$server ]
then
    mkdir ~/backups/$server
fi

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress $user@$server:backups/* ~/backups/$server


echo ""
echo "Deleting local files that are more than 14 days old"
echo ""

# delete backup files older than two weeks
find ~/backups/$server -mtime +14 -delete




user=jcr13
server=thecastledoctrine.onehouronelife.com


echo ""
echo "Using rsync to sync all backups from $server"
echo ""


if [ ! -f ~/backups/$server ]
then
    mkdir ~/backups/$server
fi

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress $user@$server:backups/* ~/backups/$server


echo ""
echo "Deleting local files that are more than 14 days old"
echo ""

# delete backup files older than two weeks
find ~/backups/$server -mtime +14 -delete




user=jcr13
server=cordialminuet.onehouronelife.com


echo ""
echo "Using rsync to sync all backups from $server"
echo ""


if [ ! -f ~/backups/$server ]
then
    mkdir ~/backups/$server
fi

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress $user@$server:backups/* ~/backups/$server


echo ""
echo "Deleting local files that are more than 14 days old"
echo ""

# delete backup files older than two weeks
find ~/backups/$server -mtime +14 -delete

 




if [ ! -f ~/backups/northcountrynotes ]
then
    mkdir ~/backups/northcountrynotes
fi

rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress northcountrynotes.org:backups/* ~/backups/northcountrynotes/







# delete backup files in main older than two weeks
find ~/backups/main -mtime +14 -delete
