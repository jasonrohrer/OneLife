# Script run on main server to copy before.rules for ufw from git repo
# to all remote game servers, using root access
\


echo "About to update UFW before.rules on all remote servers"
echo ""
echo -n "Hit [ENTER] when ready: "
read

cd ~/checkout/OneLifeWorking
git pull


# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do
  echo "  Updating before.rules on $server"
  scp scripts/before.rules root@$server:

  ssh -n root@$server 'cat before.rules > /etc/ufw/before.rules; rm before.rules; ufw reload'
done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done running UFW before.rules update on all servers."
echo ""
