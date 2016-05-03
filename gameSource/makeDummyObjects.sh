#!/bin/sh


# usage makeDummyObjects.sh idToCopy numDummies

if [ "$#" -lt "2" ] ; then
   echo "usage makeDummyObjects.sh idToCopy numDummies"
   exit 1
fi



idToCopy="$1"
numDummies="$2"


nextID=`cat objects/nextObjectNumber.txt`

echo "Copying object $idToCopy $numDummies times, starting with dest $nextID"

count=0;

until [ $count -ge $numDummies ]
do
  cp "objects/$idToCopy.txt" "objects/$nextID.txt"
  
  sed "s/id=$idToCopy/id=$nextID/" "objects/$nextID.txt" > objects/temp.txt

  mv objects/temp.txt  "objects/$nextID.txt"

  count=`expr $count + 1`
  nextID=`expr $nextID + 1`
done

echo "New nextID = $nextID"

echo -n "$nextID" > objects/nextObjectNumber.txt


rm objects/cache.fcz