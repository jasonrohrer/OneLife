#!/bin/sh


# usage makeDummySprites.sh idToCopy numDummies

if [ "$#" -lt "2" ] ; then
   echo "usage makeDummySprites.sh idToCopy numDummies"
   exit 1
fi



idToCopy="$1"
numDummies="$2"


nextID=`cat sprites/nextSpriteNumber.txt`

echo "Copying sprite $idToCopy $numDummies times, starting with dest $nextID"

count=0;

until [ $count -ge $numDummies ]
do
  cp "sprites/$idToCopy.tga" "sprites/$nextID.tga"
  
  echo -n "dummy$nextID $nextID" > "sprites/$nextID.txt" 

  count=`expr $count + 1`
  nextID=`expr $nextID + 1`
done

echo "New nextID = $nextID"

echo -n "$nextID" > sprites/nextSpriteNumber.txt


rm sprites/cache.fcz