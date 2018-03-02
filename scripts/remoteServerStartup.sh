echo "" 
echo "Re-launching server"
echo ""


echo -n "0" > ~/checkout/OneLife/server/settings/shutdownMode.ini



cd ~/checkout/OneLife/server/

sh ./runHeadlessServerLinux.sh

echo -n "1" > ~/keepServerRunning.txt
