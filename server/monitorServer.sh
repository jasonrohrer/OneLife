serverPID=`pidof OneLifeServer`

echo "Server PID = $serverPID"


numPlayers=`nc -w 1 localhost 8005 | head -2 | tail -1 | sed 's/\/.*//'`

echo "Num players = $numPlayers"


cpu=`top -b -n 1 -p $serverPID | grep $serverPID | awk '{print \$9}'`

echo "CPU Usage = $cpu"

cpuPerPlayer=`bc <<< "scale=2; $cpu / $numPlayers"`


echo "CPU per player $cpuPerPlayer"