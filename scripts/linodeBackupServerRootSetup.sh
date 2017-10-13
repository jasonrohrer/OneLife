# For remote backup server

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
apt-get -y install emacs-nox mercurial git fail2ban ufw



echo ""
echo ""
echo "Whitelisting only main server IP address for ssh"
echo ""
echo ""

ufw allow from 72.14.184.149 to any port 22
ufw --force enable



echo ""
echo ""
echo "Setting up new user account jcr13 without a password..."
echo ""
echo ""

useradd -m -s /bin/bash jcr13


su jcr13<<EOSU

cd /home/jcr13


cd /home/jcr13

mkdir backups

mkdir backups/main

mkdir checkout
cd checkout
git clone https://github.com/jasonrohrer/OneLife.git


echo ""
echo "Setting up remote cron job to run backup at hour 10 UTC."
echo ""

cd /home/jcr13

echo "0 10 * * * /home/jcr13/checkout/OneLife/scripts/backupCron.sh" > crontabSource

crontab crontabSource



exit
EOSU


echo ""
echo "Done with remote setup."
echo ""



