# Step 2 brings even servers back online, and then applies update to odd
# servers (which waits for them to shutdown), and finally brings
# odd servers back up.


echo "" 
echo "Triggering EVEN remote server restarts."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Triggering server startup on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerStartup.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )




echo "" 
echo "Triggering ODD remote server code updates."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
	fi
	i=$((i + 1))  
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering ODD remote server restarts."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Triggering server startup on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerStartup.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )
