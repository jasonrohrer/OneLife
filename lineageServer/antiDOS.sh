# run as root


# these IPs are our game servers
# they are never blocked
whiteList="104.237.138.91 \
45.33.20.31      \
96.126.120.148   \
45.79.97.51      \
173.255.255.33   \
104.237.138.161	 \
74.207.228.57	 \
45.33.78.109	 \
50.116.0.119	 \
50.116.22.16	 \
74.207.232.209	 \
45.56.113.253	 \
45.33.80.159	 \
45.33.47.27	     \
45.33.118.185	 \
192.155.92.146	 \
172.104.22.245   \
45.79.79.186"



tail -n 400000 /var/log/nginx/access.log | sed "s/ -.*//" | sort | uniq -c | sort -n | \
while read line ; do
	# split into parts
	parts=($line)
	count=${parts[0]}
	ip=${parts[1]}

	if [[ $count -gt 2000 ]];
	then
		if echo "$whiteList" | grep -q "$ip"; then
			# IP on whitelist, do nothing
			echo "IP $ip has $count requests recently, but it's on white list."
		else
			echo "blocking future connections from $ip"
			echo "since it has made $count requests recently"
		
			alreadyBlocked="$( ufw status | grep -c $ip )"

			if [[ $alreadyBlocked -eq 0 ]];
			then
				ufw insert 3 deny from $ip to any port 443
				ufw insert 3 deny from $ip to any port 80
			else
				echo "already blocked by ufw, doing nothing."
			fi
		fi
	fi
done
