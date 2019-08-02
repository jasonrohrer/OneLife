This script will automate the process of git-pulling and installing your own 
server on a GNU/Linux machine.

Make a folder for it first, like this:

mkdir myServer

Make sure you have read/write permissions to this folder, assuming that your 
user account will be running the server.  If the server is going to be run
by some other account instead, make sure THAT account has read-write
permissions.


Then copy this script into that folder.  Suppose myServer is in your home
directory:

cp serverPullAndBuild.sh ~/myServer


Now change to that directory and run the script:

cd ~/myServer
bash serverPullAndBuild.sh
