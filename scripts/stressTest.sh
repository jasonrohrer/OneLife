
# NOTE
# Server must have password checking and ticket server hash checking off
# for this stress test to work

for i in `seq 1 100`;
do
	echo -n "LOGIN test$i@test.com a a#" | nc -q 20 192.168.1.3 8005 &
done    