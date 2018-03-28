./makeLogRotator
nohup catchsegv unbuffer ./OneLifeServer | ./logRotator 400000000 serverOut.txt >/dev/null &