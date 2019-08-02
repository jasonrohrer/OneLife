# For remote web servers
# no game server running, but web server installed with php/mysql

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



add-apt-repository ppa:ondrej/php
apt-get -o Acquire::ForceIPv4=true update
add-apt-repository ppa:ondrej/php
apt-get -y install emacs-nox nginx fail2ban ufw git mysql-server php5.6-fpm php5.6-mysql php5.6 php5.6-curl php-pear php5.6-xml php-elisp



echo ""
echo ""
echo "Whitelisting only main server IP address for ssh"
echo "Opening port 80 for web"
echo ""
echo ""

ufw allow from 72.14.184.149 to any port 22
ufw allow from 173.230.147.48 to any port 22
ufw allow 80
ufw --force enable



echo ""
echo ""
echo "Setting up new user account jcr13 without a password..."
echo ""
echo ""

useradd -m -s /bin/bash jcr13


su jcr13<<EOSU

cd /home/jcr13

mkdir .ssh

chmod 744 .ssh

cd .ssh

echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAsiD85AwaNxcUzNzHPapVaGVIQCTUfdKT2tyd26MWqEds2UHLZund+S930BWz7guu3/mzuTomJnPxZPSyb+62ZiuAR0YaGZwYwMrFkrbPsXf6//MZDBvdMMcqPyqLj3Iny2ZZ9LTnSIs0hqQ3SksvP/qqHthS1YQWMwZlRxs6MmZdEuy4qZZgpnexf6uaWTEcxEO2Nij8LdEN8+jJZLHVkXSSD9c/ssTmdXss3/sVSZHYR+28HeqUahRsO0Rz6mR3FwB+ZslZlWiMFTzjkH5IA/XOiNK5Ezf1+EsQOn2OVfaCMlfJQ6YGp1kgFI01j2ZWHnqHaacvVc+C+9fAmIscZw==" > authorized_keys

chmod 644 authorized_keys


cd /home/jcr13

mkdir checkout
cd checkout
git clone https://jasonrohrer@github.com/jasonrohrer/OneLife.git

cd /home/jcr13
mkdir public_html

exit
EOSU


cd ~
mkdir checkout
cd checkout
git clone https://jasonrohrer@github.com/jasonrohrer/OneLife.git

cp OneLife/scripts/nginx/sites-available/default /etc/nginx/sites-available/default

nginx -s reload

echo ""
echo "Done with remote setup."
echo ""


