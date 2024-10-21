# run as root

tail -n 200000 /var/log/nginx/access.log | sed "s/ -.*//" | sort | uniq -c | sort -n | \
while read line ; do
	# split into parts
	parts=($line)
	count=${parts[0]}
	ip=${parts[1]}
	

	# 100k let's selb's api bot through, which does less than 50k
	if [[ $count -gt 100000 ]];
	then
		echo "blocking future connections from $ip"
		echo "since it has made $count requests recently"
		
		alreadyBlocked="$( ufw status | grep -c $ip )"

		if [[ $alreadyBlocked -eq 0 ]];
		then
			ufw insert 5 deny from $ip to any port 443
			ufw insert 5 deny from $ip to any port 80
		else
			echo "already blocked by ufw, doing nothing."
		fi
	fi
done