# this script can be called via SSH on a remote server
# it launches a headless process that puts the server in shutdown mode
# and waits for it to exit before updating it and restarting it


nohup unbuffer ./remoteServerUpdateHeadless.sh >> remoteUpdateOut.txt &