
echo "" 
echo "Triggering ODD remote server FORCED shutdowns."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Force Shutting down $server"
		ssh -n $user@$server 'echo 1 > ~/checkout/OneLife/server/settings/forceShutdownMode.ini; sleep 3; echo -n 0 > ~/checkout/OneLife/server/settings/forceShutdownMode.ini'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )

