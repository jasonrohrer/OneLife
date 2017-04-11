# Run as root to perform initial setup and user account creation

# Runs interactively (asks for a password

apt-get update
apt-get -y install emacs-nox mercurial g++ expect gdb make


echo ""
echo ""
echo "Setting up new user account jcr13..."
echo ""
echo ""

useradd jcr13
mkhomedir_helper jcr13
passwd jcr13


su jcr13
cd ~

mkdir checkout
cd checkout

hg clone http://hg.code.sf.net/p/hcsoftware/OneLife
hg clone http://hg.code.sf.net/p/hcsoftware/OneLifeData7
hg clone http://hg.code.sf.net/p/minorgems/minorGems

cd OneLife/server

echo "http://onehouronelife.com/ticketServer/server.php" > settings/ticketServerURL.ini

ln -s ../../OneLifeData7/objects .
ln -s ../../OneLifeData7/transitions .

./configure 1

make

./runHeadlessServerLinux.sh


exit