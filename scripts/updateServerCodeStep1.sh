# Step 1 takes us all the way to the point where both odd and even 
# servers are in shutdown mode, with even servers all off and updated.


echo "" 
echo "Triggering EVEN remote server shutdowns."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )



echo "" 
echo "Triggering EVEN remote server code updates."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 0 ];
	then
		echo "  Starting update on $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )






echo "" 
echo "Triggering ODD remote server shutdowns."
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
i=1
while read user server port
do
	if [ $((i % 2)) -eq 1 ];
	then
		echo "  Shutting down $server"
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerShutdown.sh'
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/reflector/remoteServerList.ini )
