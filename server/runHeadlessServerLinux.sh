./makeLogRotator
nohup catchsegv unbuffer ./OneLifeServer 2>&1 | ./logRotator 20000000 serverOut.txt >/dev/null 2>&1 &