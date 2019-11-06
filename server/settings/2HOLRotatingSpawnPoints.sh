#!/bin/bash

#This script must work in pair with a cron job running at a desired interval
#forceEveLocation.ini must be true for this to work

xCoords=(500 500 -500 -500)
yCoords=(500 -500 500 -500)

if [[ ${#xCoords[@]} -eq ${#yCoords[@]} ]]; then

    amtCoords=${#xCoords[@]}

    rand=$(($RANDOM%$amtCoords))
    
    echo "${xCoords[$rand]}" #> ./forceEveLocationX.ini #Must be the full file path
    echo "${yCoords[$rand]}" #> ./forceEveLocationY.ini #Must be the full file path
    
else
    echo "Invalid coordinates. Arrays must be equal in indexes."
fi
