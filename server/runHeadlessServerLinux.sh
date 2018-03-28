./makeLogRotator
nohup catchsegv unbuffer ./OneLifeServer | ./logRotator 20000000 serverOut.txt >/dev/null &