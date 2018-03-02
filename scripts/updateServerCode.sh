# script run on main server to shutdown all servers and update them
# to the latest binary server code.  Does NOT update game data for the servers.


# shuts down each server and rebuilds them, one by one, allowing
# players to continue playing on the other servers in the mean time





echo "" 
echo "Re-compiling non-running local server code base as a sanity check"
echo ""

cd ~/checkout/minorGems
git pull

cd ~/checkout/OneLife/server
git pull

./configure 1
make





~/checkout/OneLifeWorking/scripts/updateServerCodeStep1.sh

~/checkout/OneLifeWorking/scripts/updateServerCodeStep2.sh









echo "" 
echo "Done updating code on all servers."
echo ""
