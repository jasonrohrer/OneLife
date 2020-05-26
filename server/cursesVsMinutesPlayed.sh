

./listCursedEmails.sh > tempCursedEmails.txt


find lifeLog/ -type f -mtime -30 -mtime +0 -print0 | xargs -0 grep -h "^D " | sed "s/D [0-9]* [0-9]* //" | sed "s/ [FM] .*//" > tempEmailLives.txt


g++ -g -I../.. -o countCursesAndLives countCursesAndLives.cpp


./countCursesAndLives

#rm tempCursedEmails.txt tempEmailLives.txt