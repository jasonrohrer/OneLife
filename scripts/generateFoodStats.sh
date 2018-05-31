cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


sh makePrintFoodLogStatsHTML



while read user server port
do
  echo ""
  echo "Using rsync to sync all food logs from $server"
  echo ""

  if [ ! -d ~/checkout/OneLife/server/foodLog_$server ]
  then
	  echo "Making local directory foodLog_$server"
	  mkdir ~/checkout/OneLife/server/foodLog_$server
  fi

  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/foodLog/*.txt ~/checkout/OneLife/server/foodLog_$server

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


/home/jcr15/checkout/OneLifeWorking/server/printFoodLogStatsHTML /home/jcr15/checkout/OneLife/server /home/jcr15/checkout/OneLife/server/objects /home/jcr15/public_html/foodStatsData.php
