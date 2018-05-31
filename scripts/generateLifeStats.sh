cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


sh makePrintLifeLogStatsHTML



while read user server port
do
  echo ""
  echo "Using rsync to sync all life logs from $server"
  echo ""

  if [ ! -d ~/checkout/OneLife/server/lifeLog_$server ]
  then
	  echo "Making local directory lifeLog_$server"
	  mkdir ~/checkout/OneLife/server/lifeLog_$server
  fi

  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/lifeLog/*.txt ~/checkout/OneLife/server/lifeLog_$server

done <  <( grep "" ~/www/reflector/remoteServerList.ini )




/home/jcr15/checkout/OneLifeWorking/server/printLifeLogStatsHTML /home/jcr15/checkout/OneLife/server /home/jcr15/public_html/lifeStats.php
