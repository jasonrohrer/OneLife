# Step 2 brings even servers back online, and then applies update to odd
# servers (which waits for them to shutdown), and finally brings
# odd servers back up.

wipeMaps = 0

if [ $# -eq 1 ]
then
	wipeMaps=$1
fi

withMapWipeNote=""
serverStartupCommand="~/checkout/OneLife/scripts/remoteServerStartup.sh"

if [ $wipeMaps -eq 1 ]
then
	withMapWipeNote=" (with map wipe)"
	serverStartupCommand="~/checkout/OneLife/scripts/remoteServerWipeMap.sh"
fi



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
		echo "  Triggering server startup$withMapWipeNote on $server"
		ssh -n $user@$server "$serverStartupCommand"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )




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
		ssh -n $user@$server '~/checkout/OneLife/scripts/serverPull.sh'
		ssh -n $user@$server '~/checkout/OneLife/scripts/remoteServerCodeUpdate.sh'
	fi
	i=$((i + 1))  
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )



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
		echo "  Triggering server startup$withMapWipeNote on $server"
		ssh -n $user@$server "$serverStartupCommand"
	fi
	i=$((i + 1))
done <  <( grep "" ~/www/ahapReflector/remoteServerList.ini )
