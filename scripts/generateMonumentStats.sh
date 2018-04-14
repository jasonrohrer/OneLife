cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


sh makePrintLifeLogStatsHTML



while read user server port
do
  echo ""
  echo "Using rsync to sync all monument logs from $server"
  echo ""

  dirName="~/checkout/OneLife/server/monumentLogs_${server}/"
  mkdir -p  $dirName
  
  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/monumentLogs/*.txt $dirName

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


cd ~/checkout/OneLife/server/

numDone=`ls -l monumentLogs*/ | grep -c done`

monumentWord="monuments"

if [ "$numDone" -eq "1" ]; then
	monumentWord="monument";
fi


echo "$numDone $monumentWord" > /home/jcr15/public_html/monumentStats.php
