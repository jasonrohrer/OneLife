while read user server port
do
	host $server | sed "s/.*has address //"
done <  <( grep "" ~/www/reflector/remoteServerList.ini )

