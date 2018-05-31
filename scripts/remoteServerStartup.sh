echo "" 
echo "Re-launching server"
echo ""

echo "    Setting shutdownMode to 0"
echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

echo "    Running runHeadlessServerLinux"

# use login shell to ensure that cores are dumped properly
bash -l ./runHeadlessServerLinux.sh

echo "    Setting keepServerRunning flag"
echo -n "1" > ~/keepServerRunning.txt


echo "    Done re-launching server"
