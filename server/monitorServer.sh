serverPID=`pidof OneLifeServer`

echo "Server PID = $serverPID"

# 360 is 30 minutes in 5-second steps 
i=1
while [ $i -lt 360 ]
do 

	numPlayers=`nc -w 1 localhost 8005 | head -2 | tail -1 | sed 's/\/.*//'`

	cpu=`top -b -n 1 -p $serverPID | grep $serverPID | awk '{print \$9}'`

	cpuPerPlayer=`bc <<< "scale=2; $cpu / $numPlayers"`

	echo "$i $numPlayers $cpu $cpuPerPlayer"

	sleep 5

	i=$((i+1))
done
