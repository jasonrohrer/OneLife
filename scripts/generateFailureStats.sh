cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


sh makePrintFailureLogStatsHTML



while read user server port
do
  echo ""
  echo "Using rsync to sync all failure logs from $server"
  echo ""

  if [ ! -d ~/checkout/OneLife/server/failureLog_$server ]
  then
	  echo "Making local directory failureLog_$server"
	  mkdir ~/checkout/OneLife/server/failureLog_$server
  fi

  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/failureLog/*.txt ~/checkout/OneLife/server/failureLog_$server

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


/home/jcr15/checkout/OneLifeWorking/server/printFailureLogStatsHTML /home/jcr15/checkout/OneLife/server /home/jcr15/checkout/OneLife/server/objects /home/jcr15/public_html/failureStatsData.php
