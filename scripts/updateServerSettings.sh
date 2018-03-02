# Script run on main server to copy several live settings to all
# remote servers.

# Copies latest version from git repository.



echo "NOTE:"
echo "This script is only for replicating server settings that have NOT yet"
echo "been commited in git."
echo "Most of the time, you DO NOT want to do this, because those customized"
echo "settings files will block git pull operations in the future."
echo ""
echo "Usually, you'll just want to run ./remoteServerGitPull.sh instead"
echo "to get all remote servers updated to the settings files in git."
echo ""
echo "If you really want to do this..."
echo -n "Hit [ENTER] when ready: "
read



git pull




cd ~/checkout/OneLifeWorking/server/settings



settingsList="babyBirthFoodDecrement.ini eatBonus.ini maxFoodDecrementSeconds.ini minFoodDecrementSeconds.ini"





echo "" 
echo "About to copy these settings files to all remote servers:"
echo $settingsList
echo ""

# feed file through grep to add newlines at the end of each line
# otherwise, read skips the last line if it doesn't end with newline
while read user server port
do

  echo "  Sending to $server"
  scp $settingsList $user@$server:checkout/OneLife/server/settings/

done <  <( grep "" ~/www/reflector/remoteServerList.ini )


echo "" 
echo "Done updating settings on all servers."
echo ""
