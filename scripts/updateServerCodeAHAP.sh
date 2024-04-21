# script run on main server to shutdown all servers and update them
# to the latest binary server code.  Does NOT update game data for the servers.


# shuts down each server and rebuilds them, one by one, allowing
# players to continue playing on the other servers in the mean time





echo "" 
echo "Re-compiling non-running local server code base as a sanity check"
echo ""

cd ~/checkout/minorGems
git pull --tags

cd ~/checkout/OneLife/server
git pull --tags

./configure 1
make


echo "About to post AHAP server code (WITHOUT tagging on git)."
echo ""
echo -n "Hit [ENTER] when ready: "
read



wipeMaps=0

echo ""
echo ""
echo "Auto wipe maps as part of process?"
echo ""
echo "Enter WIPE to wipe, or press [ENTER] to skip."
echo ""
echo -n "Wipe maps: "
read wipeMapsWord

if [ "$wipeMapsWord" = "WIPE" ]
then
	echo
	echo "Auto-wiping maps."
	echo
	wipeMaps=1
else
	echo
	echo "NOT Auto-wiping maps."
	echo
fi





~/checkout/OneLifeWorking/scripts/updateServerCodeAHAPStep1.sh

~/checkout/OneLifeWorking/scripts/updateServerCodeAHAPStep2.sh $wipeMaps









echo "" 
echo "Done updating code on all AHAP servers."
echo ""
