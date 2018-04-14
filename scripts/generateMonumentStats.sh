cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


sh makePrintLifeLogStatsHTML



while read user server port
do
  echo ""
  echo "Using rsync to sync all monument logs from $server"
  echo ""

  if [ ! -d "~/checkout/OneLife/server/monumentLogs_$server" ]
  then
	  echo "Making local directory monumentLogs_$server"
	  mkdir ~/checkout/OneLife/server/monumentLog_$server
  fi

  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/monumentLogs/*.txt ~/checkout/OneLife/server/monumentLogs_$server

done <  <( grep "" ~/www/reflector/remoteServerList.ini )



$numDone=`ls -l monumentLogs*/*done* | grep -c done`

$monumentWord="monuments"

if [ "$numDone" -eq "1" ]; then
	$monumentWord="monument";
fi


echo "$numDone $monumentWord" > /home/jcr15/public_html/monumentStats.php
