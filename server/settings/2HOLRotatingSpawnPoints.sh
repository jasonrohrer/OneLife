#!/bin/bash

#This script must work in pair with a cron job running at a desired interval
#forceEveLocation.ini must be true for this to work

xCoords=(1 2 3 4 5 6 7 8 9 0)
yCoords=(1 2 3 4 5 6 7 8 9 0)

if [[ ${#xCoords[@]} -eq ${#yCoords[@]} ]]; then

    amtCoords=${#xCoords[@]}

    rand=$(($RANDOM%$amtCoords))
    
    echo "${xCoords[$rand]}" #> ./forceEveLocationX.ini
    echo "${yCoords[$rand]}" #> ./forceEveLocationY.ini
    
else
    echo "Invalid coordinates. Arrays must be equal in indexes."
fi
