cd /home/jcr15
cd checkout/OneLifeWorking/server

git pull


./makePrintMonumentHTML




while read user server port
do
  echo ""
  echo "Using rsync to sync all monument logs from $server"
  echo ""

  dirName=~/checkout/OneLife/server/monumentLogs_$server
  mkdir -p  $dirName
  
  rsync -avz -e ssh --progress $user@$server:checkout/OneLife/server/monumentLogs/*.txt $dirName

done <  <( grep "" ~/www/reflector/remoteServerList.ini )



./printMonumentHTML ~/checkout/OneLife/server/ ~/www/monuments/



cd ~/checkout/OneLife/server/

numDone=`ls -l monumentLogs*/ | grep -c done`

numInProgress=`ls -l monumentLogs*/ | grep -c -v done`


monumentWord="monuments"

if [ "$numDone" -eq "1" ]; then
	monumentWord="monument";
fi


echo "<a href=monuments>$numDone $monumentWord completed</a>, $numInProgress in progress" > /home/jcr15/public_html/monumentStats.php
