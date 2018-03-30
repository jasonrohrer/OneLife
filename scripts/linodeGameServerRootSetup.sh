# For remote game servers

# Run as root to perform initial setup and user account creation



echo ""
echo ""
echo "Future root ssh logins will be key-based only, with password forbidden"
echo ""
echo ""


mkdir .ssh

chmod 744 .ssh

cd .ssh

echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAsiD85AwaNxcUzNzHPapVaGVIQCTUfdKT2tyd26MWqEds2UHLZund+S930BWz7guu3/mzuTomJnPxZPSyb+62ZiuAR0YaGZwYwMrFkrbPsXf6//MZDBvdMMcqPyqLj3Iny2ZZ9LTnSIs0hqQ3SksvP/qqHthS1YQWMwZlRxs6MmZdEuy4qZZgpnexf6uaWTEcxEO2Nij8LdEN8+jJZLHVkXSSD9c/ssTmdXss3/sVSZHYR+28HeqUahRsO0Rz6mR3FwB+ZslZlWiMFTzjkH5IA/XOiNK5Ezf1+EsQOn2OVfaCMlfJQ6YGp1kgFI01j2ZWHnqHaacvVc+C+9fAmIscZw==" > authorized_keys

chmod 644 authorized_keys

sed -i 's/PermitRootLogin yes/PermitRootLogin without-password/' /etc/ssh/sshd_config

service ssh restart


echo ""
echo ""
echo "Installing packages..."
echo ""
echo ""



apt-get -o Acquire::ForceIPv4=true update
apt-get -y install emacs-nox mercurial git g++ expect gdb make fail2ban ufw


echo ""
echo ""
echo "Whitelisting only main server and backup server IP address for ssh"
echo "Opening port 8005 for game server"
echo ""
echo ""

ufw allow from 72.14.184.149 to any port 22
ufw allow from 173.230.147.48 to any port 22
ufw allow 8005
ufw --force enable

echo ""
echo ""
echo "Setting up custom core file name"
echo ""
echo ""

echo "core.%e.%p.%t" > /proc/sys/kernel/core_pattern

echo "" >> /etc/sysctl.conf
echo "" >> /etc/sysctl.conf
echo "# custom core file name" >> /etc/sysctl.conf
echo "kernel.core_pattern=core.%e.%p.%t" >> /etc/sysctl.conf



echo ""
echo ""
echo "Setting up new user account jcr13 without a password..."
echo ""
echo ""

useradd -m -s /bin/bash jcr13


dataName="OneLifeData7"


su jcr13<<EOSU

cd /home/jcr13

echo "ulimit -c unlimited >/dev/null 2>&1" >> ~/.bash_profile

ulimit -c unlimited

mkdir checkout
cd checkout



echo "Using data repository $dataName"

git clone https://github.com/jasonrohrer/OneLife.git
git clone https://github.com/jasonrohrer/$dataName.git
git clone https://github.com/jasonrohrer/minorGems.git


cd $dataName

lastTaggedDataVersion=\`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//'\`


echo "" 
echo "Most recent Data git version is:  \$lastTaggedDataVersion"
echo ""

git checkout -q OneLife_v\$lastTaggedDataVersion



cd ../OneLife/server

echo "http://onehouronelife.com/ticketServer/server.php" > settings/ticketServerURL.ini

ln -s ../../$dataName/objects .
ln -s ../../$dataName/transitions .
ln -s ../../$dataName/categories .
ln -s ../../$dataName/dataVersionNumber.txt .

git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags | sed -e 's/OneLife_v//' > serverCodeVersionNumber.txt


./configure 1

make

./runHeadlessServerLinux.sh

echo -n "1" > ~/keepServerRunning.txt

crontab /home/jcr13/checkout/OneLife/scripts/remoteServerCrontabSource


cd /home/jcr13
mkdir .ssh

chmod 744 .ssh

cd .ssh

echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAsiD85AwaNxcUzNzHPapVaGVIQCTUfdKT2tyd26MWqEds2UHLZund+S930BWz7guu3/mzuTomJnPxZPSyb+62ZiuAR0YaGZwYwMrFkrbPsXf6//MZDBvdMMcqPyqLj3Iny2ZZ9LTnSIs0hqQ3SksvP/qqHthS1YQWMwZlRxs6MmZdEuy4qZZgpnexf6uaWTEcxEO2Nij8LdEN8+jJZLHVkXSSD9c/ssTmdXss3/sVSZHYR+28HeqUahRsO0Rz6mR3FwB+ZslZlWiMFTzjkH5IA/XOiNK5Ezf1+EsQOn2OVfaCMlfJQ6YGp1kgFI01j2ZWHnqHaacvVc+C+9fAmIscZw==" > authorized_keys

chmod 644 authorized_keys


exit
EOSU

echo ""
echo "Done with remote setup."
echo ""

